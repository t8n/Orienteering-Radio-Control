/*
 * fifoQueue.c
 *
 *  Created on: Mar 17, 2024
 *      Author: tateneedham
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "fifoQueue.h"

/// A pretty standard First In, First Out Queue that can hold any arbitrary data
/// To be used for adding things to be processed later (eg. log messages, SRR punches, etc)

void queueInit(struct FifoQueue *q) {
    q->front = q->rear = NULL;
}

bool queueIsEmpty(struct FifoQueue *q) {
    return (q->front == NULL);
}

void queuePush(struct FifoQueue *q, struct QueueData data) {
    struct QueueNode *newNode = (struct QueueNode *)malloc(sizeof(struct QueueNode));
    if (newNode == NULL) {
        // Memory allocation failed. This is bad. Maybe just die for now?
        exit(1);
    }
    if (data.size == 0) {
        return;
    }

    newNode->data.size = data.size;
    newNode->data.data = (uint8_t *)malloc(data.size * sizeof(uint8_t));
    if (newNode->data.data == NULL) {
        // Memory allocation failed. This is equally bad. Maybe just die for now?
        free(newNode);
        exit(1);
    }
    memcpy(newNode->data.data, data.data, data.size);
    newNode->next = NULL;

    if (queueIsEmpty(q)) {
        q->front = q->rear = newNode;
    } else {
        q->rear->next = newNode;
        q->rear = newNode;
    }
}

struct QueueData queuePop(struct FifoQueue *q) {
    if (queueIsEmpty(q)) {
        // Queue is empty. Logic error, as this should never happen. Maybe just die for now?
        exit(1);
    }

    struct QueueNode *temp = q->front;
    struct QueueData data;
    data.size = temp->data.size;
    data.data = (uint8_t *)malloc(data.size * sizeof(uint8_t));
    if (data.data == NULL) {
        // Memory allocation failed. Still bad. Die.
        exit(1);
    }
    memcpy(data.data, temp->data.data, data.size);

    q->front = q->front->next;
    free(temp->data.data);  // Free memory allocated for the data
    free(temp);             // Free memory allocated for the node

    if (q->front == NULL) {
        q->rear = NULL;
    }

    return data;
}
