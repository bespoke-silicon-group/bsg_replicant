#!/usr/bin/env python3
"""Record DRAMSim3 build options without touching an unchanged stamp."""
import argparse
import json
import os
from pathlib import Path
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    parser.add_argument('--compiler', required=True)
    parser.add_argument('--flags', required=True)
    parser.add_argument('--link-flags', required=True)
    parser.add_argument('--source', required=True, type=Path)
    args = parser.parse_args()
    if '-DDRAMSIM3_NO_STATISTICS' in args.flags:
        header = (args.source / 'src/simple_stats.h').read_text()
        if '#define DRAMSIM3_STATISTICS_CONTROL 1' not in header:
            parser.error('exec requires DRAMSim3 statistics-control support; '
                         'update the BaseJump DRAMSim3 submodule')
    content = json.dumps(dict(compiler=args.compiler, flags=args.flags,
                              link_flags=args.link_flags, source=str(args.source)),
                         sort_keys=True, indent=2) + '\n'
    if args.output.exists() and args.output.read_text() == content:
        return
    args.output.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.NamedTemporaryFile(mode='w', dir=args.output.parent,
                                     prefix=args.output.name + '.', delete=False) as stream:
        stream.write(content)
        temporary = Path(stream.name)
    try:
        os.replace(temporary, args.output)
    finally:
        temporary.unlink(missing_ok=True)


if __name__ == '__main__':
    main()
