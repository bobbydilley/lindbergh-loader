/* 
 * Filename:    dqueue.c
 * Date:        13/10/17
 * Licence:     GNU GPL V3
 *
 * Library for a generic, dynamically allocated queue
 *
 * Functions:
 * queue_t * queue_init(unsigned int block_num, size_t block_size, size_t element_size);    - Initialise the queue data structure and return a pointer to the first element 
 * void * queue_pop(queue_t * queue);                                                       - Pop an element from the front of the queue
 * int queue_push(const void * const element, queue_t * queue);                             - Push an element to the back of the queue
 * int queue_debug(const queue_t * const queue);                                            - Dump information about the queue 
 * void queue_destroy(queue_t * queue);                                                     - Destroy the queue data structure
 *
 * Return/exit codes:
 *  QUEUE_OK        - No error
 *  SIZE_ERROR      - Queue size error (invalid block size or number of elements)
 *  MEM_ERROR       - Memory allocation error
 *  INDEX_ERROR     - Couldn't pop data from the queue
 *
 * Todo:
 * 
 */

#include "dqueue.h"
#include <stdio.h>
#include <string.h>

//#define _DEBUG

queue_t * queue_init(unsigned int block_num, size_t block_size, size_t element_width)
{
    queue_t * queue;
    unsigned int i, j;

#ifdef _DEBUG
	printf("queue_init() block_num:%d block_size:%d element_width:%d\r\n",
		block_num, block_size, element_width);
#endif
    if(block_size == 0)
        block_size = DEFAULT_BLOCK;

    if(!(queue = (queue_t*)malloc(sizeof(queue_t))))
        return NULL;

    if((queue->block_size = block_size) <= 0 || (queue->total_blocks = block_num) <= 0 || (queue->element_width = element_width) <= 0) {
        queue->status = SIZE_ERROR;
	printf("queue_init() SIZE_ERROR\r\n");
        return queue;
    }

    if(!(queue->base_p = (char**)malloc(queue->total_blocks * sizeof(char *)))) {
        queue->status = MEM_ERROR;
	printf("queue_init() MEM_ERROR\r\n");
        return queue;
    }

    for(i = 0; i < queue->total_blocks; i++) {
        if(!(queue->base_p[i] = (char*)malloc(queue->block_size * queue->element_width))) {
            fprintf(stderr, "Error: Could not allocate memory!\n");
            
            for(j = 0; j < i; j++)
                free(queue->base_p[i]);

            free(queue->base_p);
        }
    }

    queue->cur_block = queue->last_block = 0;
    queue->cur_block_pos = queue->last_block_pos = 0;
    queue->status = QUEUE_OK;

    return queue;
}

void queue_destroy(queue_t * queue)
{
    while(queue->cur_block < queue->total_blocks)
        free(queue->base_p[queue->cur_block++]);
    
    queue->cur_block        = 0;
    queue->cur_block_pos    = 0;
    queue->last_block       = 0;
    queue->last_block_pos   = 0;
    queue->total_blocks     = 0;
    queue->block_size       = 0;
    queue->element_width    = 0;
    queue->status           = 0;

    free(queue->base_p);
    queue->base_p           = NULL;
    free(queue);
}

int queue_push(const void * const element, queue_t * queue)
{
    memcpy(queue->base_p[queue->last_block] + queue->last_block_pos * queue->element_width, element, queue->element_width);

    if(queue->last_block == (queue->total_blocks - queue->cur_block) - 1 && queue->last_block_pos == queue->block_size - 1) {
        queue->total_blocks++;
        queue->last_block++;
        queue->last_block_pos = 0;

        if(!(queue->base_p = (char**)realloc(queue->base_p, (queue->total_blocks - queue->cur_block) * sizeof(void *)))) {
            fprintf(stderr, "Error: Could not reallocate memory!\n");
            queue->status = MEM_ERROR;
            queue->total_blocks--;
            queue->last_block--;
            queue->last_block_pos = queue->block_size - 1;
            return MEM_ERROR;
        }

        if(!(queue->base_p[queue->last_block] = (char*)malloc(queue->block_size * queue->element_width))) {
            fprintf(stderr, "Error: Could not allocate memory!\n");
            queue->total_blocks--;
            queue->last_block--;
            queue->last_block_pos = queue->block_size - 1;
            queue->status = MEM_ERROR;
            return MEM_ERROR;
        }
    } else if(queue->last_block_pos == queue->block_size - 1) {
        queue->last_block++;
        queue->last_block_pos = 0;
    } else {
        queue->last_block_pos++;
    }
    
    return QUEUE_OK;
}

void * queue_pop(queue_t * queue)
{
    void * data;

    if(queue->last_block == queue->cur_block && queue->cur_block_pos == queue->last_block_pos) {
        fprintf(stderr, "Error: Queue empty!\n");
        queue->status = INDEX_ERROR;
        return NULL;
    }

    if(!(data = malloc(queue->element_width))) {
        fprintf(stderr, "Error: Could not allocate memory!\n");
        queue->status = MEM_ERROR;
        return NULL;
    }

    if(queue->cur_block_pos == queue->block_size - 1) {
        memcpy(data, queue->base_p[queue->cur_block] + queue->cur_block_pos * queue->element_width, queue->element_width);
        free(queue->base_p[queue->cur_block]);

        queue->cur_block++;
        queue->cur_block_pos = 0;
    } else {
        memcpy(data, queue->base_p[queue->cur_block] + queue->cur_block_pos * queue->element_width, queue->element_width);
        queue->cur_block_pos++;
    }

    return data;
}

int queue_isempty(const queue_t * const queue)
{
	int result = 1;
    if(queue == NULL) {
        printf("Error: Invalid queue pointer!\n");
        return MEM_ERROR;
    }

	result = (queue->total_blocks - queue->cur_block) == queue->total_blocks;
#ifdef _DEBUG
	printf("queue_isempty() %i - %i -> %i\r\n", 
		queue->total_blocks, queue->cur_block, result);
#endif
	return result;
}

int queue_debug(const queue_t * const queue)
{
    if(queue == NULL) {
        printf("Error: Invalid queue pointer!\n");
        return MEM_ERROR;
    }

    if(queue->status == QUEUE_OK)
        printf("Queue has %d blocks of size %d and each element is %d bytes wide!\n", (queue->total_blocks - queue->cur_block), (int)queue->block_size, (int)queue->element_width);
    else if(queue->status == MEM_ERROR)
        printf("Memory error in queue!\n");
    else if(queue->status == SIZE_ERROR)
        printf("Size error in queue");

    return queue->status;
}
