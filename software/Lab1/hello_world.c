#include <stdio.h>
#include "system.h"
#include "sys/alt_irq.h"
#include "altera_avalon_pio_regs.h"

static interrupt_count = 0;

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
	interrupt_count += 1;
	// clear our interrupt bit here
	IOWR(STIMULUS_IN_BASE, 3, 0x0);
}

int main()
{
	alt_irq_register(STIMULUS_IN_IRQ, (void *) 0, stimulus_in_ISR);

	IOWR(STIMULUS_IN_BASE, 2, 0x1);

	// max period 0xffff
	int period = 1000;
	int pulse_width = period / 2;
	IOWR(EGM_BASE, 2, period);
	IOWR(EGM_BASE, 3, pulse_width);

	IOWR(EGM_BASE, 0, 1);
	while(IORD(EGM_BASE, 1)) {
		printf("looping \n");
		background();
	}
	IOWR(EGM_BASE, 0, 0);

	printf("done program %d \n", interrupt_count);
	return 0;
}
