#ifndef DRIVER_H
#define DRIVER_H

#define F1_MAGIC 'F'

#define IOCTL_WR_ADDR_HIGH _IOW(F1_MAGIC, 0, uint32_t)
#define IOCTL_WR_ADDR_LOW _IOW(F1_MAGIC, 1, uint32_t)
#define IOCTL_WR_HEAD  _IOW(F1_MAGIC, 2, uint32_t)
#define IOCTL_WR_LEN _IOW(F1_MAGIC, 3, uint32_t)
#define IOCTL_WR_BUF_SIZE _IOW(F1_MAGIC, 4, uint32_t)
#define IOCTL_START_WRITE _IOW(F1_MAGIC, 5, uint32_t)
#define IOCTL_STOP _IOW(F1_MAGIC, 6, uint32_t)
#define IOCTL_POP _IOW(F1_MAGIC, 7, uint32_t)

#endif


