template = \
    """asm volatile("fadd.s  %0, %1, %2"     : "+f"(tmp{0}) : "f"(tmp{1}),  "f"(tmp{2}));
//------------------------------------
"""

numReg   = 10
regIndex = 0
string = ""
for i in range(255):
  rd  = regIndex
  rs1 = (regIndex + 1) % numReg
  rs2 = (regIndex + 2) % numReg
  string += template.format(rd, rs1, rs2)
  regIndex = (regIndex + 1) % numReg

print(string)
