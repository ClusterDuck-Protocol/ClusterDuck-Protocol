/**
   arduino-timer - library for delaying function calls

   Copyright (c) 2018, Michael Contreras
   All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are
   met:

   1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

   3. Neither the name of the copyright holder nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
   IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
   TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
   PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
   HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
   TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef _CM_ARDUINO_TIMER_H__
#define _CM_ARDUINO_TIMER_H__

#if defined(ARDUINO) && ARDUINO >= 100
#include <Arduino.h>
#else
#include <WProgram.h>
#endif

#ifndef TIMER_MAX_TASKS
    #define TIMER_MAX_TASKS 0x10
#endif

template <
    size_t max_tasks = TIMER_MAX_TASKS, /* max allocated tasks */
    unsigned long (*time_func)() = millis /* time function for timer */
>
class Timer {
  public:

    typedef bool (*handler_t)(void *opaque); /* task handler func signature */

    /* Calls handler with opaque as argument in delay units of time */
    bool
    in(unsigned long delay, handler_t h, void *opaque = NULL)
    {
        return add_task(time_func(), delay, h, opaque);
    }

    /* Calls handler with opaque as argument at time */
    bool
    at(unsigned long time, handler_t h, void *opaque = NULL)
    {
        const unsigned long now = time_func();
        return add_task(now, time - now, h, opaque);
    }

    /* Calls handler with opaque as argument every interval units of time */
    bool
    every(unsigned long interval, handler_t h, void *opaque = NULL)
    {
        return add_task(time_func(), interval, h, opaque, interval);
    }

    /* Ticks the timer forward - call this function in loop() */
    void
    tick()
    {
        tick(time_func());
    }

    /* Ticks the timer forward - call this function in loop() */
    inline
    void
    tick(unsigned long t)
    {
        for (size_t i = 0; i < max_tasks; ++i) {
            struct task * const task = &tasks[i];
            const unsigned long duration = t - task->start;

            if (task->handler && duration >= task->expires) {
                task->repeat = task->handler(task->opaque) && task->repeat;

                if (task->repeat) task->start = t;
                else remove(task);
            }
        }
    }

  private:

    struct task {
        handler_t handler; /* task handler callback func */
        void *opaque; /* argument given to the callback handler */
        unsigned long start,
                      expires, /* when the task expires */
                      repeat; /* repeat task */
    } tasks[max_tasks];

    inline
    void
    remove(struct task *task)
    {
        task->handler = NULL;
        task->opaque = NULL;
        task->start = 0;
        task->expires = 0;
        task->repeat = 0;
    }

    inline
    struct task *
    next_task_slot()
    {
        for (size_t i = 0; i < max_tasks; ++i) {
            struct task * const slot = &tasks[i];
            if (slot->handler == NULL) return slot;
        }

        return NULL;
    }

    inline
    struct task *
    add_task(unsigned long start, unsigned long expires,
             handler_t h, void *opaque, bool repeat = 0)
    {
        struct task * const slot = next_task_slot();

        if (!slot) return NULL;

        slot->handler = h;
        slot->opaque = opaque;
        slot->start = start;
        slot->expires = expires;
        slot->repeat = repeat;

        return slot;
    }
};

/* create a timer with the default settings */
inline Timer<>
timer_create_default()
{
    return Timer<>();
}

#endif
