#pragma once

#include <librf24/rf24adaptor.hpp>

namespace rf24boot { 
#include "../../../include/rf24boot.h"
	class RF24BootPartitionTable { 
	public: 
		RF24BootPartitionTable(librf24::LibRF24Adaptor *a, unsigned char remote_addr[5]);
		bool is_big_endian;
		std::string name; 
		void select(int i);
		void select(const char *name);
		void restore(const char *filepath);
		void save(const char *filepath);
		bool verify(const char *filepath);
		void run();
		/* Debugging leftovers */
		void writeOne(uint32_t addr, const char *data, size_t size);
	private:
		std::vector<rf24boot_partition_header> ptable;
		struct rf24boot_partition_header *currentPart;
		bool enablePbar = true;
		int numPacketsPerRun = 8;
		bool verifyFailed;
		FILE *fileFd;
		long  fileSize;
		unsigned int currentPartId;
		struct timeval	tv, tv0;

		void display_progressbar(int pad, int max, int value);
		librf24::LibRF24Adaptor *adaptor;
		void do_open(const char *filepath, const char *mode); 
		bool readSome(librf24::LibRF24IOTransfer &io, struct rf24boot_cmd *dat);
		uint32_t saveSome(librf24::LibRF24IOTransfer &io); 
		uint32_t verifySome(librf24::LibRF24IOTransfer &io, long int toverify); 
		void  timer_reset();
		float timer_elapsed();
		int numToQueue(uint32_t currentAddr);
	};
};
