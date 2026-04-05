#!/usr/bin/env python3
"""Generate i18n.hpp from i18n.tpl by converting encoding.

Usage: gen_i18n.py <input.tpl> <output.hpp> <encoding>
"""
import sys

def main():
    if len(sys.argv) != 4:
        print(f"Usage: {sys.argv[0]} <input> <output> <encoding>", file=sys.stderr)
        sys.exit(1)
    src, dst, enc = sys.argv[1], sys.argv[2], sys.argv[3]
    data = open(src, 'rb').read().decode('utf8')
    data = data.replace('\u301c', '\uff5e')
    open(dst, 'wb').write(data.encode(enc, 'replace'))

if __name__ == '__main__':
    main()
