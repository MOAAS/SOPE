#include "queue.h"

RequestQueue* queue_create(unsigned capacity) { 
    RequestQueue* queue = malloc(sizeof(RequestQueue)); 
    queue->capacity = capacity; 
    queue->front = queue->size = 0;  
    queue->rear = capacity - 1;
    queue->array = malloc(queue->capacity * sizeof(tlv_request_t)); 
    return queue; 
} 

void queue_destroy(RequestQueue* q) {
	free(q->array);
	free(q);
}


bool queue_full(RequestQueue* queue) {  
	return (queue->size == queue->capacity);  
} 
  
bool queue_empty(RequestQueue* queue) {  
	return (queue->size == 0);
} 

void queue_push(RequestQueue* queue, tlv_request_t item) { 
    if (queue_full(queue)) {
		queue_resize(queue);
	}
    queue->size++;
    queue->rear++;
	if (queue->rear == queue->capacity) {
		queue->rear = 0; 
	}
    queue->array[queue->rear] = item;
} 

void queue_resize(RequestQueue* queue) {
	RequestQueue* new_queue = queue_create(queue->capacity * 2);
	while (queue->size > 0) {
		queue_push(new_queue, queue_pop(queue));
	}
	free(queue->array);
	*queue = *new_queue;
}

tlv_request_t queue_pop(RequestQueue* queue) { 
    tlv_request_t item = queue->array[queue->front]; 
    queue->size--;
    queue->front++;
	if (queue->front == queue->capacity)
		queue->front = 0; 
    return item; 
} 
  
tlv_request_t queue_front(RequestQueue* queue) { 
    return queue->array[queue->front]; 
} 
