#include <librf24/librf24.hpp>
#include <librf24/rf24boot.hpp>
#include <sys/ioctl.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

using namespace rf24boot;
using namespace librf24;


RF24BootPartitionTable::RF24BootPartitionTable(LibRF24Adaptor *a, unsigned char addr[5]) { 
	struct rf24boot_cmd hs;
	adaptor = a;
	hs.op = RF_OP_HELLO;
	memcpy(hs.data, addr, 5);
	LibRF24IOTransfer rq(*a);
	rq.makeWriteStream(true);
	rq.fromBuffer((const char*) &hs, sizeof(hs));
	rq.execute();
	/* Whenever we succeeded or failed - wait for ptable */
	LibRF24IOTransfer rq2(*a);
	rq2.makeRead(1);
	rq2.setTimeout(1000);

	if (TRANSFER_COMPLETED != rq2.execute()) { 
		throw std::runtime_error("Ptable request failed");
	}

	LibRF24Packet *p = rq2.getPacket(0);
	char *dData = p->c_str();

	struct rf24boot_cmd *hcmd = (struct rf24boot_cmd *) dData;
	struct rf24boot_hello_resp *hello = (struct rf24boot_hello_resp *) hcmd->data;
	
	fprintf(stderr, "Target:     %s\n", hello->id);
	fprintf(stderr, "Endianness: %s\n", hello->is_big_endian ? "BIG" : "little");
	int numParts = hello->numparts;
	fprintf(stderr, "Partitions: %d\n", numParts);

	rq2.makeRead(numParts);
	rq2.execute();

	for (int i=0; i< ((int) numParts); i++) 
	{ 
		hcmd = (struct rf24boot_cmd *) rq2.getPacket(i)->c_str();
		struct rf24boot_partition_header *hdr = (struct rf24boot_partition_header *) hcmd->data;
		ptable.push_back(*hdr);
		fprintf(stderr, "%.1d. %12s  size %8d iosize %d pad %d\n",
			i, hdr->name, hdr->size, hdr->iosize, hdr->pad);
	}
};


void RF24BootPartitionTable::select(int i) { 
	currentPartId = i; 
	currentPart = &ptable.at(i);
}

void RF24BootPartitionTable::select(const char *name)
{
	for (unsigned int i=0; i< ptable.size(); i++) {
		if (strcmp(ptable.at(i).name, name)==0) { 
			select(i);
			return;
		}
	}
	throw std::runtime_error("No such partition");
}


void RF24BootPartitionTable::do_open(const char *filepath, const char *mode) 
{
	fileFd = fopen(filepath, mode);
	if (!fileFd) {
		perror(filepath);
		throw std::runtime_error("Can't open input file");
	}

	fseek(fileFd, 0L, SEEK_END);
	fileSize = ftell(fileFd);
	rewind(fileFd);	
}




void RF24BootPartitionTable::display_progressbar(int pad, int max, int value)
{
	float percent = 100.0 - (float) value * 100.0 / (float) max;
	int cols;
	struct winsize w;
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
	cols = w.ws_col;

	int txt = printf("%.02f %% done [", percent);
	int max_bars = cols - txt - 7 - pad;
	int bars = max_bars - (int)(((float) value * max_bars) / (float) max);

	if (max_bars > 0) {
		int i;	
		for (i=0; i<bars; i++)
			printf("#");
		for (i=bars; i<max_bars; i++)
			printf(" ");
		printf("]\r");
	}
	fflush(stdout);
}

void RF24BootPartitionTable::timer_init()
{
	gettimeofday(&tv0, NULL);
}

float RF24BootPartitionTable::timer_since(float offset)
{
	return elapsed-offset;
}

void RF24BootPartitionTable::timer_update()
{
	last_frame_time_counter = time_counter;
	gettimeofday(&tv, NULL);
	time_counter = (float)(tv.tv_sec-tv0.tv_sec) + 0.000001*((float)(tv.tv_usec-tv0.tv_usec));
	dt = time_counter - last_frame_time_counter;
	elapsed+=dt;
}


void RF24BootPartitionTable::restore(const char *filepath) 
{ 
	do_open(filepath, "r");
	fprintf(stderr, "Writing partition %s: %lu/%u bytes \n", currentPart->name, fileSize, currentPart->size);
	struct rf24boot_cmd cmd;
	cmd.op = RF_OP_WRITE;
	struct rf24boot_data *dat = (struct rf24boot_data *) cmd.data;
	dat->part = (uint8_t) currentPartId;
	dat->addr = 0;
	timer_init();
	LibRF24IOTransfer io(*adaptor);
	/* Bulk */
	io.makeWriteBulk(false);
	io.setTimeout(15000);

	io.execute();
	while (readSome(io, &cmd)) {
		timer_update();
		int pad = printf("%u/%lu bytes | %.02f s | ", (dat->addr), fileSize, timer_since(0)); 
		display_progressbar(pad, fileSize, fileSize - (dat->addr));
		io.execute();
	};

	io.clear();
	io.makeWriteBulk(true); /* Sync it! */
	memset(dat->data, 0xff, currentPart->iosize);

	bool dontneedpad = (!currentPart->pad || 0 == (dat->addr % currentPart->pad));
	int padded = 0 ;
	if (!dontneedpad)
		do {
			io.appendFromBuffer((char *) &cmd, sizeof(struct rf24boot_data));
			dat->addr += currentPart->iosize;
			padded +=currentPart->iosize;
		} while (dat->addr % currentPart->pad);
	io.execute();
	int pad = printf("%u/%lu bytes | %.02f s | ", (dat->addr), fileSize, timer_since(0)); 
	display_progressbar(pad, fileSize, fileSize);	
	printf("\nPadded %d bytes\n", padded);
}

