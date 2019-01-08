#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>

#include "deploy.h"
#include "../device.h"

int main () {
	int val;

	printf("Running head/tail DMA tests.\n\n");

	/* Setup host */
	struct Host *host = (struct Host *) malloc(sizeof(struct Host));	 
	deploy_init_host(host, DMA_BUFFER_SIZE, axi4_align);	
	
	/* Setup device */
	ioctl(dev_fd, IOCTL_WR_ADDR_HIGH);
	ioctl(dev_fd, IOCTL_WR_ADDR_LOW);
	ioctl(dev_fd, IOCTL_WR_LEN, axi4_size);
	ioctl(dev_fd, IOCTL_WR_BUF_SIZE);

	/* read regs */
	ioctl(dev_fd, IOCTL_READ_WR_ADDR_HIGH, &val);
	printf("WR_ADDR_HIGH: %d\n", val);
	ioctl(dev_fd, IOCTL_READ_WR_ADDR_LOW, &val);
	printf("WR_ADDR_LOW: %d\n", val);
	ioctl(dev_fd, IOCTL_READ_WR_LEN, axi4_size, &val);
	printf("WR_LEN: %d\n", val);
	ioctl(dev_fd, IOCTL_READ_WR_BUF_SIZE, &val);
	printf("WR_BUF_SIZE: %d\n", val);

	/* start write */
	host->start_write(host);
	
	sleep(10);
	
	/* read */
	host->pop(host, 64);
	host->print(host, 0, 64);	

}
