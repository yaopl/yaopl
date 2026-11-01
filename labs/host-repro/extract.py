"""Copy named top-level C functions, #defines and structs out of an OPL source file.

Usage: extract.py <source.c> <name> [<name> ...]
A name matches a function definition (`... name(` at column 0, body up to the
next `}` at column 0), a `#define name` (with its continuation lines), a
`struct name` block, a `typedef struct { ... } name;` block or a one-line
`static` variable (`static int name = 1;`). A name that starts with `?` may
be absent (a helper that only a fixed tree has). Output goes to stdout in
source order.
"""
import re
import sys


def main():
    path = sys.argv[1]
    required = set(n for n in sys.argv[2:] if not n.startswith("?"))
    names = set(n.lstrip("?") for n in sys.argv[2:])
    lines = open(path, encoding="latin-1").read().splitlines()
    out, found = [], set()
    i = 0
    while i < len(lines):
        line = lines[i]
        m = re.match(r"#define\s+(\w+)", line)
        if m and m.group(1) in names:
            j = i
            while lines[j].rstrip().endswith("\\"):
                j += 1
            out.extend(lines[i : j + 1])
            found.add(m.group(1))
            i = j + 1
            continue
        m = re.match(r"^static\s+[^()=;]*?\b(\w+)\s*(\[[^\]]*\])?\s*(=[^;]*)?;\s*$", line)
        if m and m.group(1) in names:
            out.append(line)
            found.add(m.group(1))
            i += 1
            continue
        m = re.match(r"^struct\s+(\w+)\s*$", line)
        if m and m.group(1) in names:
            j = i
            while not lines[j].startswith("}"):
                j += 1
            out.extend(lines[i : j + 1])
            out.append("")
            found.add(m.group(1))
            i = j + 1
            continue
        if re.match(r"^typedef struct(\s+\w+)?\s*$", line):
            j = i
            while not lines[j].startswith("}"):
                j += 1
            m = re.match(r"^\}\s*(\w+)\s*;", lines[j])
            if m and m.group(1) in names:
                out.extend(lines[i : j + 1])
                out.append("")
                found.add(m.group(1))
                i = j + 1
                continue
        m = re.match(r"^[A-Za-z_].*?\b(\w+)\s*\(", line)
        if m and m.group(1) in names and not line.rstrip().endswith(";"):
            j = i
            while lines[j] != "}":
                j += 1
            out.extend(lines[i : j + 1])
            out.append("")
            found.add(m.group(1))
            i = j + 1
            continue
        i += 1
    missing = required - found
    if missing:
        sys.exit("not found in %s: %s" % (path, ", ".join(sorted(missing))))
    print("\n".join(out))


main()
