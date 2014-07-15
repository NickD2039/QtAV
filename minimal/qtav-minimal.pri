INCLUDEPATH *= $$PWD/../src $$PWD/../../ffmpeg-lgpl/include
win32: INCLUDEPATH *= $$PWD/../src/compat/msvc
DEFINES += __STDC_CONSTANT_MACROS

LIBS += -L$$PWD/lib
LIBS += -lQtAVd
