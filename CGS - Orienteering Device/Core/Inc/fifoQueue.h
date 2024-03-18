/*
 * fifoQueue.h
 *
 *  Created on: Mar 17, 2024
 *      Author: tateneedham
 */

#ifndef INC_FIFOQUEUE_H_
#define INC_FIFOQUEUE_H_

#include <stdbool.h>

struct QueueData {
    uint8_t *data;
    size_t size;
};

struct QueueNode {
    struct QueueData data;
    struct QueueNode *next;
};

struct FifoQueue {
    struct QueueNode *front;
    struct QueueNode *rear;
};

void queueInit(struct FifoQueue *q);
bool queueIsEmpty(struct FifoQueue *q);
void queuePush(struct FifoQueue *q, struct QueueData data);
struct QueueData queuePop(struct FifoQueue *q);

#endif /* INC_FIFOQUEUE_H_ */
