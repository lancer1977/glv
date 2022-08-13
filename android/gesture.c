/*
  Based on https://github.com/android/ndk-samples/blob/master/teapots/common/ndk_helper/gestureDetector.cpp
*/


#include "gesture.h"


static int32_t event_findIndex(const AInputEvent* ev, int32_t id)
{
    int32_t i;
    int32_t pcount = AMotionEvent_getPointerCount(ev);
    for( i = 0; i < pcount; ++i )
    {
        if( id == AMotionEvent_getPointerId(ev, i) )
            return i;
    }
    return -1;
}


void pinch_init(PinchDetector* det)
{
    det->idCount = 0;
}


static void pinch_appendId(PinchDetector* det, int32_t id)
{
    if( det->idCount < PINCH_MAX_IDS )
        det->pointerId[ det->idCount++ ] = id;
}


// Remove id from pointerId array and return index of the removed element.
static int pinch_removeId(PinchDetector* det, int32_t id)
{
    int32_t* it  = det->pointerId;
    int32_t* end = it + det->idCount;
    int32_t index = 0;
    for( ; it != end; ++it, ++index )
    {
        if( *it == id )
        {
            for( --end; it != end; ++it )   // Pack remaining elements.
                it[0] = it[1];
            --det->idCount;
            break;
        }
    }
    return index;
}


/*
  Return GestureState given an event of type AINPUT_EVENT_TYPE_MOTION.
*/
int pinch_detect(PinchDetector* det, const AInputEvent* ev)
{
    int state = GESTURE_STATE_NONE;
    int32_t action = AMotionEvent_getAction(ev);
    int32_t count  = AMotionEvent_getPointerCount(ev);

    switch( action & AMOTION_EVENT_ACTION_MASK )
    {
        case AMOTION_EVENT_ACTION_DOWN:
            pinch_appendId(det, AMotionEvent_getPointerId(ev, 0));
            break;

        case AMOTION_EVENT_ACTION_POINTER_DOWN:
        {
            int32_t index = (action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK)
                             >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
            pinch_appendId(det, AMotionEvent_getPointerId(ev, index));
            if( count == 2 )
                state = GESTURE_STATE_START;
        }
            break;

        case AMOTION_EVENT_ACTION_UP:
            --det->idCount;
            break;

        case AMOTION_EVENT_ACTION_POINTER_UP:
        {
            int32_t index = (action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK)
                             >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
            int i = pinch_removeId(det, AMotionEvent_getPointerId(ev, index));
            if( i <= 1 ) {
                // Start new gesture.
                if( count != 2 )
                    state = GESTURE_STATE_START | GESTURE_STATE_END;
            }
        }
            break;

        case AMOTION_EVENT_ACTION_MOVE:
            if( count > 1 )
                state = GESTURE_STATE_MOVE;     // Multi touch.
            break;

        case AMOTION_EVENT_ACTION_CANCEL:
            break;
    }
    return state;
}


/*
  Return non-zero if pinch is active and pointer p1 & p2 x/y are set.
*/
int pinch_positions(PinchDetector* det, const AInputEvent* ev,
                    float* p1, float* p2)
{
    int32_t index;

    if( det->idCount < 2 )
        return 0;

    index = event_findIndex(ev, det->pointerId[0]);
    if( index < 0 )
        return 0;
    p1[0] = AMotionEvent_getX(ev, index);
    p1[1] = AMotionEvent_getY(ev, index);

    index = event_findIndex(ev, det->pointerId[1]);
    if( index < 0 )
        return 0;
    p2[0] = AMotionEvent_getX(ev, index);
    p2[1] = AMotionEvent_getY(ev, index);
    return 1;
}


//----------------------------------------------------------------------------


#define DOUBLE_TAP_TIMEOUT  (300 * 1000000)
#define DOUBLE_TAP_SLOP     100


void dtap_init(DoubleTapDetector* det)
{
    det->pointerId = -1;
    det->tapTime = 0;
}


