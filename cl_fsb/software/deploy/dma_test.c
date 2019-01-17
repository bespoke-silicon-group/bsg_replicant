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
	ioctl(dev_fd, IOCTL_CLEAR_BUFFER);
	ioctl(dev_fd, IOCTL_WR_HEAD, 0);

	ioctl(dev_fd, IOCTL_WR_ADDR_HIGH);
	#ifdef DEBUG
	ioctl(dev_fd, IOCTL_READ_WR_ADDR_HIGH, &val);
	printf("WR_ADDR_HIGH: %d\n", val);
	#endif

	ioctl(dev_fd, IOCTL_WR_ADDR_LOW);
	#ifdef DEBUG
	ioctl(dev_fd, IOCTL_READ_WR_ADDR_LOW, &val);
	printf("WR_ADDR_LOW: %d\n", val);
	#endif
	
	ioctl(dev_fd, IOCTL_WR_LEN, axi4_size);
	#ifdef DEBUG
	ioctl(dev_fd, IOCTL_READ_WR_LEN, &val);
	printf("WR_LEN: %d\n", val);
	#endif

	ioctl(dev_fd, IOCTL_WR_BUF_SIZE);
	#ifdef DEBUG
	ioctl(dev_fd, IOCTL_READ_WR_BUF_SIZE, &val);
	printf("WR_BUF_SIZE: %d\n", val);
	#endif

	/* start write */
	host->start_write(host);
	
//	sleep(1);
	
	/* read */
	pop_loop(host);
	
	return 0;
}
