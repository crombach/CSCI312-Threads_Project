/* Cullen Rombach (cmromb)
 * 11/22/2015
 *
 * Project "PP"
 * Part 2 - openmp.c
 *
 * This program generates a Monte Carlo estimation of pi using OpenMP threading.
 */

#include <omp.h>
#include <stdio.h>
#include <stdlib.h>

// Set the number of threads to 4, the number of cores on the machines
// in the CS lab.
#define NUM_THREADS 4

void main() {
	// Declare private variables.
	int i, seed, hits, samples;
	double x, y;

	// Initialize some of them.
	hits = 0;

	// Run the required number of threads.
	#pragma omp parallel num_threads(NUM_THREADS) default(none) \
		private(i,seed,x,y) reduction(+:samples,hits)
	{
		// Get between 25mil and 26mil samples.
		seed = omp_get_wtime() * 623 + omp_get_thread_num() * 315;
		samples = rand_r(&seed) % 1000000 + 25000000;

		// Generate and check the required number of sample points.
		for(i = 0; i < samples; i++) {
			// Generate random coordinates from -0.5 to 0.5, inclusive.
			x = (double)(rand_r(&seed))/(RAND_MAX) - 0.5;
			y = (double)(rand_r(&seed))/(RAND_MAX) - 0.5;
			// Check to see if the point is inside the circle.
			if ((x * x + y * y) <= 0.25) {
				hits++;
			}
		}
	}

	// Print out the number of samples.
	printf("\nTotal sample points: %i\n", samples);
	// Print out the number of hits.
	printf("Sample points inside circle: %i\n", hits);
	// Print out the estimation of pi.
	double pi =	4.0 * (double)hits / (double)samples;
	printf("Estimation of pi: %f\n\n", pi);

	return;
}
