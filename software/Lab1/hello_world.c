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
	IOWR(RESPONSE_OUT_BASE, 0, 1);
	IOWR(RESPONSE_OUT_BASE, 0, 0);
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

	// get our data

	int avg_latency = IORD(EGM_BASE, 4);
	int missed = IORD(EGM_BASE, 5);
	int multi = IORD(EGM_BASE, 6);

	IOWR(EGM_BASE, 0, 0);

	printf("done program %d with avg_latency: %d, missed: %d, multi: %d \n", interrupt_count, avg_latency, missed, multi);
	return 0;
}
