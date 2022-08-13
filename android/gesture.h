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


#define PINCH_MAX_IDS 6

typedef struct
{
    int32_t pointerId[ PINCH_MAX_IDS ];
    uint32_t idCount;
}
PinchDetector;


typedef struct
{
    int32_t pointerId;
    float tapX;
    float tapY;
    int64_t tapTime;
}
DoubleTapDetector;


enum DPadState
{
    DPAD_STATE_ACTIVE = 1,
    DPAD_STATE_UP     = 4,
    DPAD_STATE_DOWN   = 8,
    DPAD_STATE_LEFT   = 0x10,
    DPAD_STATE_RIGHT  = 0x20
};

typedef struct
{
    uint16_t state;
    uint16_t prevState;
    int32_t pointerId;
    float originX;
    float originY;
    float rect[4];      // minX, minY, maxX, maxY
}
DPadDetector;


#ifdef __cplusplus
extern "C" {
#endif

extern void pinch_init(PinchDetector*);
extern int  pinch_detect(PinchDetector*, const AInputEvent*);
extern int  pinch_positions(PinchDetector*, const AInputEvent*,
                            float* p1, float* p2);

extern void dtap_init(DoubleTapDetector*);
extern int  dtap_detect(DoubleTapDetector*, const AInputEvent*, float dpFactor);

extern void dpad_init(DPadDetector*);
extern int  dpad_detect(DPadDetector*, const AInputEvent*, float dpFactor);

#ifdef __cplusplus
}
#endif


#endif //GESTURE_H
