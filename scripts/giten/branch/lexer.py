"""Lexical source transforms for the generated branches, without parsing C.

Removed spellings (comments, claims) become a REMOVED sentinel so that
`finish` can drop a line that held nothing else instead of leaving a blank
line in its place. Tokens are never joined: a removal inside a line reads as
one space.
"""

from __future__ import annotations

import re

REMOVED = "\x01"

#: Comments that survive: licence notices and formatter directives.
_KEPT_COMMENT = re.compile(r"copyright|SPDX-License|clang-format (?:off|on)", re.I)


def tokens(text: str):
    """Yield (kind, spelling): comment, literal, word or punctuation."""
    i = 0
    n = len(text)
    while i < n:
        start = i
        if text.startswith("//", i):
            i = text.find("\n", i)
            if i < 0:
                i = n
            while i < n and text[i - 1] == "\\":
                following = text.find("\n", i + 1)
                i = n if following < 0 else following
            yield "comment", text[start:i]
        elif text.startswith("/*", i):
            end = text.find("*/", i + 2)
            if end < 0:
                raise ValueError("unterminated block comment")
            i = end + 2
            yield "comment", text[start:i]
        elif text[i] in "\"'":
            quote = text[i]
            i += 1
            while i < n and text[i] != quote:
                if text[i] == "\n":
                    raise ValueError("newline in a quoted literal")
                i += 2 if text[i] == "\\" else 1
            if i >= n:
                raise ValueError("unterminated quoted literal")
            i += 1
            yield "literal", text[start:i]
        elif text[i].isalpha() or text[i] == "_":
            i += 1
            while i < n and (text[i].isalnum() or text[i] == "_"):
                i += 1
            yield "word", text[start:i]
        else:
            i += 1
            yield "punctuation", text[start:i]


def words(text: str) -> set[str]:
    return {spelling for kind, spelling in tokens(text) if kind == "word"}


def strip_comments(text: str) -> str:
    """Replace every comment but licence notices, formatter directives and
    glosses: a non-ASCII comment on the line of a string literal, which reads
    an escaped Shift-JIS literal (`"\\202\\141" // "Ｂ"`)."""
    stream = list(tokens(text))
    line = 0
    literal_lines = set()
    starts = []
    for kind, spelling in stream:
        starts.append(line)
        if kind == "literal" and spelling.startswith('"'):
            literal_lines.add(line)
        line += spelling.count("\n")
    out = []
    for (kind, spelling), start in zip(stream, starts):
        gloss = start in literal_lines and not spelling.isascii()
        if kind == "comment" and not (gloss or _KEPT_COMMENT.search(spelling)):
            out.append(REMOVED + ("\n" + REMOVED) * spelling.count("\n"))
        else:
            out.append(spelling)
    return "".join(out)


def rewrite_macros(text: str, calls: dict, objects: dict[str, str] | None = None) -> str:
    """Expand function-like macros `calls` {name: (arity, transform(args))} and
    object-like macros `objects` {name: replacement} outside literals."""
    objects = objects or {}
    parts = list(tokens(text))

    def rewrite(parts) -> str:
        output = []
        i = 0
        while i < len(parts):
            kind, word = parts[i]
            if kind == "word" and word in objects:
                output.append(objects[word])
                i += 1
                continue
            if kind != "word" or word not in calls:
                output.append(word)
                i += 1
                continue
            opening = i + 1
            while opening < len(parts) and parts[opening][1].isspace():
                opening += 1
            if opening == len(parts) or parts[opening][1] != "(":
                raise ValueError(f"{word}: expected a macro invocation")
            stack = ["("]
            args = []
            start = cursor = opening + 1
            while cursor < len(parts):
                token_kind, spelling = parts[cursor]
                if token_kind == "punctuation":
                    if spelling in "([{":
                        stack.append(spelling)
                    elif spelling in ")]}":
                        if stack.pop() != {")": "(", "]": "[", "}": "{"}[spelling]:
                            raise ValueError(f"{word}: unbalanced macro arguments")
                        if not stack:
                            args.append(rewrite(parts[start:cursor]).strip())
                            break
                    elif spelling == "," and len(stack) == 1:
                        args.append(rewrite(parts[start:cursor]).strip())
                        start = cursor + 1
                cursor += 1
            else:
                raise ValueError(f"{word}: unterminated macro invocation")
            arity, transform = calls[word]
            if len(args) != arity:
                raise ValueError(f"{word}: expected {arity} argument(s), got {len(args)}")
            output.append(transform(args))
            i = cursor + 1
        return "".join(output)

    return rewrite(parts)


