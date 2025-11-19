/* ----------------------------------------------------------------------------------------
	CLOCK page replacement algorithm.
	Course: MAT3501 - Principles of Operating Systems, MIM - HUS
	Summary: if page i is used, set its attribute R=1. Upon a page request, erase attribute R 
		and move on for a page with R=0. See physicalMem_t in ramEmu.h
------------------------------------------------------------------------------------------- */

#include <stdio.h>
#include <stdlib.h>

#ifndef RAMEMU_H
#include "ramEmu.h"
#endif

#ifndef CLOCK_H
#include "clock.h"
#endif

// initiate physical memory for CLOCK algo
void clock_memInit()
{
	physicalMem.current = 0;
	for (int i = 0; i < NUMFRAMES; i++)
		physicalMem.clock[i] = -1;
}

/* look up for a physical frame to be replaced by CLOCK algo */
int clock_lookUp()
{
	int p = physicalMem.current;

	while (physicalMem.clock[p] != 0) { // Nếu R=1 thì reset về 0
		physicalMem.clock[p] = 0;
		p = (p + 1) % NUMFRAMES; // Di chuyển vòng tròn
	}

	int found = p;
	physicalMem.current = (p + 1) % NUMFRAMES; // Move kim đồng hồ

	return found;
}

/* Update a frame's attributes after each access */
void clock_updateFrameAttributes(int frameNo, int state)
{
	if (state != 0)
		physicalMem.clock[frameNo] = 1; // Set R=1
	else
		physicalMem.clock[frameNo] = 0; // Clear R=0
}
