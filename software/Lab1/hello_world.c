#include <stdio.h>
#include "system.h"
#include "sys/alt_irq.h"
#include "altera_avalon_pio_regs.h"

static background_count = 0;

//#define interrupt
#define polling

int background()
{
	int j;
	int x = 0;
	int grainsize = 4;
	int g_taskProcessed = 0;
	for (j = 0; j < grainsize; j++)
	{
		g_taskProcessed++;
	}
	return x;
}

static void stimulus_in_ISR(void *context, alt_u32 id)
{
	// Return to Response Out here
	background_count += 1;
	IOWR(RESPONSE_OUT_BASE, 0, 1);
	IOWR(RESPONSE_OUT_BASE, 0, 0);
	// clear our interrupt bit here
	IOWR(STIMULUS_IN_BASE, 3, 0x0);
}

int main()
{
#ifdef interrupt
	alt_irq_register(STIMULUS_IN_IRQ, (void *)0, stimulus_in_ISR);

	IOWR(STIMULUS_IN_BASE, 2, 0x1);
#endif
	IOWR(RESPONSE_OUT_BASE, 4, 1);
	IOWR(RESPONSE_OUT_BASE, 5, 1);
	int avg_latency, missed, multi;

	// max period 0xffff
	int period = 1000;
	int pulse_width = period / 2;
	IOWR(EGM_BASE, 2, period);
	IOWR(EGM_BASE, 3, pulse_width);

#ifdef interrupt
	IOWR(EGM_BASE, 0, 1);
	while (IORD(EGM_BASE, 1))
	{
		background();
	}
#endif
#ifdef polling
	int character_timing = -1;
	int i;
	IOWR(EGM_BASE, 0, 1);
	while (IORD(STIMULUS_IN_BASE, 0) == 0)
	{
	}
	IOWR(RESPONSE_OUT_BASE, 0, 1);
	IOWR(RESPONSE_OUT_BASE, 0, 0);
	while (IORD(EGM_BASE, 1))
	{
		if (character_timing < 0)
		{
			while (IORD(STIMULUS_IN_BASE, 0) == 0)
			{
				background();
				background_count += 1;
			}
			character_timing = background_count - 1;
			IOWR(STIMULUS_IN_BASE, 0, 0x00);
			IOWR(RESPONSE_OUT_BASE, 0, 1);
			IOWR(RESPONSE_OUT_BASE, 0, 0);
		}
		for (i = 0; i < character_timing; i++)
		{
			background();
			background_count += 1;
		}
		if (IORD(STIMULUS_IN_BASE, 0) == 1)
		{
			IOWR(STIMULUS_IN_BASE, 0, 0x00);
			IOWR(RESPONSE_OUT_BASE, 0, 1);
			IOWR(RESPONSE_OUT_BASE, 0, 0);
		}
	}
	printf("character count %d \n", character_timing);
#endif

	// get our data

	/** NOTES
	 *
	 * Need to loop through from 2 <= Period <= 5000
	 *
	 * Remember that our data needs to be:
	 * Output results for each test run:
	 * (PERIOD, PULSE WIDTH, BG TASK CALLS RUN, AVG LATENCY, MISSED and MULTI PULSES (separated by commas))
	 */
	avg_latency = IORD(EGM_BASE, 4);
	missed = IORD(EGM_BASE, 5);
	multi = IORD(EGM_BASE, 6);

	IOWR(EGM_BASE, 0, 0);

	printf("done program %d with avg_latency: %d, missed: %d, multi: %d \n", background_count, avg_latency, missed, multi);
	return 0;
}
