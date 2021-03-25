template = \
"""asm volatile("fadd.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
//------------------------------------
"""

string = ""
for i in range(255):
  string += template.format()

print(string)