int dtap_detect(DoubleTapDetector* det, const AInputEvent* ev, float dpFactor)
{
    int32_t action;

    // Only support single touch
    if (AMotionEvent_getPointerCount(ev) > 1)
        return GESTURE_STATE_NONE;

    action = AMotionEvent_getAction(ev);
    switch (action & AMOTION_EVENT_ACTION_MASK)
    {
        case AMOTION_EVENT_ACTION_DOWN:
            if (det->pointerId < 0) {
                det->pointerId = AMotionEvent_getPointerId(ev, 0);
            } else {
                det->pointerId = -1;
                if (AMotionEvent_getEventTime(ev) - det->tapTime <=
                    DOUBLE_TAP_TIMEOUT) {
                    float x = AMotionEvent_getX(ev, 0) - det->tapX;
                    float y = AMotionEvent_getY(ev, 0) - det->tapY;
                    if (x * x + y * y <
                        DOUBLE_TAP_SLOP * DOUBLE_TAP_SLOP * dpFactor) {
                        return GESTURE_STATE_ACTION;
                    }
                }
            }
            break;

        case AMOTION_EVENT_ACTION_UP:
            if (AMotionEvent_getPointerId(ev, 0) == det->pointerId) {
                det->tapTime = AMotionEvent_getEventTime(ev);
                det->tapX    = AMotionEvent_getX(ev, 0);
                det->tapY    = AMotionEvent_getY(ev, 0);
            }
            break;
    }
    return GESTURE_STATE_NONE;
}


//----------------------------------------------------------------------------


#define PAD_RADIUS  40.0f
#define PAD_ACTIVE  (det->pointerId >= 0)


void dpad_init(DPadDetector* det)
{
    det->state = det->prevState = 0;
    det->pointerId = -1;
    /*
    det->rect[0] = det->rect[1] = 0.0f;
    det->rect[2] = det->rect[3] = 400.0f;
    */
}


/*
  Update DPadDetector given an event of type AINPUT_EVENT_TYPE_MOTION.

  Return non-zero if the state changed.
*/
int dpad_detect(DPadDetector* det, const AInputEvent* ev, float dpFactor)
{
    int32_t idx;
    int32_t action = AMotionEvent_getAction(ev);
    int32_t count  = AMotionEvent_getPointerCount(ev);
    uint16_t state;

    switch( action & AMOTION_EVENT_ACTION_MASK )
    {
        case AMOTION_EVENT_ACTION_DOWN:
            idx = 0;
action_down:
            if (! PAD_ACTIVE) {
                float x = AMotionEvent_getX(ev, idx);
                float y = AMotionEvent_getY(ev, idx);
                if (x >= det->rect[0] && x < det->rect[2] &&
                    y >= det->rect[1] && y < det->rect[3]) {
                    det->pointerId = AMotionEvent_getPointerId(ev, idx);
                    det->originX = x;
                    det->originY = y;
                    state = DPAD_STATE_ACTIVE;
                    goto changed;
                }
            }
            break;

        case AMOTION_EVENT_ACTION_POINTER_DOWN:
            idx = (action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK)
                   >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
            goto action_down;

        case AMOTION_EVENT_ACTION_UP:
        case AMOTION_EVENT_ACTION_CANCEL:
            if (PAD_ACTIVE) {
                for (idx = 0; idx < count; ++idx) {
                    if (AMotionEvent_getPointerId(ev, idx) == det->pointerId) {
                        det->pointerId = -1;
                        state = 0;
                        goto changed;
                    }
                }
            }
            break;

        case AMOTION_EVENT_ACTION_POINTER_UP:
            idx = (action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK)
                  >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
            if (AMotionEvent_getPointerId(ev, idx) == det->pointerId) {
                det->pointerId = -1;
                state = 0;
                goto changed;
            }
            break;

        case AMOTION_EVENT_ACTION_MOVE:
            if (! PAD_ACTIVE)
                return 0;

            for (idx = 0; idx < count; ++idx) {
                if (AMotionEvent_getPointerId(ev, idx) == det->pointerId) {
                    float dx = AMotionEvent_getX(ev, idx) - det->originX;
                    float dy = AMotionEvent_getY(ev, idx) - det->originY;
                    float radius = PAD_RADIUS * dpFactor;
                    state = DPAD_STATE_ACTIVE;

                    if (dx < -radius)
                        state |= DPAD_STATE_LEFT;
                    else if (dx > radius)
                        state |= DPAD_STATE_RIGHT;

                    if (dy < -radius)
                        state |= DPAD_STATE_UP;
                    else if (dy > radius)
                        state |= DPAD_STATE_DOWN;

                    if (state != det->state)
                        goto changed;
                    break;
                }
            }
            break;
    }
    return 0;

changed:
    det->prevState = det->state;
    det->state = state;
    return 1;
}
