project: "examples"

default [
    debug
    warn
    opengl
    objdir %obj
    win32 [
        include_from %../win32
        libs_from %../win32 {glv}
    ]
    unix [
        include_from %../x11
        libs_from %../x11 {glv}
        ;cflags {-Wno-unused-parameter}
    ]
    macx [
        include_from %../mac
        libs_from %../mac {glv}
    ]
]

exe "doc" [
    sources [ %doc.c ]
]

exe "window" [
    sources [ %window.c ]
]

exe "complete" [
    sources [ %complete.c ]
]

exe "es2" [
    sources [ %es2.c ]
]

;eof
