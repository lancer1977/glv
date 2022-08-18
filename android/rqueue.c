/*
    rqueue - A queue of values implemented with a circular buffer.

    Copyright (c) 2022 Karl Robillard

    Permission is hereby granted, free of charge, to any person obtaining a
    copy of this software and associated documentation files (the "Software"),
    to deal in the Software without restriction, including without limitation
    the rights to use, copy, modify, merge, publish, distribute, sublicense,
    and/or sell copies of the Software, and to permit persons to whom the
    Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in
    all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
    THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
    DEALINGS IN THE SOFTWARE.
*/
/*
    This is a C template for an expandable queue.

    To generate the source implementation:

        #include "rqueue.c"
        RQUEUE_IMP(MyStruct)

    If rqueue.c is included in more than one file in a project then all other
    inclusions must be preceeded by a "#define RQUEUE_NO_BASE" line.

    To generate header declarations:

        #include "rqueue.h"
        RQUEUE_DECLARE(MyStruct);
*/

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "rqueue.h"

#ifdef RQUEUE_NO_BASE
extern void rqueue_resize(RQueue* rb, int count, size_t elemSize);
#else
void rqueue_resize(RQueue* rb, int count, size_t elemSize)
{
    void* newValues = malloc(count * elemSize);
    assert(newValues);

    if (rb->used) {
        uint8_t* src = ((uint8_t*) rb->values) + rb->head * elemSize;
        if (rb->head + rb->used > rb->avail) {
            size_t len1 = rb->avail - rb->head;
            size_t len2 = rb->used - len1;

            len1 *= elemSize;
            memcpy(newValues, src, len1);
            memcpy(((uint8_t*) newValues) + len1, rb->values, len2 * elemSize);
        } else
            memcpy(newValues, src, rb->used * elemSize);

        rb->head = 0;
    }

    free(rb->values);
    rb->values = newValues;
    rb->avail = count;
}

void rqueue_init(RQueue* rb, int reserve, size_t elemSize)
{
    memset(rb, 0, sizeof(RQueue));
    if (reserve > 0)
        rqueue_resize(rb, reserve, elemSize);
}

void rqueue_clear(RQueue* rb)
{
    rb->head = rb->used = 0;
}

void rqueue_free(RQueue* rb)
{
    free(rb->values);
    rb->values = NULL;
    rb->avail = rb->used = 0;
}
#endif

/*
 * TYPE* rqueue<TYPE>_append(RQueue*)
 * Return pointer to a value in the buffer which the caller must initialize.
 *
 * TYPE* rqueue<TYPE>_removeHead(RQueue*)
 * Return pointer to the previous head value.
 */

#define RQUEUE_IMP(T) \
T* rqueue_append_ ## T(RQueue* rb) { \
    uint32_t next;  \
    if (rb->avail == 0)  \
        rqueue_resize(rb, 8, sizeof(T)); \
    else if (rb->used == rb->avail) \
        rqueue_resize(rb, rb->avail * 2, sizeof(T)); \
    next = (rb->head + rb->used) % rb->avail; \
    rb->used++; \
    return ((T*) rb->values) + next; \
} \
T* rqueue_removeHead_ ## T(RQueue* rb) { \
    uint32_t cur, next; \
    if (rb->used) { \
        rb->used--; \
        cur = rb->head; \
        next = cur + 1; \
        rb->head = (next >= rb->avail) ? 0 : next; \
        return ((T*) rb->values) + cur; \
    } \
    return NULL; \
}
