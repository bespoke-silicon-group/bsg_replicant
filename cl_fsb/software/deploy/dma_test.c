#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>

#include "deploy.h"
#include "../device.h"

void write_fifo_0(int byte_num);
void write_fifo_1(int byte_num);
void write_fifo_2(int byte_num);
void write_fifo_3(int byte_num);
void read_fifo_0();
void read_fifo_1();
void read_fifo_2();
void read_fifo_3();

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
	
<<<<<<< HEAD
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




void write_fifo_0(int byte_num) {
	int val;
	for(int i=0; i<byte_num/4; i++) {
		ioctl(dev_fd, FIFO_TDFD_WR0, i);
		printf("write dword @ offset %x = %d\n", CROSSBAR_M0, i);
	}
	ioctl(dev_fd, FIFO_TDFV_RD0, &val);
	printf("TDFV : %d => ", val);
	ioctl(dev_fd, FIFO_TLR_WR0, byte_num-byte_num%4);
	sleep(0);
	ioctl(dev_fd, FIFO_TDFV_RD0, &val);
	printf("TDFV : %d => ", val);
	ioctl(dev_fd, FIFO_RDFO_RD0, &val);
	printf("RDFO : %d \n", val);
	return;
}

void write_fifo_1(int byte_num) {
	int val;
	for(int i=0; i<byte_num/4; i++) {
		ioctl(dev_fd, FIFO_TDFD_WR1, i);
		printf("write dword @ offset %x = %d\n", CROSSBAR_M0_1, i);
	}
	ioctl(dev_fd, FIFO_TDFV_RD1, &val);
	printf("TDFV : %d => ", val);
	ioctl(dev_fd, FIFO_TLR_WR1, byte_num-byte_num%4);
	sleep(0);
	ioctl(dev_fd, FIFO_TDFV_RD1, &val);
	printf("TDFV : %d => ", val);
	ioctl(dev_fd, FIFO_RDFO_RD1, &val);
	printf("RDFO : %d \n", val);
	return;
}

void write_fifo_2(int byte_num) {
	int val;
	for(int i=0; i<byte_num/4; i++) {
		ioctl(dev_fd, FIFO_TDFD_WR2, i);
		printf("write dword @ offset %x = %d\n", CROSSBAR_M0_2, i);
	}
	ioctl(dev_fd, FIFO_TDFV_RD2, &val);
	printf("TDFV : %d => ", val);
	ioctl(dev_fd, FIFO_TLR_WR2, byte_num-byte_num%4);
	sleep(0);
	ioctl(dev_fd, FIFO_TDFV_RD2, &val);
	printf("TDFV : %d => ", val);
	ioctl(dev_fd, FIFO_RDFO_RD2, &val);
	printf("RDFO : %d \n", val);
	return;
}

void write_fifo_3(int byte_num) {
	int val;
	for(int i=0; i<byte_num/4; i++) {
		ioctl(dev_fd, FIFO_TDFD_WR3, i);
		printf("write dword @ offset %x = %d\n", CROSSBAR_M0_3, i);
	}
	ioctl(dev_fd, FIFO_TDFV_RD3, &val);
	printf("TDFV : %d => ", val);
	ioctl(dev_fd, FIFO_TLR_WR3, byte_num-byte_num%4);
	sleep(0);
	ioctl(dev_fd, FIFO_TDFV_RD3, &val);
	printf("TDFV : %d => ", val);
	ioctl(dev_fd, FIFO_RDFO_RD3, &val);
	printf("RDFO : %d \n", val);
	return;
}



void read_fifo_0() {
	printf("read FIFO 0 ===> \n");
	int val;
	int num;
	int byte_num;
	ioctl(dev_fd, FIFO_RDFO_RD0, &num);
	if (num==0) {
		printf("0 occupancy!\n");
	} else {
		while(num!=0) {
			ioctl(dev_fd, FIFO_RLR_RD0, &byte_num);
			printf("RLR: %d:", byte_num);
			for (int i=0; i<byte_num/4; i++) {
				ioctl(dev_fd, FIFO_RDFD_RD0, &val);
				printf("%d    ", val);
			}	
		ioctl(dev_fd, FIFO_RDFO_RD0, &num);
		printf("RDFO: %d\n", num);
		}
	}
	return;
}

void read_fifo_1() {
	printf("read FIFO 1 ===> \n");
	int val;
	int num;
	int byte_num;
	ioctl(dev_fd, FIFO_RDFO_RD1, &num);
	if (num==0) {
		printf("0 occupancy!\n");
	} else {
		while(num!=0) {
			ioctl(dev_fd, FIFO_RLR_RD1, &byte_num);
			printf("RLR: %d:", byte_num);
			for (int i=0; i<byte_num/4; i++) {
				ioctl(dev_fd, FIFO_RDFD_RD1, &val);
				printf("%d    ", val);
			}	
		ioctl(dev_fd, FIFO_RDFO_RD1, &num);
		printf("RDFO: %d\n", num);
		}
	}
	return;
}

void read_fifo_2() {
	printf("read FIFO 2 ===> \n");
	int val;
	int num;
	int byte_num;
	ioctl(dev_fd, FIFO_RDFO_RD2, &num);
	if (num==0) {
		printf("0 occupancy!\n");
	} else {
		while(num!=0) {
			ioctl(dev_fd, FIFO_RLR_RD2, &byte_num);
			printf("RLR: %d:", byte_num);
			for (int i=0; i<byte_num/4; i++) {
				ioctl(dev_fd, FIFO_RDFD_RD2, &val);
				printf("%d    ", val);
			}	
		ioctl(dev_fd, FIFO_RDFO_RD2, &num);
		printf("RDFO: %d\n", num);
		}
	}
	return;
}

void read_fifo_3() {
	printf("read FIFO 3 ===> \n");
	int val;
	int num;
	int byte_num;
	ioctl(dev_fd, FIFO_RDFO_RD3, &num);
	if (num==0) {
		printf("0 occupancy!\n");
	} else {
		while(num!=0) {
			ioctl(dev_fd, FIFO_RLR_RD3, &byte_num);
			printf("RLR: %d:", byte_num);
			for (int i=0; i<byte_num/4; i++) {
				ioctl(dev_fd, FIFO_RDFD_RD3, &val);
				printf("%d    ", val);
			}	
		ioctl(dev_fd, FIFO_RDFO_RD3, &num);
		printf("RDFO: %d\n", num);
		}
	}
	return;
}
