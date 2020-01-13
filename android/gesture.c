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
    if( det->idCount < GESTURE_MAX_IDS )
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
