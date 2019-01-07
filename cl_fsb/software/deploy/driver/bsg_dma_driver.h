#ifndef DRIVER_H
#define DRIVER_H

#define DMA_BUFFER_SIZE 4096

#define F1_MAGIC 'F'

// write to registers
#define IOCTL_WR_ADDR_HIGH _IO(F1_MAGIC, 0)
#define IOCTL_WR_ADDR_LOW _IO(F1_MAGIC, 1)
#define IOCTL_WR_HEAD  _IOW(F1_MAGIC, 2, uint32_t)
#define IOCTL_WR_LEN _IOW(F1_MAGIC, 3, uint32_t)
#define IOCTL_WR_BUF_SIZE _IO(F1_MAGIC, 4)
#define IOCTL_CFG _IOW(F1_MAGIC, 5, uint32_t)
#define IOCTL_CNTL _IOW(F1_MAGIC, 6, uint32_t)
 
#define IOCTL_TAIL _IOR(F1_MAGIC, 7, uint32_t)

#define IOCTL_CLEAR_BUFFER _IO(F1_MAGIC, 8)

#endif


