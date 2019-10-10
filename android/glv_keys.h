#ifndef GLVIEW_KEYS_H
#define GLVIEW_KEYS_H
/*===========================================================================/

  GLView Keys for Android
  Copyright (C) 2012 Karl Robillard

  These defines and macros are for GLV_EVENT_KEY_DOWN/UP events.

/===========================================================================*/


#include <android/keycodes.h>


#define KEY_Grave        AKEYCODE_GRAVE
#define KEY_Escape       AKEYCODE_ESCAPE
#define KEY_Space        AKEYCODE_SPACE
#define KEY_Return       AKEYCODE_ENTER
#define KEY_Tab          AKEYCODE_TAB
#define KEY_Back_Space   AKEYCODE_DEL
#define KEY_Print        AKEYCODE_SYSRQ
#define KEY_Scroll_Lock  AKEYCODE_SCROLL_LOCK
#define KEY_Pause        AKEYCODE_BREAK
#define KEY_Num_Lock     AKEYCODE_NUM_LOCK
#define KEY_Bracket_L    AKEYCODE_LEFT_BRACKET
#define KEY_Bracket_R    AKEYCODE_RIGHT_BRACKET
#define KEY_Minus        AKEYCODE_MINUS
#define KEY_Equal        AKEYCODE_EQUALS
#define KEY_Backslash    AKEYCODE_BACKSLASH
#define KEY_Semicolon    AKEYCODE_SEMICOLON
#define KEY_Apostrophe   AKEYCODE_APOSTROPHE
#define KEY_Comma        AKEYCODE_COMMA
#define KEY_Period       AKEYCODE_PERIOD
#define KEY_Slash        AKEYCODE_SLASH

#define KEY_Left         AKEYCODE_DPAD_LEFT
#define KEY_Right        AKEYCODE_DPAD_RIGHT
#define KEY_Up           AKEYCODE_DPAD_UP
#define KEY_Down         AKEYCODE_DPAD_DOWN
#define KEY_Insert       AKEYCODE_INSERT
#define KEY_Delete       AKEYCODE_FORWARD_DEL
#define KEY_Home         AKEYCODE_MOVE_HOME
#define KEY_End          AKEYCODE_MOVE_END
#define KEY_Page_Up      AKEYCODE_PAGE_UP
#define KEY_Page_Down    AKEYCODE_PAGE_DOWN

#define KEY_KP_Up        AKEYCODE_NUMPAD_8
#define KEY_KP_Begin     AKEYCODE_NUMPAD_5
#define KEY_KP_Left      AKEYCODE_NUMPAD_4
#define KEY_KP_Right     AKEYCODE_NUMPAD_6
#define KEY_KP_Home      AKEYCODE_NUMPAD_7
#define KEY_KP_Down      AKEYCODE_NUMPAD_2
#define KEY_KP_Page_Up   AKEYCODE_NUMPAD_9
#define KEY_KP_Page_Down AKEYCODE_NUMPAD_3
#define KEY_KP_End       AKEYCODE_NUMPAD_1
#define KEY_KP_Insert    AKEYCODE_NUMPAD_0
#define KEY_KP_Delete    AKEYCODE_NUMPAD_DOT

#define KEY_KP_Enter     AKEYCODE_NUMPAD_ENTER
#define KEY_KP_Divide    AKEYCODE_NUMPAD_DIVIDE
#define KEY_KP_Multiply  AKEYCODE_NUMPAD_MULTIPLY
#define KEY_KP_Add       AKEYCODE_NUMPAD_ADD
#define KEY_KP_Separator AKEYCODE_NUMPAD_COMMA
#define KEY_KP_Subtract  AKEYCODE_NUMPAD_SUBTRACT
#define KEY_KP_Decimal   AKEYCODE_NUMPAD_DOT
#define KEY_KP_Equal     AKEYCODE_NUMPAD_EQUALS

#define KEY_Caps_Lock    AKEYCODE_CAPS_LOCK
#define KEY_Shift_L      AKEYCODE_SHIFT_LEFT
#define KEY_Shift_R      AKEYCODE_SHIFT_RIGHT
#define KEY_Control_L    AKEYCODE_CTRL_LEFT
#define KEY_Control_R    AKEYCODE_CTRL_RIGHT
#define KEY_Alt_L        AKEYCODE_ALT_LEFT
#define KEY_Alt_R        AKEYCODE_ALT_RIGHT
#define KEY_Meta_L       AKEYCODE_META_L
#define KEY_Meta_R       AKEYCODE_META_R

#define KEY_1       AKEYCODE_1
#define KEY_2       AKEYCODE_2
#define KEY_3       AKEYCODE_3
#define KEY_4       AKEYCODE_4
#define KEY_5       AKEYCODE_5
#define KEY_6       AKEYCODE_6
#define KEY_7       AKEYCODE_7
#define KEY_8       AKEYCODE_8
#define KEY_9       AKEYCODE_9
#define KEY_0       AKEYCODE_0

#define KEY_a       AKEYCODE_A
#define KEY_b       AKEYCODE_B
#define KEY_c       AKEYCODE_C
#define KEY_d       AKEYCODE_D
#define KEY_e       AKEYCODE_E
#define KEY_f       AKEYCODE_F
#define KEY_g       AKEYCODE_G
#define KEY_h       AKEYCODE_H
#define KEY_i       AKEYCODE_I
#define KEY_j       AKEYCODE_J
#define KEY_k       AKEYCODE_K
#define KEY_l       AKEYCODE_L
#define KEY_m       AKEYCODE_M
#define KEY_n       AKEYCODE_N
#define KEY_o       AKEYCODE_O
#define KEY_p       AKEYCODE_P
#define KEY_q       AKEYCODE_Q
#define KEY_r       AKEYCODE_R
#define KEY_s       AKEYCODE_S
#define KEY_t       AKEYCODE_T
#define KEY_u       AKEYCODE_U
#define KEY_v       AKEYCODE_V
#define KEY_w       AKEYCODE_W
#define KEY_x       AKEYCODE_X
#define KEY_y       AKEYCODE_Y
#define KEY_z       AKEYCODE_Z

#define KEY_F1      AKEYCODE_F1
#define KEY_F2      AKEYCODE_F2
#define KEY_F3      AKEYCODE_F3
#define KEY_F4      AKEYCODE_F4
#define KEY_F5      AKEYCODE_F5
#define KEY_F6      AKEYCODE_F6
#define KEY_F7      AKEYCODE_F7
#define KEY_F8      AKEYCODE_F8
#define KEY_F9      AKEYCODE_F9
#define KEY_F10     AKEYCODE_F10
#define KEY_F11     AKEYCODE_F11
#define KEY_F12     AKEYCODE_F12
#define KEY_F13     AKEYCODE_F13
//#define KEY_F14     AKEYCODE_F14
//#define KEY_F15     AKEYCODE_F15

#define KEY_Back        AKEYCODE_BACK
#define KEY_Volume_Up   AKEYCODE_VOLUME_UP
#define KEY_Volume_Down AKEYCODE_VOLUME_DOWN
#define KEY_Power       AKEYCODE_POWER
#define KEY_Help        AKEYCODE_HELP


#define KEY_ASCII(ev)       glv_ascii(ev)


#endif //GLVIEW_KEYS_H