bool RF24BootPartitionTable::readSome(LibRF24IOTransfer &io, struct rf24boot_cmd *cmd)
{
	struct rf24boot_data *dat = (struct rf24boot_data *) cmd->data;
	io.clear();

	if (dat->addr >= currentPart->size)
		return false;
	for (int i=0; i< numPacketsPerRun; i++) {
		int ret = fread(dat->data, 1, currentPart->iosize, fileFd);
		if (ret==0 && i==0)
			return false;
		io.appendFromBuffer((char *) cmd, sizeof(struct rf24boot_data));
		dat->addr += currentPart->iosize;
		if (dat->addr >= currentPart->size)
			return true;
	}
	return true;
}

uint32_t RF24BootPartitionTable::saveSome(LibRF24IOTransfer &io) 
{
	/* TODO: sanity checking */
	int i; 
	uint32_t retaddr;
	for (i=0; i<io.getPacketCount(); i++) { 
		LibRF24Packet *pck;
		struct rf24boot_cmd *cmd; 
		pck = io.getPacket(i);
		cmd = (struct rf24boot_cmd *) pck->c_str();
		struct rf24boot_data *dat = (struct rf24boot_data *) cmd->data;
		retaddr = dat->addr;
		fwrite(dat->data, currentPart->iosize, 1, fileFd);
		if (retaddr == currentPart->size - currentPart->iosize) { 
			return currentPart->size;
		}

	}
	io.clear();
	io.makeRead(numPacketsPerRun); 
	return retaddr;
}
	
void RF24BootPartitionTable::save(const char *filepath) 
{ 
	do_open(filepath, "w+");		
	fprintf(stderr, "Reading partition %s: %u bytes \n", currentPart->name, currentPart->size);	

	struct rf24boot_cmd cmd;
	cmd.op = RF_OP_READ;
	struct rf24boot_data *dat = (struct rf24boot_data *) cmd.data;
	dat->part = (uint8_t) currentPartId;
	dat->addr = 0;
	timer_init();
	LibRF24IOTransfer io(*adaptor);
	io.makeWriteBulk(true); 
	io.appendFromBuffer((char *) &cmd, sizeof(struct rf24boot_data));
	io.execute();
	io.makeRead(numPacketsPerRun); 
	io.setTimeout(5000);
	do { 
		io.execute();
		timer_update();
		uint32_t addr = saveSome(io);
		int pad = printf("%u/%u bytes | %.02f s | ", addr, currentPart->size, timer_since(0)); 
		display_progressbar(pad, currentPart->size, currentPart->size-addr);
		if (addr == currentPart->size)
			break;
	} while(1);
	std::cout << std::endl;
	fclose(fileFd);
}


void RF24BootPartitionTable::run()
{
	struct rf24boot_cmd cmd;
	cmd.op = RF_OP_BOOT;
	struct rf24boot_data *dat = (struct rf24boot_data *) cmd.data;
	dat->part = (uint8_t) currentPartId;
	LibRF24IOTransfer io(*adaptor);
	io.makeWriteBulk(true); 
	io.appendFromBuffer((char *) &cmd, sizeof(struct rf24boot_data));
	io.execute();	
}

void RF24BootPartitionTable::writeOne(uint32_t addr, const char *data, size_t size)
{
	struct rf24boot_cmd cmd;
	cmd.op = RF_OP_WRITE;
	struct rf24boot_data *dat = (struct rf24boot_data *) cmd.data;
	dat->part = (uint8_t) currentPartId;
	LibRF24IOTransfer io(*adaptor);
	io.makeWriteBulk(true); 
	io.appendFromBuffer((char *) &cmd, sizeof(struct rf24boot_data));
	io.execute();	
}

void RF24BootPartitionTable::verify(const char *filepath)
{
	do_open(filepath, "r");		
	/* 
	uint32_t toverify = std::min(currentPart->size, fileSize);

	fprintf(stderr, "Verifying partition %s against file %s, %lu bytes \n", 
		currentPart->name, filepath, currentPart->size);	

	struct rf24boot_cmd cmd;
	cmd.op = RF_OP_READ;
	struct rf24boot_data *dat = (struct rf24boot_data *) cmd.data;
	dat->part = (uint8_t) currentPartId;
	dat->addr = 0;

	timer_init();
	LibRF24IOTransfer io(*adaptor);
	io.makeWriteBulk(true); 
	io.appendFromBuffer((char *) &cmd, sizeof(struct rf24boot_data));
	io.execute();
	io.makeRead(numPacketsPerRun); 
	io.setTimeout(5000);
	do { 
		io.execute();
		uint32_t addr = saveSome(io);
		int pad = printf("%u/%lu bytes | %.02f s | ", addr, currentPart->size, timer_since(0)); 
		display_progressbar(pad, currentPart->size, currentPart->size-addr);
		if (addr == currentPart->size)
			break;
	} while(1);
	std::cout << std::endl;
	fclose(fileFd);	
	*/
}
