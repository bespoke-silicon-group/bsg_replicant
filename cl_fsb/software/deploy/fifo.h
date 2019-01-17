const uint8_t NUM_FIFO = 10;
const uint8_t fifo[10][4] = {{0, 0, 0, 0} 
						, {0, 0, 0, 0}
						, {0, 0, 0, 0}
						, {0, 0, 0, 0}
						, {0, 0, 0, 0}
						, {0, 0, 0, 0}
						, {0, 0, 0, 0}	
						, {0, 0, 0, 0}
						, {0, 0, 0, 0}
						, {0, 0, 0, 0}};

static const uint8_t FIFO_READ = 0, FIFO_WRITE = 1, FIFO_OCCUPANCY = 2, FIFO_VACANCY = 3; 


