"""Bound observer controls and coalesced DPI dispatch, using real small models."""
import importlib.util
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

PLATFORM = Path(__file__).resolve().parents[1] / "libraries/platforms/bigblade-verilator"
SPEC = importlib.util.spec_from_file_location("hierarchy", PLATFORM / "hierarchy.py")
H = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(H)


@unittest.skipUnless(os.environ.get("TEST_VERILATOR"), "set TEST_VERILATOR for native boundary checks")
class ProfileBoundary(unittest.TestCase):
    def test_edges_controls_and_duplicate_dpi_exports(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            source = root / "bsg_manycore_hetero_socket.sv"
            source.write_text(r'''
module top(input logic clk, input logic [31:0] tag, ctr,
           input logic valid, trace_on, output logic [31:0] a, b);
  bsg_manycore_hetero_socket #(.N(32)) u0(clk, ctr, valid, tag, trace_on, a);
  bsg_manycore_hetero_socket #(.N(32)) u1(clk, ctr, valid, tag, trace_on, b);
  // Deliberately shares the child observer's export name and signature.
  export "DPI-C" function bsg_dpi_init;
  function void bsg_dpi_init(); endfunction
endmodule
// BSG_VERILATOR_PROFILE_PORTS: fixture for the required MC interface.
module bsg_manycore_hetero_socket #(parameter N=16)
  (input logic clk, input logic [31:0] profiler_global_ctr_i,
   input logic profiler_print_stat_v_i, input logic [31:0] profiler_print_stat_tag_i,
   input logic profiler_trace_en_i, output logic [N-1:0] result);
  leaf l(clk, result);
endmodule
module leaf(input logic clk, output logic [31:0] result);
  initial result=0;
  always @(posedge clk) result <= result+1;
endmodule
module observer(input logic clk, input logic [31:0] ctr, tag,
                input logic valid, trace_on);
  int unsigned pos_sum=0, neg_sum=0, traced=0;
  bit initialized=0;
  always @(posedge clk) if (valid) pos_sum <= pos_sum+ctr+tag;
  always @(negedge clk) begin
    if (valid) neg_sum <= neg_sum+ctr+tag;
    if (trace_on) traced <= traced+1;
  end
  export "DPI-C" function bsg_dpi_init;
  export "DPI-C" function probe_read;
  function void bsg_dpi_init(); initialized=1; endfunction
  function void probe_read(output int p, n, t);
    assert(initialized); p=pos_sum; n=neg_sum; t=traced;
  endfunction
endmodule
bind leaf observer obs(.clk(clk),
  .ctr(bsg_manycore_hetero_socket.profiler_global_ctr_i),
  .tag(bsg_manycore_hetero_socket.profiler_print_stat_tag_i),
  .valid(bsg_manycore_hetero_socket.profiler_print_stat_v_i),
  .trace_on(bsg_manycore_hetero_socket.profiler_trace_en_i));
''')
            driver = root / "main.cpp"
            driver.write_text(r'''
#include "Vtop.h"
#include "verilated.h"
#include "verilated_syms.h"
#include "verilated_dpi.h"
#include <cassert>
#include <cstring>
#include <cstdio>
extern "C" void bsg_dpi_init();
extern "C" void probe_read(int*,int*,int*);
double sc_time_stamp() { return 0; }
int main() {
  Vtop model;
  model.clk=0; model.valid=0; model.ctr=0; model.tag=0; model.trace_on=0; model.eval();
  int expected_p=0, expected_n=0, expected_t=0;
  for(int i=1; i<=20; ++i) {
    model.ctr=i; model.tag=100+i; model.valid=(i%3==0); model.trace_on=(i>=5 && i<9);
    // Controls change between edges and must not move counter sampling.
    model.eval(); model.clk=1; model.eval();
    if(model.valid) expected_p+=i+100+i;
    model.ctr=1000+i; model.tag=200+i; model.valid=(i%4==0);
    model.eval(); model.clk=0; model.eval();
    if(model.valid) expected_n+=1000+i+200+i;
    if(model.trace_on) ++expected_t;
  }
  assert(model.a==20 && model.b==20);
  int observers=0;
  for(const auto& entry : *Verilated::scopeNameMap()) {
    const char* suffix=std::strrchr(entry.first,'.');
    if(!suffix || std::strcmp(suffix,".obs")) continue;
    auto scope=svGetScopeFromName(entry.first); assert(scope); svSetScope(scope);
    bsg_dpi_init(); int p,n,t; probe_read(&p,&n,&t);
    assert(p==expected_p && n==expected_n && t==expected_t); ++observers;
  }
  assert(observers==2);
  std::printf("PASS observers=%d pos=%d neg=%d trace=%d\n",observers,expected_p,expected_n,expected_t);
  model.final();
}
''')
            compiler = os.environ.get("CXX", "c++")
            verilator = os.environ["TEST_VERILATOR"]
            vroot = Path(os.environ["VERILATOR_ROOT"])
            outputs = []
            for mode in ("flat", "processor"):
                generated = root / mode
                generated.mkdir()
                H.generate(generated, "top", mode,
                           [verilator, "--cc", "--assert", "--top-module", "top", str(source)], True)
                command = [shutil.which("gmake") or "make", "-j2", "-f", "Vtop.mk",
                           "CXX="+compiler,
                           "VM_FAST=$(filter-out %__Dpi,$(VM_CLASSES_FAST) $(VM_SUPPORT_FAST))", "Vtop__ALL.a"]
                subprocess.run(command, cwd=generated, check=True, capture_output=True, text=True)
                command = [compiler, "-std=c++14", "-I"+str(generated), "-I"+str(vroot/"include"),
                           "-I"+str(vroot/"include/vltstd"), str(driver), str(generated/"bsg_unified_dpi.cpp"),
                           str(generated/"Vtop__ALL.a"), str(vroot/"include/verilated.cpp"),
                           str(vroot/"include/verilated_dpi.cpp"), str(vroot/"include/verilated_threads.cpp"),
                           "-pthread", "-o", str(generated/"probe")]
                result = subprocess.run(command, capture_output=True, text=True)
                self.assertEqual(result.returncode, 0, result.stdout+result.stderr)
                result = subprocess.run([str(generated/"probe")], capture_output=True, text=True)
                self.assertEqual(result.returncode, 0, result.stdout+result.stderr)
                outputs.append(result.stdout)
            self.assertEqual(outputs[0], outputs[1])


if __name__ == "__main__":
    unittest.main()
