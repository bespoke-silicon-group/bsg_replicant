"""Real DRAM timing/statistics checks plus all four Verilator link policies.

Run with CXX=clang++ BASEJUMP_STL_DIR=/path/to/basejump_stl python3
tests/test_dramsim3_modes.py. Requires the DRAMSim3 statistics-control update.
DRAMSIM3_TEST_LIBRARY_DIR optionally reuses already-built libraries.
"""
import configparser
import json
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import tempfile
import unittest

REPO = Path(__file__).resolve().parents[1]
FEATURE = REPO / 'libraries/features/dma/simulation'
PLATFORM = REPO / 'libraries/platforms/bigblade-verilator'
BASEJUMP = Path(os.environ.get('BASEJUMP_STL_DIR', REPO.parent / 'basejump_stl')).resolve()
MAKE = shutil.which('gmake') or shutil.which('make')
CXX = shlex.split(os.environ.get('CXX', 'c++'))
MODE_FLAGS = {
    'exec': ['-DDRAMSIM3_NO_STATISTICS'],
    'profile': [],
    'trace': ['-DBLOOD_GRAPH', '-DBLOOD_GRAPH_ENABLE_TRACE'],
}


def run(command, directory, **kwargs):
    result = subprocess.run(command, cwd=directory, text=True, capture_output=True, **kwargs)
    if result.returncode:
        raise AssertionError(result.stdout + result.stderr)
    return result


