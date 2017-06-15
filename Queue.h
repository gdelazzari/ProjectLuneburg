#ifndef _QUEUE_H
#define _QUEUE_H


template<typename T>
class Queue {

  public:
    Queue(int maxSize);
    ~Queue(void);

    void push(const T obj);

    T pop(void);
    T peek(void) const;

    int count(void) const;
    bool empty(void) const;

    int getAverageUsage(void) const;
    int getMaximumUsage(void) const;

  private:
    int maxSize;

    T* pool;
    bool* usedFlags;

    int queueLen;
    int* queue;

    int maxUsage = 0;
    int avgUsage = 0;
    int avgCount = 0;

    int getFreePoolIdx(void) const;
    void updateAverage(void);

};


/* Class implementation */

template<typename T>
Queue<T>::Queue(int maxSize) {
  this->maxSize = maxSize;
  this->pool = (T*) malloc(sizeof(T) * this->maxSize);
  this->usedFlags = (bool*) malloc(sizeof(bool) * this->maxSize);
  this->queue = (int*) malloc(sizeof(int) * this->maxSize);

  for (int i = 0; i < this->maxSize; i++) {
    this->usedFlags[i] = false;
  }

  this->queueLen = 0;
}

template<typename T>
Queue<T>::~Queue(void) {
  free(this->pool);
  free(this->usedFlags);
  free(this->queue);
}

template<typename T>
void Queue<T>::push(const T obj) {
  if ((this->queueLen + 1) > this->maxSize)
    return;

  int poolIdx = this->getFreePoolIdx();

  if (poolIdx == -1)
    return;

  this->pool[poolIdx] = obj;              // Put the object in the pool
  this->queue[this->queueLen] = poolIdx;  // Assign the obj pool id to the queue
  this->usedFlags[poolIdx] = true;        // Flag the pool obj as used

  this->queueLen++;

  /* Statistics */
  if (this->queueLen > this->maxUsage) {
    this->maxUsage = this->queueLen;
  }

  this->updateAverage();
}

template<typename T>
T Queue<T>::pop(void) {
  if (this->queueLen == 0) {
    // The queue is empty... return something casual because we can't handle
    // exceptions any other way
    return this->pool[0];
  }

  int poolIdx = this->queue[0];     // Get the obj index in the pool

  // Shift everything up
  for (int i = 0; i < (this->queueLen - 1); i++) {
    if (i == (this->queueLen - 1)) {
      this->queue[i] = 0;
    } else {
      this->queue[i] = this->queue[i + 1];
    }
  }

  this->queueLen--;                 // Keep track that we removed one element
  this->usedFlags[poolIdx] = false; // And flag it as free in the pool

  return this->pool[poolIdx];       // Return the obj tracked above from the pool
}

template<typename T>
T Queue<T>::peek(void) const {
  if (this->queueLen == 0) {
    // The queue is empty... return something casual because we can't handle
    // exceptions any other way
    return this->pool[0];
  }

  int poolIdx = this->queue[0];

  return this->pool[poolIdx];
}

template<typename T>
int Queue<T>::count(void) const {
  return this->queueLen;
}

template<typename T>
bool Queue<T>::empty(void) const {
  return (this->queueLen == 0) ? true : false;
}

template<typename T>
int Queue<T>::getAverageUsage(void) const {
  return this->avgUsage;
}

template<typename T>
int Queue<T>::getMaximumUsage(void) const {
  return this->maxUsage;
}

template<typename T>
void Queue<T>::updateAverage(void) {
  this->avgUsage = ((this->avgUsage * 3) + this->queueLen) / 4;
}

template<typename T>
int Queue<T>::getFreePoolIdx() const {
  for (int i = 0; i < this->maxSize; i++) {
    if (this->usedFlags[i] == false)
      return i;
  }

  return -1;
}


#endif
