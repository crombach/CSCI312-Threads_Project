/* Cullen Rombach (cmromb)
 * 11/22/2015
 *
 * Project "PP"
 * Part 1 - pthread.c
 *
 * This program solves the producer-consumer problem using pthreads.
 */

#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

// The total number of tasks: this comes from user input.
int N = 10;
// The number of tasks that have been completed by the consumers.
int tasksCompleted = 0;
// The number of tasks completed by each consumer.
int tasksCompletedConsumer1 = 0, tasksCompletedConsumer2 = 0;
// Keep track of whether or not each consumer thread has terminated.
int consumerOneDone = 0, consumerTwoDone = 0;

/* Define the task_queue struct.
 * This queue and its corresponding methods are
 * based on an example from a New Mexico State University
 * tutorial. */
typedef struct {
	int data[2];
	int isFull, isEmpty;
	long head, tail;
	pthread_mutex_t *lock;
	pthread_cond_t *notFull, *notEmpty;
} task_queue;

// Define a struct that holds the input to the consumer function.
typedef struct {
	int thread_num;
	task_queue * tq;
} consumer_input;

// Initialize a consumer input.
consumer_input *initConsumerInput() {
	consumer_input *tq;

	// Allocate the data for the queue.
	tq = (consumer_input *)malloc (sizeof (consumer_input));
	if (tq == NULL) return (NULL);

	return tq;
}

// Initialize the task queue.
task_queue *initTaskQ() {
	task_queue *tq;

	// Allocate the data for the queue.
	tq = (task_queue *)malloc (sizeof (task_queue));
	if (tq == NULL) return (NULL);

	// Set up its internal values.
	tq->isEmpty = 1;
	tq->isFull = 0;
	tq->head = 0;
	tq->tail = 0;
	tq->lock = (pthread_mutex_t *) malloc (sizeof (pthread_mutex_t));
	pthread_mutex_init (tq->lock, NULL);
	tq->notFull = (pthread_cond_t *) malloc (sizeof (pthread_cond_t));
	pthread_cond_init (tq->notFull, NULL);
	tq->notEmpty = (pthread_cond_t *) malloc (sizeof (pthread_cond_t));
	pthread_cond_init (tq->notEmpty, NULL);

	// Return the initialized queue.
	return tq;
}

// Add the given integer to the given task queue.
void addToTaskQ (task_queue *tq, int in) {
	// Put the data in the queue.
	tq->data[tq->tail] = in;
	// Make the tail point to the next item.
	tq->tail++;
	// If the queue is full, the tail should point back to the first entry.
	if (tq->tail == 2) {
		tq->tail = 0;
	}
	// If the tail points to the head, the queue must be full.
	if (tq->tail == tq->head) {
		tq->isFull = 1;
	}
	// Mark that the queue isn't empty.
	tq->isEmpty = 0;

	return;
}

// Delete the next task from the given task queue.
void delFromTaskQ (task_queue *tq) {
	// Make the head point to the next item.
	tq->head++;
	// If the head would point to an illegal place, fix it.
	if (tq->head == 2) {
		tq->head = 0;
	}
	// If the head is now also the tail, the queue must be empty.
	if (tq->head == tq->tail) {
		tq->isEmpty = 1;
	}
	// Mark that the queue isn't full.
	tq->isFull = 0;

	return;
}

// Delete the given task queue.
void deleteTaskQ (task_queue *tq) {
	// Delete everything inside the task_queue struct.
	pthread_mutex_destroy (tq->lock);
	free(tq->lock);
	pthread_cond_destroy (tq->notFull);
	free(tq->notFull);
	pthread_cond_destroy (tq->notEmpty);
	free(tq->notEmpty);
	free(tq);
}

// Delete the given consumer input.
void deleteConsumerInput (consumer_input *ci) {
	// Delete everything inside the consumer_input struct.
	free(ci);
}

