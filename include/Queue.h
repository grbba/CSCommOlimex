/**
 * @file Queue.h
 * @author Gregor Baues
 * @brief Queue datastructure implementation based on compile time allocated memeory. 
 * Size of the queue is part of the template
 * @date 2023-01-13
 *
 * @copyright Copyright (c) 2023
 *
 * This is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * See the GNU General Public License for more details <https://www.gnu.org/licenses/>
 */

#ifndef ARDUINO_QUEUE_H
#define ARDUINO_QUEUE_H

#include <Arduino.h>
#include <DIAG.h>

// Queue capacity is S - 1
template <typename T, size_t S>
class Queue
{
private:
  T queue_[S];
  size_t head_;
  size_t tail_;

public:
  Queue() : head_(0), tail_(0) {}

  bool isEmpty() const
  {
    return head_ == tail_;
  }

  bool isFull() const
  {
    return (tail_ + 1) % S == head_;
  }

  void push(const T &element)
  {
    if (isFull())
    {
      ERR(F("Queue is full. Element hasn't been queued" CR));
      return;
    }
    queue_[tail_] = element;
    tail_ = (tail_ + 1) % S;
  }

  T pop()
  {
    if (isEmpty())
    {
      WARN(F("Queue is empty. Returning void element" CR));
      return T();
    }
    T element = queue_[head_];
    head_ = (head_ + 1) % S;
    return element;
  }

  T peek()
  {
      if (isEmpty()) {
        WARN(F("Queue is empty. Returning void element" CR));
        return T();
      }
      return queue_[head_];
  }

  void print() const
  {
    size_t idx = head_;
    // std::cout << "start " << idx << CR;
    while (idx != tail_)
    {
      // print some info here of the queue :: needs a serializer of the content
      idx = (idx + 1) % S;
    }
  }

  size_t size() const
  {
    return (tail_ + S - head_) % S;
  }
};

#endif