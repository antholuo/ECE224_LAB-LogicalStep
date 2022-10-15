#include <stdio.h>
#include "system.h"
#include "sys/alt_irq.h"
#include "altera_avalon_pio_regs.h"

int background() {
	int j;
	int x = 0;
	int grainsize = 4;
	int g_taskProcessed = 0;
	for (j = 0; j < grainsize; j++) {
		g_taskProcessed++;
	}
	return x;
}

static void stimulus_in_ISR(void* context, alt_u32 id) {
	// Return to Response Out here

	// clear our interrupt bit here
	IOWR(STIMULUS_IN_BASE, 3, 0x0);
}

int main()
{
	alt_irq_register(BUTTON_PIO_IRQ, (void *) 0, stimulus_in_ISR);

	IOWR(STIMULUS_IN_BASE, 2, 0x1);

	IOWR(EGM_BASE, 2, 0xffff);
	IOWR(EGM_BASE, 3, 0xfff);

	IOWR(EGM_BASE, 0, 1);
	while(IORD(EGM_BASE, 1)) {
		printf("looping \n");
	}
	IOWR(EGM_BASE, 0, 0);

	printf("done program \n");
	return 0;
}
