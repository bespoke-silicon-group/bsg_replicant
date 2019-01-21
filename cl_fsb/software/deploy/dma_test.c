#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>

#include "deploy.h"
#include "../device.h"

int main () {
	int val;
	int write, read;

	printf("Running head/tail DMA tests.\n\n");

	/* Setup host */
	struct Host *host = (struct Host *) malloc(sizeof(struct Host));	 
	deploy_init_host(host, DMA_BUFFER_SIZE, axi4_align);	

	/* mmap the OCL BAR */
	char *ocl_base = deploy_mmap_ocl();
	if (ocl_base == 0) {
		printf("Error when mmap'ing OCL Bar.\n");
		return 0;
	}

	#ifdef DEBUG
	printf("OCL base address is %p\n", ocl_base);
	#endif 
	
	/* Setup dma device */
	ioctl(dev_fd, IOCTL_CLEAR_BUFFER);
	
	write_wr_head(host, 0);

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
	
	write_wr_len(host, axi4_size);
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

	sleep(1);

	/* read */
	pop_loop(host);

	host->stop(host);

	deploy_close();
	return 0;
}
