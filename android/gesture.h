#ifndef GESTURE_H
#define GESTURE_H


#include <android/input.h>


enum GestureState
{
    GESTURE_STATE_NONE  = 0,
    GESTURE_STATE_START = 1,
    GESTURE_STATE_MOVE  = 2,
    GESTURE_STATE_END   = 4,
    GESTURE_STATE_ACTION = (GESTURE_STATE_START | GESTURE_STATE_END)
};


#define GESTURE_MAX_IDS 6

typedef struct
{
    int32_t pointerId[ GESTURE_MAX_IDS ];
    uint32_t idCount;
}
PinchDetector;


#ifdef __cplusplus
extern "C" {
#endif

extern void pinch_init(PinchDetector*);
extern int  pinch_detect(PinchDetector*, const AInputEvent*);
extern int  pinch_positions(PinchDetector*, const AInputEvent*,
                            float* p1, float* p2);

#ifdef __cplusplus
}
#endif


#endif //GESTURE_H
