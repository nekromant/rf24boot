#include <arch/antares.h>
#include <arch/delay.h>
#include <rf24boot.h>
#include <string.h>

#define DEBUG_LEVEL CONFIG_DEBUGGING_VERBOSITY
#define COMPONENT "dummypart"


void do_null_write(struct rf24boot_partition* part, struct rf24boot_data *dat) 
{
	/* DO F*CKING NOTHING */
}

int do_null_read(struct rf24boot_partition* part, struct rf24boot_data *dat) 
{
	/* Return all 0xff's */
	memset(dat->data, 0x0, part->info.iosize);
	return part->info.iosize; 	
}
struct rf24boot_partition dummy_part = {
	.info = { 
		.name     = "dummy",
		.size     = CONFIG_DUMMYPART_SIZE,
		.iosize   = CONFIG_DUMMYPART_IOSIZE,
		.pad      = CONFIG_DUMMYPART_PAD,
	},
	.read = do_null_read,
	.write = do_null_write
};
BOOT_PARTITION(dummy_part);
