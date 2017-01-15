#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "grid.h"
#include <omp.h>
#include "pcg_basic.h"

extern pcg32_random_t rngs[];
extern int nthreads;

// Return pointer to Grid with given dimensions or return NULL.
Grid *createGrid(unsigned int width, unsigned int height) {

	Grid *grid = (Grid *)malloc(sizeof(Grid));
	grid->width = width;
	grid->height = height;
	grid->colors = (unsigned char **)malloc(height * sizeof(unsigned char *));
	if (grid->colors == NULL) {
		printf("createGrid error: could not allocate space.\n");
		return NULL;
	}
	int error = 0;
	for (unsigned int i = 0; i < height; ++i) {
		grid->colors[i] = (unsigned char *)calloc(width, sizeof(unsigned char));
		if (grid->colors[i] == NULL) {
			error = 1;
			break;
		}
	}
	// Dealocate all allocated rows if error.
	if (error) {
		destroyGrid(grid);
		return NULL;
	}
	return grid;
}

// Dealocate given grid.
void destroyGrid(Grid *grid) {

	if (grid == NULL) return;
	for (unsigned int i = 0; i < grid->height; ++i)
		if (grid->colors[i] != NULL)
			free(grid->colors[i]);
	free(grid->colors);
	free(grid);
	grid = NULL;
}

void processGrid(Grid * grid, Grid *tempGrid, int window, int myId, int size) 
	{
	printf("Sem proces %d od %d.\n", myId, size);

	
	// length of the window to look for neighbors
	int windowSize = (1 + 2 * window);

	// array of cells inside window
	int *neighbors = (int *)calloc( (windowSize * windowSize -1) * nthreads, sizeof(int));
	int x, y;
	int width = (int)grid->width;
	int height = (int)grid->height;

#pragma omp parallel
	{
		int ix = omp_get_thread_num();		
		int offset = (windowSize * windowSize - 1) * ix;

#pragma omp for private(y, x)
		for (y = 0; y < height; y++) {
			//int id = omp_get_thread_num();
			//printf("Thread id: %d on y:%d\n", id, y);
			for (x = 0; x < width; x++) {
				int index = 0;
				if (grid->colors[y][x] == 1) { // if cell is uncolorable(wall)
					tempGrid->colors[y][x] = grid->colors[y][x];
					continue;
				}
				// look at cells inside the window and add them to array
				for (int i = -window; i <= window; i++) {
					for (int j = -window; j <= window; j++) {
						if (y + i < 0 || y + i > height - 1) break; // check if window is out of bounds  - y axis
						//printf("x+j= %d\n", (x + j));
						if (x + j >= 0 && x + j < width) {			// check that window is not out of bounds - x axis
							if (grid->colors[y + i][x + j] != 0		// neighbor must not be blank - 0
								&& !(i == 0 && j == 0)				// don't add curent cell to neighbors
								&& grid->colors[y + i][x + j] != 1	// don't add walls to neighbors
								) {
								neighbors[index + offset] = grid->colors[y + i][x + j];
								index++;
							}
						}
					}
				}
				if (index > 0) {
					int r = pcg32_boundedrand_r(&rngs[ix], index);
					tempGrid->colors[y][x] = neighbors[r + offset];
				}
				else tempGrid->colors[y][x] = grid->colors[y][x];
			}
		}
	}
	unsigned char **tmp;
	tmp = grid->colors;
	grid->colors = tempGrid->colors;
	tempGrid->colors = tmp;
	
	free(neighbors);

	
}
