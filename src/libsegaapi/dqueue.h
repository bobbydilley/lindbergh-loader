#ifndef DQUEUE_H
#define DQUEUE_H

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

#define QUEUE_OK 0
#define MEM_ERROR -1                /* Memory allocation error */
#define SIZE_ERROR -2               /* Queue dimension error */
#define INDEX_ERROR -3              /* No data at index */

#define DEFAULT_BLOCK 256           /* By default use 256 bytes per block */

typedef struct {
    char ** base_p;                 /* Base pointer of the queue */
    unsigned int cur_block;         /* Index of the block containing the first element */
    unsigned int cur_block_pos;     /* Position of the first element within the block */
    unsigned int last_block;        /* Index of the block containing the last element */
    unsigned int last_block_pos;    /* Position of the last element within the block */
    unsigned int total_blocks;      /* Total number of blocks ever allocated to the queue */
    size_t block_size;              /* Number of elements in each block */
    size_t element_width;           /* Size of each element */
    int status;                     /* Status of the queue */
} queue_t;

queue_t * queue_init(unsigned int block_num, size_t block_size, size_t element_size);   /* Initialise the queue data structure and return a pointer to the first element */
void * queue_pop(queue_t * queue);                                                      /* Pop an element from the front of the queue */
int queue_push(const void * const element, queue_t * queue);                            /* Push an element to the back of the queue */
int queue_debug(const queue_t * const queue);                                           /* Dump information about the queue  */
void queue_destroy(queue_t * queue);                                                    /* Destroy the queue data structure */
int queue_isempty(const queue_t * const queue);

#ifdef __cplusplus
}
#endif

#endif // DQUEUE_H
