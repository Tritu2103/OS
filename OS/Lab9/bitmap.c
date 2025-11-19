/* ----------------------------------------------------------------------------------------
	Memory allocation simulation using bitmap.
	Course: MAT3501 - Principles of Operating Systems, MIM - HUS
------------------------------------------------------------------------------------------- */

#include <stdio.h>
#include <stdlib.h>

#define PAGESIZE	4096
#define MAXBYTES	8
#define MAXPAGES	8*MAXBYTES
#define MAXPROCESSES	256

#define B2b_t "%c%c%c%c%c%c%c%c"
#define B2b(byte)  \
  (byte & 0x80 ? '1' : '0'), \
  (byte & 0x40 ? '1' : '0'), \
  (byte & 0x20 ? '1' : '0'), \
  (byte & 0x10 ? '1' : '0'), \
  (byte & 0x08 ? '1' : '0'), \
  (byte & 0x04 ? '1' : '0'), \
  (byte & 0x02 ? '1' : '0'), \
  (byte & 0x01 ? '1' : '0') 
  
static unsigned char bitmap[MAXBYTES];

static struct {
	unsigned int state; 	// 0 not in memory, 1 in memory
	unsigned int startPage;
	unsigned int numPages;
} processList[MAXPROCESSES];

/* Bitmap functions --------------------------------------------------------------*/

int getBit(int bitPos)
{	
	int i, j;
	unsigned char val;
		
	i = bitPos / 8;	
	j = 7 - bitPos % 8;	
	val = bitmap[i] << (7 - j);
	val = val >> 7;
	return val;
}

void setBit(int bitPos, int value)
{	
	int i, j;
	i = bitPos / 8;
	j = 7 - bitPos % 8;
	
	if (value) bitmap[i] |= (1 << j);
	else bitmap[i] &= (0xFF ^ (1 << j));
}

void flipBits(int bitPos, int k)
{	
	int i;
	for (i = bitPos; i < bitPos + k; i++) {
		if (getBit(i)) setBit(i, 0);
		else setBit(i, 1);
	}
}

/* Search Algorithms --------------------------------------------------------------*/

int firstFitSearch(int k)
{
	int i, j;
	for (i = 0; i <= MAXPAGES - k; i++) {
		for (j = 0; j < k; j++) {
			if (getBit(i + j)) break;
		}
		if (j == k) return i;
	}
	return -1;
}

int nextFitSearch(int k)
{
	static int lastPos = 0;
	int i, j;

	for (i = lastPos; i <= MAXPAGES - k; i++) {
		for (j = 0; j < k; j++)
			if (getBit(i + j)) break;
		if (j == k) {
			lastPos = i + k;
			return i;
		}
	}

	for (i = 0; i <= lastPos - k; i++) {
		for (j = 0; j < k; j++)
			if (getBit(i + j)) break;
		if (j == k) {
			lastPos = i + k;
			return i;
		}
	}

	return -1;
}

int bestFitSearch(int k)
{
	int i, count, minCount = MAXPAGES + 1, pos = -1;

	for (i = 0; i <= MAXPAGES - 1;) {
		count = 0;
		while ((i + count < MAXPAGES) && (!getBit(i + count))) count++;
		if (count >= k && count < minCount) {
			minCount = count;
			pos = i;
		}
		i += (count > 0) ? count : 1;
	}

	return pos;
}

int worstFitSearch(int k)
{
	int i, count, maxCount = 0, pos = -1;

	for (i = 0; i <= MAXPAGES - 1;i++) {
		count = 0;
		while ((i + count < MAXPAGES) && (!getBit(i + count))) count++;
		if (count >= k && count > maxCount) {
			maxCount = count;
			pos = i;
		}
		i += (count > 0) ? count : 1;
	}

	return pos;
}

/* Memory management functions ---------------------------------------------------*/

int loadProcess(unsigned char name, int pages)
{
	int pos;

	if (processList[name].state) return -1;

	// Chọn 1 trong 4 thuật toán sau bằng cách thay đổi tên hàm:
	// pos = firstFitSearch(pages);
	// pos = nextFitSearch(pages);
	// pos = bestFitSearch(pages);
	pos = worstFitSearch(pages);  // đang dùng Worst Fit

	if (pos < 0) return -2;

	processList[name].startPage = pos;
	processList[name].numPages = pages;
	processList[name].state = 1;
	flipBits(pos, pages);

	return 0;
}

int unloadProcess(unsigned char name)
{
	if (processList[name].state == 0) return -1;

	flipBits(processList[name].startPage, processList[name].numPages);
	processList[name].state = 0;

	return 0;
}

/* Main --------------------------------------------------------------------------*/

int main(void)
{
	int i, pages, err;
	unsigned char name;
			
	printf("Instruction: input a letter for process name and number of pages, e.g a 10. Enter '!' to terminate\n");
	
	for (i = 0; i < MAXBYTES; i++) printf(" " B2b_t, B2b(bitmap[i]));
	printf("\n");
	
	while (1) {
		scanf(" %c", &name);  // space to skip whitespace
		if (name == '!') break;
		scanf("%d", &pages);

		if (((name >= 'a') && (name <= 'z')) || ((name >= 'A') && (name <= 'Z'))) {
			if (pages > 0) {
				err = loadProcess(name, pages);
				switch (err) {
					case -1: fprintf(stderr, "Process %c is already in memory\n", name); break;
					case -2: fprintf(stderr, "Memory is full or no suitable block found\n"); break;
				}
			} else {
				err = unloadProcess(name);
				if (err == -1)
					fprintf(stderr, "Process %c is not in memory\n", name);
			}
			for (i = 0; i < MAXBYTES; i++) printf(" " B2b_t, B2b(bitmap[i]));
			printf("\n");
		}
	}

	return 0;
}
