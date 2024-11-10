
#include <bsg_newlib_intf.h>

#define HOST_DEV_BASE_ADDR 0x00100000

#define GETCHAR_BASE_ADDR           ((char *)(HOST_DEV_BASE_ADDR | 0x00000))
#define PUTCHAR_BASE_ADDR           ((char *)(HOST_DEV_BASE_ADDR | 0x01000))
#define FINISH_BASE_ADDR            ((char *)(HOST_DEV_BASE_ADDR | 0x02000))
#define PUTCH_CORE_BASE_ADDR        ((char *)(HOST_DEV_BASE_ADDR | 0x03000))
#define BOOTROM_BASE_ADDR           ((char *)(HOST_DEV_BASE_ADDR | 0x10000))

void dramfs_exit(int exit_status) {
  *(FINISH_BASE_ADDR) = exit_status;
}

void dramfs_sendchar(char ch) {
  *(PUTCHAR_BASE_ADDR) = ch;
}

int dramfs_getchar(void) {
  return *(GETCHAR_BASE_ADDR);
}

