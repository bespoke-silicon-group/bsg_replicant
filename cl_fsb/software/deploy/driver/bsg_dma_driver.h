#ifndef DRIVER_H
#define DRIVER_H

#define DMA_BUFFER_SIZE 512000

#define F1_MAGIC 'F'

// write to registers
#define IOCTL_WR_ADDR_HIGH _IO(F1_MAGIC, 0)
#define IOCTL_WR_ADDR_LOW _IO(F1_MAGIC, 1)
#define IOCTL_WR_HEAD  _IOW(F1_MAGIC, 2, uint32_t)
#define IOCTL_WR_LEN _IOW(F1_MAGIC, 3, uint32_t)
#define IOCTL_WR_BUF_SIZE _IO(F1_MAGIC, 4)
#define IOCTL_CFG _IOW(F1_MAGIC, 5, uint32_t)
#define IOCTL_CNTL _IOW(F1_MAGIC, 6, uint32_t)

// read from registers  
#define IOCTL_TAIL _IOR(F1_MAGIC, 7, uint32_t)
#define IOCTL_READ_WR_ADDR_HIGH _IOR(F1_MAGIC, 8, uint32_t)
#define IOCTL_READ_WR_ADDR_LOW _IOR(F1_MAGIC, 9, uint32_t)
#define IOCTL_READ_WR_HEAD  _IOR(F1_MAGIC, 10, uint32_t)
#define IOCTL_READ_WR_LEN _IOR(F1_MAGIC, 11, uint32_t)
#define IOCTL_READ_WR_BUF_SIZE _IOR(F1_MAGIC, 12, uint32_t)
#define IOCTL_READ_CFG _IOR(F1_MAGIC, 13, uint32_t)
#define IOCTL_READ_CNTL _IOR(F1_MAGIC, 14, uint32_t)

#define IOCTL_GET_OCL _IOR(F1_MAGIC, 15, uint32_t)
 
#define IOCTL_CLEAR_BUFFER _IO(F1_MAGIC, 16)

#define IOCTL_PKT_NUM _IOR(F1_MAGIC, 16, uint32_t)
#endif


