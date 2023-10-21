#ifndef GLVIEW_KEYS_H
#define GLVIEW_KEYS_H
/*===========================================================================/

  GLView Keys for X11
  Copyright (C) 2003-2006 Karl Robillard

  These defines and macros are for GLV_EVENT_KEY_DOWN/UP events.

/===========================================================================*/


#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
//#include <X11/XF86keysym.h>


#define KEY_Grave        XK_grave
#define KEY_Escape       XK_Escape
#define KEY_Space        XK_space
#define KEY_Return       XK_Return
#define KEY_Tab          XK_Tab
#define KEY_Back_Space   XK_BackSpace
#define KEY_Print        XK_Print
#define KEY_Scroll_Lock  XK_Scroll_Lock
#define KEY_Pause        XK_Pause
#define KEY_Num_Lock     XK_Num_Lock
#define KEY_Bracket_L    XK_bracketleft
#define KEY_Bracket_R    XK_bracketright
#define KEY_Minus        XK_minus
#define KEY_Equal        XK_equal
#define KEY_Backslash    XK_backslash
#define KEY_Semicolon    XK_semicolon
#define KEY_Apostrophe   XK_apostrophe
#define KEY_Comma        XK_comma
#define KEY_Period       XK_period
#define KEY_Slash        XK_slash

#define KEY_Left         XK_Left
#define KEY_Right        XK_Right
#define KEY_Up           XK_Up
#define KEY_Down         XK_Down
#define KEY_Insert       XK_Insert
#define KEY_Delete       XK_Delete
#define KEY_Home         XK_Home
#define KEY_End          XK_End
#define KEY_Page_Up      XK_Page_Up
#define KEY_Page_Down    XK_Page_Down

#define KEY_KP_Up        XK_KP_Up
#define KEY_KP_Begin     XK_KP_Begin
#define KEY_KP_Left      XK_KP_Left
#define KEY_KP_Right     XK_KP_Right
#define KEY_KP_Home      XK_KP_Home
#define KEY_KP_Down      XK_KP_Down
#define KEY_KP_Page_Up   XK_KP_Page_Up
#define KEY_KP_Page_Down XK_KP_Page_Down
#define KEY_KP_End       XK_KP_End
#define KEY_KP_Insert    XK_KP_Insert
#define KEY_KP_Delete    XK_KP_Delete
#define KEY_KP_Enter     XK_KP_Enter
#define KEY_KP_Divide    XK_KP_Divide
#define KEY_KP_Multiply  XK_KP_Multiply
#define KEY_KP_Add       XK_KP_Add
#define KEY_KP_Separator XK_KP_Separator
#define KEY_KP_Subtract  XK_KP_Subtract
#define KEY_KP_Decimal   XK_KP_Decimal
#define KEY_KP_Equal     XK_KP_Equal

#define KEY_Caps_Lock    XK_Caps_Lock
#define KEY_Shift_L      XK_Shift_L
#define KEY_Shift_R      XK_Shift_R
#define KEY_Control_L    XK_Control_L
#define KEY_Control_R    XK_Control_R
#define KEY_Alt_L        XK_Alt_L
#define KEY_Alt_R        XK_Alt_R
#define KEY_Meta_L       XK_Meta_L
#define KEY_Meta_R       XK_Meta_R

#define KEY_1       XK_1
#define KEY_2       XK_2
#define KEY_3       XK_3
#define KEY_4       XK_4
#define KEY_5       XK_5
#define KEY_6       XK_6
#define KEY_7       XK_7
#define KEY_8       XK_8
#define KEY_9       XK_9
#define KEY_0       XK_0

#define KEY_a       XK_a
#define KEY_b       XK_b
#define KEY_c       XK_c
#define KEY_d       XK_d
#define KEY_e       XK_e
#define KEY_f       XK_f
#define KEY_g       XK_g
#define KEY_h       XK_h
#define KEY_i       XK_i
#define KEY_j       XK_j
#define KEY_k       XK_k
#define KEY_l       XK_l
#define KEY_m       XK_m
#define KEY_n       XK_n
#define KEY_o       XK_o
#define KEY_p       XK_p
#define KEY_q       XK_q
#define KEY_r       XK_r
#define KEY_s       XK_s
#define KEY_t       XK_t
#define KEY_u       XK_u
#define KEY_v       XK_v
#define KEY_w       XK_w
#define KEY_x       XK_x
#define KEY_y       XK_y
#define KEY_z       XK_z

#define KEY_F1      XK_F1
#define KEY_F2      XK_F2
#define KEY_F3      XK_F3
#define KEY_F4      XK_F4
#define KEY_F5      XK_F5
#define KEY_F6      XK_F6
#define KEY_F7      XK_F7
#define KEY_F8      XK_F8
#define KEY_F9      XK_F9
#define KEY_F10     XK_F10
#define KEY_F11     XK_F11
#define KEY_F12     XK_F12
#define KEY_F13     XK_F13
#define KEY_F14     XK_F14
#define KEY_F15     XK_F15

//#define KEY_Volume_Up   XF86XK_AudioLowerVolume
//#define KEY_Volume_Down XF86XK_AudioLowerVolume
//#define KEY_Power       XF86XK_PowerOff
#define KEY_Help    XK_Help


#endif //GLVIEW_KEYS_H
