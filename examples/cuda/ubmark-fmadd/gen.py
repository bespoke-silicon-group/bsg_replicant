template = \
    """asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp{0}) : "f"(tmp{1}),  "f"(tmp{2}), "f"(tmp{3}));
//------------------------------------
"""

numReg   = 10
regIndex = 0
string = ""
for i in range(255):
  rd  = regIndex
  rs1 = (regIndex + 1) % numReg
  rs2 = (regIndex + 2) % numReg
  rs3 = (regIndex + 3) % numReg
  string += template.format(rd, rs1, rs2, rs3)
  regIndex = (regIndex + 1) % numReg

print(string)