class DramModes(unittest.TestCase):
    def test_option_changes_rebuild_and_unchanged_build_is_noop(self):
        # Exercise the real dependency graph without repeatedly compiling the
        # whole library. The compiler recorder writes the requested output.
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            compiler = root / 'compiler'
            compiler.write_text('''#!/usr/bin/env python3
import pathlib,sys
pathlib.Path(sys.argv[sys.argv.index('-o')+1]).write_text(repr(sys.argv))
''')
            compiler.chmod(0o755)
            (root / 'Makefile').write_text(f'''
CL_DIR := {REPO}
LIBRARIES_PATH := {root}/libraries
BSG_PLATFORM_PATH := {root}/runtime
BASEJUMP_STL_DIR := {BASEJUMP}
BSG_PLATFORM := bigblade-verilator
BSG_MACHINE_MEM_CFG := hbm2_test
SHARED_LIBRARY_FLAGS := -shared
SHARED_LIBRARY_ID = -Wl,-soname,$(1)
include {FEATURE}/dramsim3.mk
''')
            target = root / 'libraries/features/dma/simulation/libdramsim3_exec.so'
            command = [MAKE, '--no-print-directory', 'CXX=' + str(compiler), str(target)]
            run(command, root)
            first = target.stat().st_mtime_ns
            self.assertIn('-O2', target.read_text())
            self.assertIn('-DDRAMSIM3_NO_STATISTICS', target.read_text())
            run(command, root)
            self.assertEqual(first, target.stat().st_mtime_ns)
            run(command + ['DRAMSIM3_OPT_FLAGS=-O0'], root)
            self.assertNotEqual(first, target.stat().st_mtime_ns)
            self.assertIn('-O0', target.read_text())
            self.assertNotIn('-O2', target.read_text())
            second = target.stat().st_mtime_ns
            run(command, root)
            self.assertNotEqual(second, target.stat().st_mtime_ns)
            self.assertIn('-O2', target.read_text())
            other = root / 'other-compiler'
            shutil.copy2(compiler, other)
            run([MAKE, 'CXX=' + str(other), str(target)], root)
            self.assertIn(str(other), target.read_text())
            for mode, flags in MODE_FLAGS.items():
                variant = target.with_name('libdramsim3_' + mode + '.so')
                run([MAKE, 'CXX=' + str(compiler), str(variant)], root)
                tokens = json.loads(variant.with_suffix('.so.config').read_text())['flags']
                self.assertIn('-O2', tokens)
                for flag in ['-DDRAMSIM3_NO_STATISTICS', '-DBLOOD_GRAPH', '-DBLOOD_GRAPH_ENABLE_TRACE']:
                    self.assertEqual(flag in shlex.split(tokens), flag in flags)
            old_source = root / 'old-source/src'
            old_source.mkdir(parents=True)
            (old_source / 'simple_stats.h').write_text('// old version\n')
            result = subprocess.run(['python3', str(FEATURE / 'dramsim3_build.py'),
                                     str(root / 'old.config'), '--compiler=c++',
                                     '--flags=-DDRAMSIM3_NO_STATISTICS', '--link-flags=-shared',
                                     '--source=' + str(old_source.parent)],
                                    text=True, capture_output=True)
            self.assertNotEqual(result.returncode, 0)
            self.assertIn('update the BaseJump DRAMSim3 submodule', result.stderr)

    def test_all_mode_links(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            for name in ['hardware.mk', 'libraries.mk']:
                (root / name).touch()
            (root / 'Makefile').write_text(f'''
BSG_PLATFORM_PATH := {root}
LIBRARIES_PATH := {root}
HARDWARE_PATH := {root}
BSG_MACHINExPLATFORM_PATH := {root}/machine
BSG_DESIGN_TOP := probe
DRAMSIM3_EXEC_LIBRARY := {root}/libdramsim3_exec.so
DRAMSIM3_PROFILE_LIBRARY := {root}/libdramsim3_profile.so
DRAMSIM3_TRACE_LIBRARY := {root}/libdramsim3_trace.so
include {PLATFORM}/link.mk
''')
            for mode in ['exec', 'profile', 'trace', 'debug']:
                directory = root / 'machine' / mode
                deps = [directory / x for x in ['bsg_manycore_simulator.o',
                                                'bsg_unified_dpi.o', 'Vprobe__ALL.a']]
                support = root / 'machine' / ('debug' if mode == 'debug' else 'exec')
                deps += [support / (x + '.o') for x in
                         ['verilated', 'verilated_dpi', 'verilated_threads']]
                if mode == 'debug': deps += [support / 'verilated_fst_c.o']
                deps += [root / x for x in ['libbsg_manycore_runtime.so',
                                            'libbsgmc_cuda_legacy_pod_repl.so',
                                            'libbsg_manycore_regression.so',
                                            'libdramsim3_exec.so', 'libdramsim3_profile.so',
                                            'libdramsim3_trace.so']]
                deps += [root / 'features' / feature / 'simulation' / library
                         for feature, library in [('dma', 'libdmamem.so'),
                                                  ('tracer', 'libtracer.so'),
                                                  ('pc_histogram', 'libpc_histogram.so')]]
                args = [MAKE, '--no-print-directory', 'CXX=echo']
                for dep in deps:
                    dep.parent.mkdir(parents=True, exist_ok=True)
                    dep.touch()
                    args += ['-o', str(dep)]
                result = run(args + [str(directory / 'simsc')], root)
                tokens = shlex.split(result.stdout)
                expected = 'trace' if mode == 'debug' else mode
                self.assertEqual([x for x in tokens if x.startswith('-ldramsim3')],
                                 ['-ldramsim3_' + expected] * 2)  # recipe and echo output
                self.assertIn(str(root / ('libdramsim3_' + expected + '.so')), tokens)

    def test_request_timing_and_statistics(self):
        source = BASEJUMP / 'imports/DRAMSim3'
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            if 'DRAMSIM3_TEST_LIBRARY_DIR' in os.environ:
                libraries = Path(os.environ['DRAMSIM3_TEST_LIBRARY_DIR']).resolve()
            else:
                libraries = root / 'libraries/features/dma/simulation'
                host = os.uname().sysname
                shared = '-dynamiclib -Wl,-undefined,dynamic_lookup' if host == 'Darwin' else '-shared'
                identity = '-Wl,-install_name,@rpath/$(1)' if host == 'Darwin' else '-Wl,-soname,$(1)'
                (root / 'Makefile').write_text(f'''
CL_DIR := {REPO}
LIBRARIES_PATH := {root}/libraries
BSG_PLATFORM_PATH := {root}/runtime
BASEJUMP_STL_DIR := {BASEJUMP}
BSG_PLATFORM := bigblade-verilator
BSG_MACHINE_MEM_CFG := hbm2_test
SHARED_LIBRARY_FLAGS := {shared}
SHARED_LIBRARY_ID = {identity}
include {FEATURE}/dramsim3.mk
''')
                run([MAKE, '-j3', 'CXX=' + shlex.join(CXX)] +
                    [str(libraries / ('libdramsim3_' + mode + '.so')) for mode in MODE_FLAGS], root)
            executables = {}
            for mode, flags in MODE_FLAGS.items():
                name = 'dramsim3_' + mode
                binary = root / mode
                run(CXX + ['-O2', '-std=c++11'] + flags +
                    ['-I' + str(source / 'src'), '-I' + str(source / 'ext/headers'),
                     str(REPO / 'tests/dramsim3_timing.cpp'), '-L' + str(libraries),
                     '-Wl,-rpath,' + str(libraries), '-l' + name, '-o', str(binary)], root)
                executables[mode] = binary
                wrapper_run = root / ('wrapper-' + mode)
                wrapper_run.mkdir()
                run([str(binary), 'wrapper', 'probe'], wrapper_run)
                if mode == 'exec': self.assertEqual(list(wrapper_run.iterdir()), [])
                if mode == 'profile': self.assertEqual(list(wrapper_run.glob('blood_graph*')), [])
            for self_refresh in [False, True]:
                config = configparser.ConfigParser(inline_comment_prefixes=(';', '#'))
                config.read(source / 'configs/HBM2_1Gb_x64_32ba.ini')
                config['system']['enable_self_refresh'] = str(int(self_refresh))
                config['system']['sref_threshold'] = '32'
                config['other']['epoch_period'] = '250'
                config['other']['output_level'] = '1'
                filename = root / ('config-' + str(self_refresh) + '.ini')
                with filename.open('w') as stream: config.write(stream)
                transcripts = {}
                for mode in MODE_FLAGS:
                    directory = root / (mode + '-' + str(self_refresh))
                    directory.mkdir()
                    result = run([str(executables[mode]), str(filename)], directory)
                    transcripts[mode] = result.stdout
                    self.assertIn('PASS 2048 2048', result.stdout)
                    outputs = list(directory.iterdir())
                    if mode == 'exec':
                        self.assertEqual(outputs, [], 'exec wrote DRAM statistics')
                    else:
                        if mode == 'profile':
                            self.assertEqual(list(directory.glob('blood_graph*')), [])
                        else:
                            self.assertTrue((directory / 'blood_graph_stat.log').stat().st_size > 0)
                            with (directory / 'blood_graph_ch0.log').open() as trace:
                                self.assertEqual(trace.readline().strip(), 'time,bank,state')
                                self.assertTrue(trace.readline().strip(), 'trace has no bank-state records')
                        stats = json.loads((directory / 'dramsim3.json').read_text())
                        self.assertTrue(stats)
                        self.assertTrue(any(v.get('num_cycles', 0) > 0 for v in stats.values()))
                        # Tags retain the startup interval even though the
                        # driver resets cumulative statistics later in the run.
                        tags = json.loads((directory / 'dramsim3.tag.json').read_text())
                        sref_cycles = sum(sum(v['sref_cycles'].values()) for v in tags)
                        self.assertEqual(sref_cycles > 0, self_refresh)
                        for counter in ['num_srefe_cmds', 'num_srefx_cmds']:
                            self.assertEqual(sum(v[counter] for v in tags) > 0, self_refresh)
                self.assertEqual(transcripts['exec'], transcripts['profile'])
                self.assertEqual(transcripts['exec'], transcripts['trace'])


if __name__ == '__main__':
    unittest.main()
