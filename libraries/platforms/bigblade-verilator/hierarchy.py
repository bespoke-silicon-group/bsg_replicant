#!/usr/bin/env python3
"""Guard processor hierarchy generation; no RTL rewriting or tool patching."""
import argparse
import json
from pathlib import Path
import re
import subprocess
import sys


def stamp(path, mode, threads, identity):
    if mode not in ("processor", "flat"):
        raise ValueError("VERILATOR_HIERARCHY must be processor or flat")
    if not threads.isdecimal() or int(threads) < 1:
        raise ValueError("VERILATOR_THREADS must be a positive integer")
    if mode == "processor" and int(threads) != 1:
        raise ValueError("processor hierarchy is validated only with VERILATOR_THREADS=1; "
                         "use VERILATOR_HIERARCHY=flat for multithreaded experiments")
    content = json.dumps(dict(mode=mode, threads=int(threads), identity=identity),
                         sort_keys=True, indent=2) + "\n"
    if not path.exists() or path.read_text() != content:
        path.write_text(content)


def check_reuse(directory, top):
    # A child archive alone is insufficient: 5.050 can emit an unused library
    # while leaving the parent flattened. Require parent imports AND call sites.
    dpi = directory / ("V" + top + "__Dpi.h")
    pattern = r"\bbsg_manycore_proc_vanilla_[a-zA-Z0-9_]+_protectlib_combo_update\b"
    imports = set(re.findall(pattern, dpi.read_text())) if dpi.exists() else set()
    calls = set()
    for source in directory.glob("V" + top + "*.cpp"):
        calls.update(re.findall(r"(?m)^\s*(?:\w+\s*=\s*)?(" + pattern + r")\s*\(",
                                source.read_text()))
    if not imports or not imports.issubset(calls):
        raise ValueError("processor hierarchy was NOT used by the parent model; "
                         "refusing silent flattening. Use VERILATOR_HIERARCHY=flat "
                         "or inspect this Verilator version's hierarchical support")
    return sorted(imports)


def generate(directory, top, mode, command):
    directory = directory.resolve()
    config = Path(__file__).resolve().parent
    version = subprocess.check_output([command[0], "--version"], text=True).strip()
    options = ["-Mdir", str(directory), "--no-skip-identical"]
    if mode == "processor":
        match = re.search(r"Verilator (\d+)\.(\d+)", version)
        if not match or tuple(map(int, match.groups())) < (5, 50):
            raise ValueError("processor hierarchy requires Verilator >= 5.050; "
                             "use VERILATOR_HIERARCHY=flat with older tools")
        options += ["--hierarchical", "--hierarchical-threads", "1",
                    str(config / "processor.vlt")]
        if tuple(map(int, match.groups())) == (5, 50):
            # V3Param gates numeric-parameter wrapper substitution on a nonempty
            # hierParamFile; V3HierBlock only supplies one for TYPE parameters.
            # This internal-option workaround is deliberately version-scoped.
            options += ["--hierarchical-params-file", str(config / "empty-params.v")]
    makefile = directory / ("V" + top + ".mk")
    # The hierarchical planner runs an inner make. An existing flat parent .mk
    # can be newer than reused child wrappers and suppress parent generation.
    # Remove only generated make fragments (never RTL/run logs/libraries).
    # Forcing all child recipes also refreshes C++/header timestamps when just
    # compiler options changed, so recursive make cannot retain stale objects.
    for generated_makefile in directory.rglob("V*.mk"):
        generated_makefile.unlink()
    # Make must retry after any failure, even if Verilator emitted its .mk first.
    try:
        argv = command + options
        print("Verilator model: " + mode + " (" + version + ")", flush=True)
        subprocess.run(argv, check=True)
        imports = check_reuse(directory, top) if mode == "processor" else []
        (directory / "hierarchy.json").write_text(json.dumps(
            dict(mode=mode, version=version, command=argv, parent_imports=imports),
            indent=2) + "\n")
    except BaseException:
        if makefile.exists():
            makefile.unlink()
        raise


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="action", required=True)
    s = sub.add_parser("stamp")
    s.add_argument("path", type=Path)
    s.add_argument("--mode", required=True)
    s.add_argument("--threads", required=True)
    s.add_argument("--identity", required=True)
    g = sub.add_parser("generate")
    g.add_argument("--mode", choices=("processor", "flat"), required=True)
    g.add_argument("--mdir", type=Path, required=True)
    g.add_argument("--top", required=True)
    g.add_argument("command", nargs=argparse.REMAINDER)
    args = parser.parse_args()
    try:
        if args.action == "stamp":
            stamp(args.path, args.mode, args.threads, args.identity)
        else:
            command = args.command[1:] if args.command[:1] == ["--"] else args.command
            if not command:
                raise ValueError("missing Verilator command")
            generate(args.mdir, args.top, args.mode, command)
    except (ValueError, subprocess.CalledProcessError, OSError) as error:
        print("BSG MAKE ERROR: " + str(error), file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
