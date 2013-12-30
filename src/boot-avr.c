#include <arch/antares.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/boot.h>

#include <rf24boot.h>

static void (*nullVector)(void) __attribute__((__noreturn__));

#ifdef MCUCR
#define CR MCUCR
#endif

#ifdef GICR
#define CR GICR
#endif

/* AVR can have only one bootable partition, so we ignore the argument */
void rf24boot_boot_partition(struct rf24boot_partition *part)
{
	cli();
        boot_rww_enable();
        CR = (1 << IVCE); /* enable change of interrupt vectors */
        CR = (0 << IVSEL); /* move interrupts to application flash section */
/* We must go through a global function pointer variable instead of writing
 * ((void (*)(void))0)();
 * because the compiler optimizes a constant 0 to "rcall 0" which is not
 * handled correctly by the assembler.
 */
        nullVector();	
}

/* Extra care to move ISR vectors to boot partition */
ANTARES_INIT_LOW(avr_ivsel)
{
	CR = (1 << IVCE); /* enable change of interrupt vectors */
        CR = (1 << IVSEL); /* move interrupts to boot flash section */
}

