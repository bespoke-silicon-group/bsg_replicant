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


def check_reuse(directory, top, boundary="bsg_manycore_proc_vanilla"):
    # A child archive alone is insufficient: 5.050/5.052 can emit an unused library
    # while leaving the parent flattened. Require parent imports AND call sites.
    dpi = directory / ("V" + top + "__Dpi.h")
    pattern = r"\b" + re.escape(boundary) + r"_[a-zA-Z0-9_]+_protectlib_combo_update\b"
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


def dpi_unit(directory, top, imports):
    """Coalesce C export wrappers using Verilator's generated per-export guards.

    Link this object before the archive. Otherwise the child profiler exports
    can be dropped (weak runtime references), or extracting their object also
    duplicates the parent's generic bsg_dpi_init/fini exports. Verilator itself
    recommends compiling __Dpi.cpp files in one translation unit. Scope-based
    dispatch and all model-specific callback implementations remain unchanged.
    Only include active children, never stale files from a previous build mode.
    """
    sources = [directory / ("V" + top + "__Dpi.cpp")]
    for symbol in imports:
        child = "V" + symbol[:-len("_protectlib_combo_update")]
        sources.append(directory / child / (child + "__Dpi.cpp"))
    # A small non-DPI design can legitimately have no export wrapper.
    sources = [path for path in sources if path.exists()]
    content = "// Generated; Verilator's guards coalesce shared DPI export names.\n"
    content += "".join('#include "' + path.relative_to(directory).as_posix() + '"\n'
                       for path in sources)
    (directory / "bsg_unified_dpi.cpp").write_text(content)
    return [str(path) for path in sources]


def generate(directory, top, mode, command, profile_ports=False):
    directory = directory.resolve()
    config = Path(__file__).resolve().parent
    version = subprocess.check_output([command[0], "--version"], text=True).strip()
    options = ["-Mdir", str(directory), "--no-skip-identical"]
    boundary = "bsg_manycore_hetero_socket" if profile_ports else "bsg_manycore_proc_vanilla"
    if mode == "processor":
        match = re.search(r"Verilator (\d+)\.(\d+)", version)
        if not match or tuple(map(int, match.groups())) < (5, 50):
            raise ValueError("processor hierarchy requires Verilator >= 5.050; "
                             "use VERILATOR_HIERARCHY=flat with older tools")
        options += ["--hierarchical", "--hierarchical-threads", "1",
                    str(config / ("profile-processor.vlt" if profile_ports else "processor.vlt"))]
        if profile_ports:
            sockets = [Path(arg) for arg in command if arg.endswith("/bsg_manycore_hetero_socket.sv")]
            if not sockets or not all("BSG_VERILATOR_PROFILE_PORTS" in path.read_text() for path in sockets):
                raise ValueError("optimized profiling requires bsg_manycore's simulation-only profiler ports; "
                                 "update bsg_manycore or use VERILATOR_PROFILE_HIERARCHY=flat")
            options += ["+define+BSG_VERILATOR_PROFILE_PORTS"]
        if tuple(map(int, match.groups())) in ((5, 50), (5, 52)):
            # V3Param gates numeric-parameter wrapper substitution on a nonempty
            # hierParamFile; V3HierBlock only supplies one for TYPE parameters.
            # Verified in both releases; deliberately exclude unknown versions.
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
        imports = check_reuse(directory, top, boundary) if mode == "processor" else []
        dpi_sources = dpi_unit(directory, top, imports)
        (directory / "hierarchy.json").write_text(json.dumps(
            dict(mode=mode, version=version, command=argv, parent_imports=imports,
                 profile_ports=profile_ports and mode == "processor", dpi_sources=dpi_sources),
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
    g.add_argument("--profile-ports", action="store_true")
    g.add_argument("command", nargs=argparse.REMAINDER)
    args = parser.parse_args()
    try:
        if args.action == "stamp":
            stamp(args.path, args.mode, args.threads, args.identity)
        else:
            command = args.command[1:] if args.command[:1] == ["--"] else args.command
            if not command:
                raise ValueError("missing Verilator command")
            generate(args.mdir, args.top, args.mode, command, args.profile_ports)
    except (ValueError, subprocess.CalledProcessError, OSError) as error:
        print("BSG MAKE ERROR: " + str(error), file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
