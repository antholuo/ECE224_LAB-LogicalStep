#include <stdio.h>
#include <math.h>
#include "system.h"
#include "sys/alt_irq.h"
#include "altera_avalon_pio_regs.h"

/**
 * Some notes for tight polling
 * 1. tight poll stimulus
 * 2. get stimulus, run background immediately.
 * 3. the num_backgruond that you can run should be 3/4 or 7/8 (not -1)
 * 4. while your stimulus is still 1 (after a post), run background (otherwise you may repeat at high freq)
 * 5. 1-3 doesn't need to be in a while loop
 * */
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
	IOWR(RESPONSE_OUT_BASE, 0, 1);
	IOWR(RESPONSE_OUT_BASE, 0, 0);
	// clear our interrupt bit here
	IOWR(STIMULUS_IN_BASE, 3, 0x0);
}

int main()
{
	/////////////////
	// Defines
	/////////////////
	int period = 0;
	int pulse_width = 0;
	int avg_latency, missed, multi;
	int character_timing = -1;
	int i = 0;
	int switch_state = -1;
	int not_started = 1;
	static volatile int toggled = 0;
	static int background_count = 0;

	////////////////
	// Program Begin Sequence
	////////////////
	switch_state = (0x01 & IORD(SWITCH_PIO_BASE, 0));
	if (switch_state == 0)
	{
		printf("Interrupt method selected\n");
	}
	else
	{
		printf("Tight Polling method selected\n");
	}
	printf("Please, press PB0 to continue, or change switch position and press PB1\n");
	while (not_started)
	{
		if (0x1 & ~(IORD(BUTTON_PIO_BASE, 0)))
		{
			not_started = 0;
		}
		else if (0x2 & ~(IORD(BUTTON_PIO_BASE, 0)))
		{
			if (toggled)
			{
			}
			else
			{
				toggled = 1;
				switch_state = (0x01 & IORD(SWITCH_PIO_BASE, 0));
				if (switch_state == 0)
				{
					printf("Interrupt method selected\n");
				}
				else
				{
					printf("Tight Polling method selected\n");
				}
				printf("Please, press PB0 to continue, or change switch position and press PB1\n");
			}
		}
		if (0x2 & (IORD(BUTTON_PIO_BASE, 0)))
		{
			toggled = 0;
		}
	}

	//////////////
	// Code Begin
	//////////////
	int interrupt = !(switch_state & 1);
	int polling = (switch_state & 1);

	IOWR(RESPONSE_OUT_BASE, 4, 1);
	IOWR(RESPONSE_OUT_BASE, 5, 1);

	if (interrupt)
	{
		alt_irq_register(STIMULUS_IN_IRQ, (void *)0, stimulus_in_ISR);
		IOWR(STIMULUS_IN_BASE, 2, 0x1);
		for (period = 2; period <= 5000; period += 2)
		{
			// configure variables
			pulse_width = period / 2;
			IOWR(EGM_BASE, 2, period);
			IOWR(EGM_BASE, 3, pulse_width);
			background_count = 0;

			// start running
			IOWR(EGM_BASE, 0, 1);
			while (IORD(EGM_BASE, 1))
			{
				background();
				background_count += 1;
			}

			// get our EGM data
			avg_latency = IORD(EGM_BASE, 4);
			missed = IORD(EGM_BASE, 5);
			multi = IORD(EGM_BASE, 6);

			IOWR(EGM_BASE, 0, 0); // clear EGM for next run

			printf("%d, %d, %d, %d, %d, %d,\n", period, pulse_width, background_count, avg_latency, missed, multi);
		}
	}

	if (polling)
	{
		for (period = 2; period <= 5000; period += 2)
		{
			// configure variables
			pulse_width = period / 2;
			IOWR(EGM_BASE, 2, period);
			IOWR(EGM_BASE, 3, pulse_width);
			background_count = 0;
			character_timing = -1;
			int run = 0;
			IOWR(EGM_BASE, 0, 1);
			while (IORD(STIMULUS_IN_BASE, 0) == 0)
			{
			}
			IOWR(RESPONSE_OUT_BASE, 0, 1);
			IOWR(RESPONSE_OUT_BASE, 0, 0);
			while (IORD(STIMULUS_IN_BASE, 0))
			{
			};

			do
			{
				background();
				background_count += 1;
			} while (IORD(STIMULUS_IN_BASE, 0) == 0);
			IOWR(RESPONSE_OUT_BASE, 0, 1);
			IOWR(RESPONSE_OUT_BASE, 0, 0);
			character_timing = floor(background_count * 3 / 4);
			while (IORD(EGM_BASE, 1))
			{
				if (run == 0)
				{
					for (i = 0; i < character_timing; i++)
					{
						background();
						background_count += 1;
					}
					run = 1;
				}
				if (IORD(STIMULUS_IN_BASE, 0))
				{
					run = 0;
					IOWR(RESPONSE_OUT_BASE, 0, 1);
					IOWR(RESPONSE_OUT_BASE, 0, 0);
					while (IORD(STIMULUS_IN_BASE, 0))
					{
//						background();
//						background_count += 1;
					}
				}
			}

			avg_latency = IORD(EGM_BASE, 4);
			missed = IORD(EGM_BASE, 5);
			multi = IORD(EGM_BASE, 6);

			IOWR(EGM_BASE, 0, 0);

			printf("%d, %d, %d, %d, %d, %d,\n", period, pulse_width, background_count, avg_latency, missed, multi);
		}
	}

	printf("Program complete!\n");

	return 0;
}
