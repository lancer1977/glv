#!/usr/bin/boron -s
; Concatenate win32 & x11 source files.

read-src: func [fn] [
    src: find read/text fn "#include"
]

write %glv.c rejoin [
{{
/*===========================================================================/

  GLV Library for Windows & X11
  Copyright (C) 2003-2024  Karl Robillard
  SPDX-License-Identifier: MIT

  Documentation is at https://wickedsmoke.codeberg.page/glv_doc/

/===========================================================================*/

#ifdef _WIN32
}}
read-src %win32/glv.c
"^/#else^/"
read-src %x11/glv.c
"#endif^/"
]
