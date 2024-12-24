#!/usr/bin/boron -s
; Create separate headers for each OS.

android: "#ifdef ANDROID"
win32:   "#elif defined(_WIN32)"
x11:     "#else"

hdr: read/text %glv.h
out: make string! 8000

foreach os [android win32 x11] [
    clear out
    parse hdr [
        thru "GLV Library" it: (
            appair out slice hdr it join " for " os
        )
        some[
            it: to android :it thru android pa: to win32 :pa
                thru '^/' pw: to x11 :pw
                thru '^/' px: to "#endif" :px thru '^/' (
                appair out it get select [android pa  win32 pw  x11 px] os
            )
        ]
    ]
    append out next it
    ; write rejoin [%/tmp/glv_ to-file os %.h] out
    write join to-file os %/glv.h out
]
