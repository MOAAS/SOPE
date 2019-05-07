#pragma once

#include <stdlib.h>

#include "sope.h"

/** @defgroup queue queue
 * @{
 *
 * Functions used to create and manipulate simple array-based queues of 8 bit unsigned integers.
 */

/**
 * @brief Represents a queue.
 * 
 */
typedef struct { 
    int front;      ///< Front of the queue
    int rear;       ///< Rear of the queue
    int capacity;   ///< Queue capacity
    int size;       ///< Queue current size
    tlv_request_t* array; ///< Queue's elements
} RequestQueue;

/**
 * @brief Creates a queue
 * @param capacity Queue initial capacity
 * @return Queue* Returns the created queue
 */
RequestQueue* queue_create(unsigned capacity);

/**
 * @brief Destroys a queue, freeing all used memory.
 * 
 * @param q Queue to be destroyed
 */
void queue_destroy(RequestQueue* q);

/**
 * @brief Gets the front of the queue
 * 
 * @param q Queue to read
 * @return uint8_t Returns the front of the queue
 */
tlv_request_t queue_front(RequestQueue *q);

/**
 * @brief Removes the front of the queue
 * 
 * @param q Queue to pop
 * @return uint8_t Returns the removed item
 */
tlv_request_t queue_pop(RequestQueue* q);

/**
 * @brief Pushes an item to the end of the queue
 * 
 * @param q Queue to add an element to
 * @param data Byte to add to the queue
 */
void queue_push(RequestQueue *q, tlv_request_t data);

/**
 * @brief Checks if a queue is full
 * 
 * @param queue Queue to inspect
 * @return true The queue is full
 * @return false The queue is not full
 */
bool queue_full(RequestQueue* queue);

/**
 * @brief Checks if a queue is empty
 * 
 * @param queue Queue to inspect
 * @return true The queue is empty
 * @return false The queue is not empty
 */
bool queue_empty(RequestQueue* queue);

/**
 * @brief Doubles a queue's capacity. Called by queue_push when needed.
 * 
 * @param queue Queue to increase capacity.
 */
void queue_resize(RequestQueue* queue);

  

