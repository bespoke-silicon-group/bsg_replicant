template = \
"""asm volatile("flw     %0, {:>5}(%1)"   : "+f"(A_tmp) : "r"(A_ptr) : "memory");
asm volatile("flw     %0, {:>5}(%1)"   : "+f"(B_tmp) : "r"(B_ptr) : "memory");
//------------------------------------
"""

string = ""
for i in range(255):
  string += template.format(i*4, i*4)

print(string)
