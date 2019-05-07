#include "requestsQueue.h"

RequestsQueue* createRequestsQueue()
{
    RequestsQueue* Q;
    Q = (RequestsQueue *)malloc(sizeof(RequestsQueue));
    Q->front = NULL;
    Q->size = 0;

    return Q;
}

void push(RequestsQueue* Q, tlv_request_t request)
{
    Node* node = (Node*) malloc(sizeof(Node));
    node->nextNode = NULL;
    node->request = request;

    if(Q->front == NULL) Q->front = node;
    else Q->front->nextNode = node;

    Q->size++;
}

tlv_request_t* getFront(RequestsQueue* Q)
{
    if(Q->front == NULL) return NULL;
    else return &(Q->front->request);
}

void popFront(RequestsQueue* Q)
{
    if(Q->front != NULL)
    {
        Node* front = Q->front;
        Q->front = front->nextNode;

        free(front);
        Q->size--;
    }
}

int getSize(RequestsQueue* Q)
{
    return Q->size;
}