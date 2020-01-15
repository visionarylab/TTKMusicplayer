include(../../plugins.pri)

TARGET = $$PLUGINS_PREFIX/Visual/normalhistogram

include(../common/common.pri)

HEADERS += normalhistogram.h \
           visualnormalhistogramfactory.h

SOURCES += normalhistogram.cpp \
           visualnormalhistogramfactory.cpp

win32:{
    HEADERS += ../../../../src/qmmp/visual.h
    INCLUDEPATH += ./
}

unix{
    QMAKE_CLEAN = $$PLUGINS_PREFIX/Visual/libnormalhistogram.so
    target.path = $$LIB_DIR/qmmp/Visual
    INSTALLS += target
}
