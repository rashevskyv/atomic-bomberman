"""Collect the English texts of tr() and trf() calls into the template data/lang/en.json.

Usage:
  python tools/i18n/extract_strings.py          write data/lang/en.json
  python tools/i18n/extract_strings.py --check  check en.json and every data/lang/*.json

The template maps each English text to itself: it is the list of keys a translation
must cover. --check fails when en.json is out of date, when a translation has a key
the code does not use, or when its placeholders ({0}, {1}, ...) differ from the English.
Only the literal first argument of tr() and trf() is read; text built at run time is
not, so such text cannot be translated (say so in a comment where it is built).
"""
import glob
import json
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SOURCE_DIRS = ("src",)
CALL = re.compile(r'\btrf?\(\s*"((?:[^"\\\n]|\\.)*)"')
PLACEHOLDER = re.compile(r"\{(\d+)\}")
ESCAPES = {"n": "\n", "t": "\t", "r": "\r", '"': '"', "\\": "\\", "'": "'"}


def decode(literal):
    out = []
    i = 0
    while i < len(literal):
        c = literal[i]
        if c == "\\" and i + 1 < len(literal):
            out.append(ESCAPES.get(literal[i + 1], literal[i + 1]))
            i += 2
            continue
        out.append(c)
        i += 1
    return "".join(out)


def english_texts():
    found = {}
    for folder in SOURCE_DIRS:
        for dirpath, _, names in os.walk(os.path.join(ROOT, folder)):
            for name in sorted(names):
                if not name.endswith((".cpp", ".hpp")):
                    continue
                path = os.path.join(dirpath, name)
                with open(path, encoding="utf-8") as f:
                    code = "\n".join(line for line in f.read().split("\n") if not line.lstrip().startswith("//"))
                    for m in CALL.finditer(code):
                        found.setdefault(decode(m.group(1)), os.path.relpath(path, ROOT).replace(os.sep, "/"))
    return found


def write_template(texts):
    path = os.path.join(ROOT, "data", "lang", "en.json")
    os.makedirs(os.path.dirname(path), exist_ok=True)
    body = {k: k for k in sorted(texts)}
    with open(path, "w", encoding="utf-8", newline="\n") as f:
        json.dump(body, f, ensure_ascii=False, indent=4)
        f.write("\n")
    print(f"wrote {path}: {len(body)} texts")


def check(texts):
    problems = []
    path = os.path.join(ROOT, "data", "lang", "en.json")
    with open(path, encoding="utf-8") as f:
        template = json.load(f)
    if set(template) != set(texts) or any(template[k] != k for k in template):
        problems.append("en.json is out of date: run tools/i18n/extract_strings.py")
    for lang in sorted(glob.glob(os.path.join(ROOT, "data", "lang", "*.json"))):
        if os.path.basename(lang) == "en.json":
            continue
        with open(lang, encoding="utf-8") as f:
            table = json.load(f)
        name = os.path.basename(lang)
        for key, value in table.items():
            if key not in texts:
                problems.append(f"{name}: key not used by the code: {key[:60]!r}")
                continue
            if sorted(PLACEHOLDER.findall(key)) != sorted(PLACEHOLDER.findall(value)):
                problems.append(f"{name}: placeholders differ: {key[:60]!r}")
        missing = [k for k in texts if k not in table]
        print(f"{name}: {len(table)} texts, {len(missing)} still in English")
        if "--missing" in sys.argv:
            for k in missing:
                print(f"  missing: {k}")
    for p in problems:
        print("FAIL", p)
    return 1 if problems else 0


if __name__ == "__main__":
    found = english_texts()
    if "--check" in sys.argv:
        sys.exit(check(found))
    write_template(found)
