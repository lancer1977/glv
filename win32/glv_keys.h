#ifndef GLVIEW_KEYS_H
#define GLVIEW_KEYS_H
/*===========================================================================/

  GLView Keys for Windows
  Copyright (C) 2003-2006 Karl Robillard

  These defines and macros are for GLV_EVENT_KEY_DOWN/UP events.

/===========================================================================*/


#include <windows.h>


#define KEY_Grave           VK_OEM_3
#define KEY_Escape          VK_ESCAPE
#define KEY_Space           VK_SPACE
#define KEY_Return          VK_RETURN
#define KEY_Tab             VK_TAB
#define KEY_Back_Space      VK_BACK
#define KEY_Print           VK_PRINT
#define KEY_Scroll_Lock     VK_SCROLL
#define KEY_Pause           VK_PAUSE
#define KEY_Num_Lock        VK_NUMLOCK
#define KEY_Bracket_L       VK_OEM_4
#define KEY_Bracket_R       VK_OEM_6
#define KEY_Minus           VK_OEM_MINUS
#define KEY_Equal           VK_OEM_PLUS
#define KEY_Backslash       VK_OEM_5
#define KEY_Semicolon       VK_OEM_1
#define KEY_Apostrophe      VK_OEM_7
#define KEY_Comma           VK_OEM_COMMA
#define KEY_Period          VK_OEM_PERIOD
#define KEY_Slash           VK_OEM_2

#define KEY_Left            VK_LEFT
#define KEY_Right           VK_RIGHT
#define KEY_Up              VK_UP
#define KEY_Down            VK_DOWN
#define KEY_Insert          VK_INSERT
#define KEY_Delete          VK_DELETE
#define KEY_Home            VK_HOME
#define KEY_End             VK_END
#define KEY_Page_Up         VK_PRIOR
#define KEY_Page_Down       VK_NEXT

#define KEY_KP_Up           VK_NUMPAD8
#define KEY_KP_Begin        VK_NUMPAD5
#define KEY_KP_Left         VK_NUMPAD4
#define KEY_KP_Right        VK_NUMPAD6
#define KEY_KP_Home         VK_NUMPAD7
#define KEY_KP_Down         VK_NUMPAD2
#define KEY_KP_Page_Up      VK_NUMPAD9
#define KEY_KP_Page_Down    VK_NUMPAD3
#define KEY_KP_End          VK_NUMPAD1
#define KEY_KP_Insert       VK_NUMPAD0
#define KEY_KP_Delete       VK_DECIMAL
#define KEY_KP_Enter        0xc1	/* There is no VK_ for this */
#define KEY_KP_Divide       VK_DIVIDE
#define KEY_KP_Multiply     VK_MULTIPLY
#define KEY_KP_Add          VK_ADD
#define KEY_KP_Separator    VK_SEPARATOR
#define KEY_KP_Subtract     VK_SUBTRACT
#define KEY_KP_Decimal      VK_DECIMAL
#define KEY_KP_Equal        0   	/* ? */

#define KEY_Caps_Lock       VK_CAPITAL
#define KEY_Shift_L         VK_LSHIFT
#define KEY_Shift_R         VK_RSHIFT
#define KEY_Control_L       VK_LCONTROL
#define KEY_Control_R       VK_RCONTROL
#define KEY_Alt_L           VK_LMENU
#define KEY_Alt_R           VK_RMENU
#define KEY_Meta_L          VK_LWIN
#define KEY_Meta_R          VK_RWIN

#define KEY_1       '1'
#define KEY_2       '2'
#define KEY_3       '3'
#define KEY_4       '4'
#define KEY_5       '5'
#define KEY_6       '6'
#define KEY_7       '7'
#define KEY_8       '8'
#define KEY_9       '9'
#define KEY_0       '0'

#define KEY_a       'A'
#define KEY_b       'B'
#define KEY_c       'C'
#define KEY_d       'D'
#define KEY_e       'E'
#define KEY_f       'F'
#define KEY_g       'G'
#define KEY_h       'H'
#define KEY_i       'I'
#define KEY_j       'J'
#define KEY_k       'K'
#define KEY_l       'L'
#define KEY_m       'M'
#define KEY_n       'N'
#define KEY_o       'O'
#define KEY_p       'P'
#define KEY_q       'Q'
#define KEY_r       'R'
#define KEY_s       'S'
#define KEY_t       'T'
#define KEY_u       'U'
#define KEY_v       'V'
#define KEY_w       'W'
#define KEY_x       'X'
#define KEY_y       'Y'
#define KEY_z       'Z'

#define KEY_F1      VK_F1
#define KEY_F2      VK_F2
#define KEY_F3      VK_F3
#define KEY_F4      VK_F4
#define KEY_F5      VK_F5
#define KEY_F6      VK_F6
#define KEY_F7      VK_F7
#define KEY_F8      VK_F8
#define KEY_F9      VK_F9
#define KEY_F10     VK_F10
#define KEY_F11     VK_F11
#define KEY_F12     VK_F12
#define KEY_F13     VK_F13
#define KEY_F14     VK_F14
#define KEY_F15     VK_F15


#endif //GLVIEW_KEYS_H