// Method run by producer thread.
void *produce(void * tq) {
	// Store the task queue locally.
	task_queue *taskQ = (task_queue *)tq;

	int curTask;
	// Loop until there are no more tasks.
	for(curTask = 1; curTask <= N; curTask++) {
		// Lock access to the task queue.
		pthread_mutex_lock (taskQ->lock);
		// As long as the queue is full, wait for the consumers.
		while(taskQ->isFull) {
			pthread_cond_wait(taskQ->notFull, taskQ->lock);
		}
		// If the queue isn't full anymore, add some data to it.
		addToTaskQ(taskQ, curTask);
		// Print something saying the task was added.
		printf("Producer: added task %i to queue.\n", curTask);
		// Signal that the queue is not empty.
		pthread_cond_signal(taskQ->notEmpty);
		// Unlock access to the task queue.
		pthread_mutex_unlock(taskQ->lock);
	}

	return NULL;
}

// Method run by first consumer thread.
void *consume(void * ci) {
	// Get the consumer input.
	consumer_input *input = (consumer_input *)ci;
	// Store the task queue locally.
	task_queue *taskQ = input->tq;
	// Store the consumer number locally.
	int threadNum = input->thread_num;

	// Loop until all the tasks have been consumed.
	while(tasksCompleted < N) {
		// Lock access to the task queue.
		pthread_mutex_lock(taskQ->lock);
		// As long as the queue is empty, wait for the producer.
		while(taskQ->isEmpty) {
			pthread_cond_wait(taskQ->notEmpty, taskQ->lock);
		}
		if(tasksCompleted < N) {
			// Increment the number of tasks completed by this consumer.
			switch(threadNum) {
			case 1: tasksCompletedConsumer1++; break;
			case 2: tasksCompletedConsumer2++; break;
			}
			// Consume a task.
			delFromTaskQ(taskQ);
			// Print something saying which task was consumed.
			printf("Consumer%i: extracted task %i from queue.\n", threadNum, ++tasksCompleted);
			// Signal that the task queue isn't full.
			pthread_cond_signal(taskQ->notFull);
		}
		// Unlock access to the task queue.
		pthread_mutex_unlock(taskQ->lock);
	}

	// Mark this consumer thread as done.
	if(threadNum == 1) {
		consumerOneDone = 1;
	}
	else {
		consumerTwoDone = 1;
	}

	// Broadcast a bunch of times to get the other threads to finish.
	while((consumerOneDone & consumerTwoDone) == 0) {
		taskQ->isEmpty = 0;
		pthread_cond_broadcast(taskQ->notEmpty);
	}

	return NULL;
}

// Main method that initializes and runs the threads.
int main(int argc, char** argv) {
	task_queue *taskQ; // The task queue
	consumer_input *input1, *input2;
	pthread_t producer; // Producer thread
	pthread_t consumer1, consumer2; // Consumer threads

	// Get user input.
	if(argc == 2) {
		N = atoi(argv[1]);
	}

	// Initialize the task queue.
	taskQ = initTaskQ();

	// Initialize the consumer inputs.
	input1 = initConsumerInput();
	input2 = initConsumerInput();

	// Create the input for the consumer threads.
	input1->thread_num = 1;
	input1->tq = taskQ;
	input2->thread_num = 2;
	input2->tq = taskQ;

	// Create the producer thread.
	pthread_create(&producer, NULL, produce, taskQ);

	// Create the consumer threads.
	pthread_create(&consumer1, NULL, consume, input1);
	pthread_create(&consumer2, NULL, consume, input2);

	//  Wait for each thread to finish.
	pthread_join (producer, NULL);
	pthread_join (consumer1, NULL);
	pthread_join (consumer2, NULL);

	// Print the number of tasks consumed by each consumer.
	printf("Tasks consumed by Consumer 1: %i\n", tasksCompletedConsumer1);
	printf("Tasks consumed by Consumer 2: %i\n", tasksCompletedConsumer2);

	// Delete the consumer inputs.
	deleteConsumerInput(input1);
	deleteConsumerInput(input2);

	// Delete the task queue once the threads are done with it.
	deleteTaskQ(taskQ);

	return 0;
}
