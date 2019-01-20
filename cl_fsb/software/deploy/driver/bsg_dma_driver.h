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
 
#define IOCTL_CLEAR_BUFFER _IO(F1_MAGIC, 15)

#define IOCTL_PKT_NUM _IOR(F1_MAGIC, 16, uint32_t)

// unit32_t for offset
#define FIFO_IST_WR _IOW(F1_MAGIC, 18, uint32_t)
#define FIFO_IER_WR _IOW(F1_MAGIC, 19, uint32_t)


#define FIFO_TDFV_RD0 _IOR(F1_MAGIC, 20, uint32_t)
#define FIFO_RDFO_RD0 _IOR(F1_MAGIC, 21, uint32_t) 

#define FIFO_RDFD_RD0 _IOR(F1_MAGIC, 22, uint32_t)
#define FIFO_RLR_RD0 _IOR(F1_MAGIC, 23, uint32_t)

#define FIFO_TDFD_WR0 _IOW(F1_MAGIC, 24, uint32_t) 
#define FIFO_TLR_WR0 _IOW(F1_MAGIC, 25, uint32_t)


#define FIFO_TDFV_RD1 _IOR(F1_MAGIC, 30, uint32_t)
#define FIFO_RDFO_RD1 _IOR(F1_MAGIC, 31, uint32_t) 

#define FIFO_RDFD_RD1 _IOR(F1_MAGIC, 32, uint32_t)
#define FIFO_RLR_RD1 _IOR(F1_MAGIC, 33, uint32_t)

#define FIFO_TDFD_WR1 _IOW(F1_MAGIC, 34, uint32_t) 
#define FIFO_TLR_WR1 _IOW(F1_MAGIC, 35, uint32_t)


#define FIFO_TDFV_RD2 _IOR(F1_MAGIC, 40, uint32_t)
#define FIFO_RDFO_RD2 _IOR(F1_MAGIC, 41, uint32_t) 

#define FIFO_RDFD_RD2 _IOR(F1_MAGIC, 42, uint32_t)
#define FIFO_RLR_RD2 _IOR(F1_MAGIC, 43, uint32_t)

#define FIFO_TDFD_WR2 _IOW(F1_MAGIC, 44, uint32_t) 
#define FIFO_TLR_WR2 _IOW(F1_MAGIC, 45, uint32_t)


#define FIFO_TDFV_RD3 _IOR(F1_MAGIC, 50, uint32_t)
#define FIFO_RDFO_RD3 _IOR(F1_MAGIC, 51, uint32_t) 

#define FIFO_RDFD_RD3 _IOR(F1_MAGIC, 52, uint32_t)
#define FIFO_RLR_RD3 _IOR(F1_MAGIC, 53, uint32_t)

#define FIFO_TDFD_WR3 _IOW(F1_MAGIC, 54, uint32_t) 
#define FIFO_TLR_WR3 _IOW(F1_MAGIC, 55, uint32_t)












#endif


