#include <stdlib.h>

// AWS includes
#include "cl_utils.h"
#include "fpga_pci_sv.h"
// our software stack
#include "host.h"

#define debug 0

static const uint32_t BUF_SIZE = 4 * 1024 * 1024;
static const uint32_t ALIGNMENT = 64;
static const uint32_t POP_SIZE = 64;

static uint32_t get_tail (struct Host *host) {
	return host->buf[host->buf_size]; 
}


static void write_wr_addr_high (struct Host *host, uint32_t val) {
	OCL_BASE[WR_ADDR_HIGH] =  val; 
}

static void write_wr_addr_low (struct Host *host, uint32_t val) {
	OCL_BASE[WR_ADDR_HIGH] =  val; 
	fpga_pci_poke(PCI_BAR_HANDLE_INIT, WR_ADDR_LOW, val); 
}

static void write_wr_head (struct Host *host, uint32_t val) {
	fpga_pci_poke(PCI_BAR_HANDLE_INIT, WR_HEAD, val); 
}

static void write_wr_len (struct Host *host, uint32_t val) {
	fpga_pci_poke(PCI_BAR_HANDLE_INIT, WR_LEN, val); 
}

static void write_wr_buf_size (struct Host *host, uint32_t val) {
	fpga_pci_poke(PCI_BAR_HANDLE_INIT, WR_BUF_SIZE, val); 
}

static void start_write (struct Host *host) {
	fpga_pci_poke(PCI_BAR_HANDLE_INIT, CFG_REG, 0x10); 
	fpga_pci_poke(PCI_BAR_HANDLE_INIT, CNTL_REG, 0x1); 
}

static void stop (struct Host *host) {
	fpga_pci_poke(PCI_BAR_HANDLE_INIT, CNTL_REG, STOP); 
}

static void pop (struct Host *host, uint32_t pop_size) {
	uint32_t tail = host->get_tail();
	
	#ifdef debug
	printf("pop_data(): tail is at 0x%x\n", tail);
	#endif	

	int old_head = host->head; // for debugging messages 

	bool can_read;
	uint32_t num_dw;
	if (tail >= host->head)
		num_dw = tail - host->head;
	else
		num_dw = tail - host->head + host->buf_size;
	can_read = num_dw >= pop_size;
	if (!can_read) {
		printf("pop_data(): can't read %d bytes because (Head, Tail) = (%d, %d);\n only %d bytes available. Will try this again.\n",  pop_size, host->head, tail, num_dw); 
		return;
	}
	/* there is enough unread data; first, read data that lies before the end of system memory buffer */
	uint32_t before_dw = (host->buf_size - host->head >= pop_size) ? pop_size : host->buf_size - host->head; 
	for (int i = host->head; i < host->head + before_dw; i++) 
		host->buf_cpy[i] = host->buf[i];		
	host->head = (host->head + before_dw) % host->buf_size;
	/* read data that wraps over the end of system memory buffer */
	uint32_t after_dw = pop_size - before_dw;
	if (after_dw > 0) { /* if there is still data to read */
		/* read this data into a user buffer */
		for (int i = 0; i < after_dw; i++) 
			host->buf_cpy[i] = host->buf[i];
		host->head = (host->head + after_dw); /* update head */	
	}

   	if (fpga_pci_poke(PCI_BAR_HANDLE_INIT, WR_HEAD, host->head)) /* update head register on device */
		printf("Couldn't write to head register.\n"); 

	/* print what has been read */
	#ifdef debug
	printf("User program has popped data at [%u, %u)", old_head, old_head + before_dw);
	if (after_dw)
		printf(" and [0, %u):\n", after_dw);
	else
		printf(":\n");
	#endif 
} 

/* 
 * prints data as a sequence of unsigned chars.
 * */
void print(struct Host *host, uint32_t ofs, uint32_t size) {
	if (!host->buf_cpy)
		printf("Host::print: buf_cpy is null. can't print.\n");
	else if (!size)
		printf("Host::print: size is 0. can't print.\n");
	else if (ofs + size > host->buf_size)
		printf("Host::print: invalid range to print. can't print. \n");

	for (int i = ofs; i < size; i++) {
		printf("0x%02X", host->buf_cpy[i]);
		if ((i + 1) % 10 == 0)
			printf("\n");
		else
			printf(" ");
	}
	printf("\n");
}

void cosim_init_host (struct Host *host, uint32_t buf_size, uint32_t align) {
	host->buf_size = buf_size; /* global buffer */
	host->buf = (char *) aligned_alloc(align, buf_size + 64);
	memset(host->buf, 0, buf_size + 64);
	sv_map_host_memory(host->buf);
	host->buf_cpy = (char *) aligned_alloc(align, buf_size + 64); /* user copy of buffer */
	memset(host->buf_cpy, 0, buf_size + 64);
	
	host->head = 0;
	
	host->get_tail = get_tail;	
	host->write_wr_addr_high = write_wr_addr_high;
	host->write_wr_addr_low = write_wr_addr_low;
	host->write_wr_head = write_wr_head;
	host->write_wr_len = write_wr_len;
	host->write_wr_buf_size = write_wr_buf_size;
	host->start_write = start_write;
	host->stop = stop;
	host->pop = pop;
	host->print = print;
}

