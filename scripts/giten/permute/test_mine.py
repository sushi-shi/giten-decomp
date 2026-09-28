"""Selection and persistent accounting at the unattended search boundary."""
from pathlib import Path
import tempfile
import unittest

from giten.permute.mine import collect_run, population, search_route
from giten.permute.upstream import restore_campaign, write_checkpoint


class MiningTests(unittest.TestCase):
    def test_exact_and_historical_matches_are_not_random_searches(self):
        def entry(score, hist):
            return dict(best=score, hist=hist, addr=1, fp='hash')
        bank = {('unit', 'near'): entry(99.8, 99.8),
                ('unit', 'lost'): entry(99.9, 100),
                ('unit', 'exact'): entry(100, 100),
                ('unit', 'low'): entry(98, 99)}
        rows = population(bank, {'unit': {'source': 'src/unit.c'}}, 99)
        self.assertEqual([row['symbol'] for row in rows], ['lost', 'near'])
        self.assertEqual(search_route(rows[0], {'classification': 'regalloc'}), 'history-recovery')
        self.assertEqual(search_route(rows[1], {'classification': 'cfg'}), 'review-cfg')
        self.assertEqual(search_route(rows[1], {'classification': 'regalloc'}), 'random')

    def test_repeated_sources_do_not_inflate_compiled_mutations(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory)
            write_checkpoint(path / 'results.json', {
                'source_sha256': 'baseline', 'attempted_variant_count': 7,
                'elapsed_seconds': 3, 'best': {'score': 99.5}, 'results': [
                    {'source_sha256': 'baseline', 'state_id': 'a', 'score': 99},
                    {'source_sha256': 'mutant', 'state_id': 'b', 'score': 99.5},
                    {'source_sha256': 'mutant', 'state_id': 'b', 'score': 99.5},
                    {'source_sha256': 'failed', 'score': None},
                ],
            })
            result = collect_run(path, 'variants')
            self.assertEqual(result['unique_scored_mutations'], 1)
            self.assertEqual(result['distinct_target_states'], 2)

    def test_resume_keeps_deduplication_and_refuses_changed_source(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory)
            result = path / 'round-001.results'
            (result / 'frontier').mkdir(parents=True)
            original = b'prefix\nint f() { return 0; }\nsuffix\n'
            start, end = original.index(b'int f'), original.index(b'\nsuffix')
            candidate = 'int f() { return 1; }'
            (result / 'frontier/one.c').write_bytes(original[:start] + candidate.encode() + original[end:])
            write_checkpoint(result / 'frontier/frontier.json', [{'source': 'one.c'}])
            write_checkpoint(result / 'input.json', {'axes': [{'options': [{'replace': candidate}]}]})
            write_checkpoint(result / 'results.json', {'source_restored': True, 'results': [
                {'source_sha256': 'mutant', 'state_id': 'machine', 'score': 99.5},
            ]})
            expected = {'source_sha256': 'original'}
            write_checkpoint(path / 'campaign.json', {**expected, 'rounds': [{'round': 1}]})
            _, parents, seen, scored, states = restore_campaign(
                path, expected, original, start, end, original[start:end].decode())
            self.assertIn(candidate, parents)
            self.assertIn(candidate, seen)
            self.assertEqual(scored, {'mutant'})
            self.assertEqual(states, {'machine'})
            with self.assertRaisesRegex(ValueError, 'source_sha256 changed'):
                restore_campaign(path, {'source_sha256': 'edited'}, original, start, end, '')


if __name__ == '__main__':
    unittest.main()
