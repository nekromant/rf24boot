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
		void select(char *name);
		void restore(char *filepath);
		void save(char *filepath);
		void verify(char *filepath);
	private:
		bool enablePbar = true;
		int numPacketsPerRun = 16;
		void display_progressbar(int pad, int max, int value);
		librf24::LibRF24Adaptor *adaptor;
		void do_open(const char *filepath, const char *mode); 
		FILE *fileFd;
		long  fileSize;
		std::vector<rf24boot_partition_header> ptable;
		int currentPartId;
		struct rf24boot_partition_header *currentPart;
		bool readSome(librf24::LibRF24IOTransfer &io, struct rf24boot_cmd *dat);
		void saveSome(librf24::LibRF24IOTransfer &io); 
		void timer_init();
		float timer_since(float offset);
		void timer_update();
		struct timeval	tv, tv0;
		float	time_counter, last_frame_time_counter, dt, elapsed;

	};
};
