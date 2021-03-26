template = \
"""asm volatile("flw     %0, {:>5}(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
//------------------------------------
"""

string = ""
for i in range(32):
  string += template.format(i*4, i*4)

print(string)
