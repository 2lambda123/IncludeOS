asm(".org 0x8000");
#define SOFT_RESET_MAGIC   0xFEE1DEAD

extern "C"
__attribute__((noreturn))
void hotswap(const char* base, int len, char* dest, void* start, void* reset_data)
{
  // replace kernel
  for (int i = 0; i < len; i++)
    dest[i] = base[i];
  // jump to _start
  asm volatile("jmp *%2" : : "a" (SOFT_RESET_MAGIC), "b" (reset_data), "c" (start) : "eax", "ebx", "ecx");
  asm volatile(
  ".global __hotswap_length;\n"
  "__hotswap_length:" );
  // we can never get here!
  __builtin_unreachable();
}
