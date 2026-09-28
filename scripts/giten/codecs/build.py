"""Build the differential runner against unmodified candidate COFF objects."""
from pathlib import Path
import subprocess
from giten.core.paths import REPO, BUILD
from giten.tool import cl, link
from giten.tool.wine import winepath

UNITS = ('datafile', 'bmpseek', 'resourceblit', 'midistream', 'areamap', 'itemrecord', 'range', 'd3dapp', 'textchar')


def build():
    out = BUILD / 'codecs'
    out.mkdir(parents=True, exist_ok=True)
    subprocess.run(['python3', '-m', 'giten', 'configure'], check=True)
    objects = [BUILD / 'objdiff/base' / (unit + '.obj') for unit in UNITS]
    subprocess.run(['ninja', '-f', 'build/build.ninja',
                    'build/exe/DDS.EXE', *[str(p.relative_to(REPO)) for p in objects]], cwd=REPO, check=True)
    obj = out / 'runner.obj'
    exe = out / 'runner.exe'
    exe.unlink(missing_ok=True)
    (out / 'compile.log').write_text(cl.compile(
        Path(__file__).with_name('runner.cpp'), obj,
        ['/nologo', '/c', '/Ox', '/Zp1', '/ML', '/GX']))
    (out / 'link.log').write_text(link.link([
        '/NOLOGO', '/BASE:0x10000000', '/INCREMENTAL:NO', '/SUBSYSTEM:CONSOLE',
        '/FORCE:UNRESOLVED', '/OUT:' + winepath(exe),
        winepath(obj), *[winepath(p) for p in objects], 'kernel32.lib', 'winmm.lib', 'user32.lib', 'gdi32.lib', 'dsound.lib', 'ddraw.lib', 'dinput.lib', 'dxguid.lib'],
        cwd=out, expect=[exe]))
    return exe


if __name__ == '__main__':
    print(build())
