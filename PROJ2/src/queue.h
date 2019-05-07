#include <stdio.h>
#include <stdlib.h>
#include "types.h"

typedef struct Node
{
    tlv_request_t request;
    Node* nextNode;
}Node;

typedef struct RequestsQueue
{
    Node* front;
    int size;
}RequestsQueue;

RequestsQueue* createRequestsQueue();
void push(RequestsQueue* Q, tlv_request_t request);
tlv_request_t* getFront(RequestsQueue* Q);
void popFront(RequestsQueue* Q);
int getSize(RequestsQueue* Q);