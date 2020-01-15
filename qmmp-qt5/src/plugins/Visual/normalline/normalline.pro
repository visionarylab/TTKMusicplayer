include(../../plugins.pri)

TARGET = $$PLUGINS_PREFIX/Visual/normalline

include(../common/common.pri)

HEADERS += normalline.h \
           visualnormallinefactory.h
           
SOURCES += normalline.cpp \
           visualnormallinefactory.cpp

win32:{
    HEADERS += ../../../../src/qmmp/visual.h
    INCLUDEPATH += ./
}

unix{
    QMAKE_CLEAN = $$PLUGINS_PREFIX/Visual/libnormalline.so
    target.path = $$LIB_DIR/qmmp/Visual
    INSTALLS += target
}