_DIRECTIVE = re.compile(r"[ \t]*#[ \t]*(if|ifdef|ifndef|elif|else|endif)\b(.*)", re.S)
_CONDITION_TOKEN = re.compile(r"\s*(defined|&&|\|\||!|\(|\)|[A-Za-z_]\w*)")


def _mentions(expression: str, names) -> bool:
    return any(re.search(rf"\b{name}\b", expression) for name in names)


def _evaluate(expression: str, defined: dict[str, bool]) -> bool:
    """`defined(NAME)` / `defined NAME` with !, &&, || and parentheses."""
    stream = []
    position = 0
    text = expression.strip()
    while position < len(text):
        match = _CONDITION_TOKEN.match(text, position)
        if not match:
            raise ValueError(f"unsupported conditional: {expression.strip()}")
        stream.append(match.group(1))
        position = match.end()
    cursor = 0

    def peek():
        return stream[cursor] if cursor < len(stream) else None

    def take(expected=None):
        nonlocal cursor
        token = peek()
        if token is None or (expected is not None and token != expected):
            raise ValueError(f"unsupported conditional: {expression.strip()}")
        cursor += 1
        return token

    def primary() -> bool:
        token = take()
        if token == "!":
            return not primary()
        if token == "(":
            value = disjunction()
            take(")")
            return value
        if token == "defined":
            parenthesized = peek() == "("
            if parenthesized:
                take("(")
            name = take()
            if parenthesized:
                take(")")
            if name not in defined:
                raise ValueError(f"unsupported conditional: {expression.strip()}")
            return defined[name]
        raise ValueError(f"unsupported conditional: {expression.strip()}")

    def conjunction() -> bool:
        value = primary()
        while peek() == "&&":
            take()
            value = primary() and value
        return value

    def disjunction() -> bool:
        value = conjunction()
        while peek() == "||":
            take()
            value = conjunction() or value
        return value

    value = disjunction()
    if cursor != len(stream):
        raise ValueError(f"unsupported conditional: {expression.strip()}")
    return value


def _condition(command: str, expression: str, defined: dict[str, bool]) -> bool | None:
    if command in ("ifdef", "ifndef"):
        name = expression.strip()
        if name not in defined:
            if _mentions(name, defined):
                raise ValueError(f"unsupported conditional: #{command} {name}")
            return None
        return defined[name] if command == "ifdef" else not defined[name]
    if not _mentions(expression, defined):
        return None
    return _evaluate(expression, defined)


def resolve_conditionals(text: str, defined: dict[str, bool]) -> str:
    """Decide every conditional on the macros in `defined` ({name: defined?}).

    A decided conditional keeps only its selected arm; its directives go.
    Other conditionals stay, whatever they contain. A condition that mixes a
    decided macro with anything but defined(), !, && and || is an error.
    """
    lines = text.splitlines(keepends=True)
    stack: list[list[bool] | None] = []
    output = []
    i = 0
    while i < len(lines):
        logical = lines[i]
        i += 1
        while logical.rstrip("\r\n").endswith("\\") and i < len(lines):
            logical += lines[i]
            i += 1
        match = _DIRECTIVE.match(logical)
        emit = True
        if match:
            command = match.group(1)
            expression = match.group(2).replace("\\\n", " ")
            if command in ("if", "ifdef", "ifndef"):
                condition = _condition(command, expression, defined)
                stack.append(None if condition is None else [condition, condition])
                emit = condition is None
            elif command in ("elif", "else"):
                if not stack:
                    raise ValueError(f"unmatched #{command}")
                frame = stack[-1]
                if frame is None:
                    if command == "elif" and _mentions(expression, defined):
                        raise ValueError(f"unsupported conditional arm: #elif {expression.strip()}")
                else:
                    if command == "else":
                        condition = True
                    else:
                        condition = _condition("if", expression, defined)
                        if condition is None:
                            raise ValueError(
                                f"unsupported conditional arm: #elif {expression.strip()}")
                    frame[0] = not frame[1] and condition
                    frame[1] = frame[1] or frame[0]
                    emit = False
            else:
                if not stack:
                    raise ValueError("unmatched #endif")
                emit = stack.pop() is None
        if emit and all(frame is None or frame[0] for frame in stack):
            output.append(logical)
    if stack:
        raise ValueError("unterminated conditional")
    return "".join(output)


def finish(text: str) -> str:
    """Drop lines that held only removed spellings; tidy blank lines."""
    if "\0" in text:
        raise ValueError("NUL in source text")
    lines = []
    for line in text.split("\n"):
        if REMOVED in line and not line.replace(REMOVED, "").strip():
            continue
        lines.append(line.replace(REMOVED, " ").rstrip())
    text = "\n".join(lines)
    return re.sub(r"\n{3,}", "\n\n", text).strip("\n") + "\n"
