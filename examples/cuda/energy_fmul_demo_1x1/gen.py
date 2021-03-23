template = \
    """asm volatile("flw     %0, {:04d}(%1)"   : "=f"(A_tmp) : "r"(A_ptr) : "memory");
asm volatile("flw     %0, {:04d}(%1)"   : "=f"(B_tmp) : "r"(B_ptr) : "memory");
asm volatile("fmadd.s %0, %1, %2, %0" : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
//------------------------------------
"""

string = ""
for i in range(255):
  string += template.format(i*4, i*4)

print(string)
