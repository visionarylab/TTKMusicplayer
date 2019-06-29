#Common settings for Qmmp build

#Extra clean target

unix: QMAKE_DISTCLEAN += -r .build

#Some conf to redirect intermediate stuff in separate dirs

UI_DIR=./.build/ui/
MOC_DIR=./.build/moc/
OBJECTS_DIR=./.build/obj
RCC_DIR=./.build/rcc

#Defines

DEFINES += QT_NO_CAST_FROM_BYTEARRAY QT_STRICT_ITERATORS
QMAKE_CXXFLAGS += -std=c++11
DEFINES += QMMP_LIBRARY
#Configuration

CONFIG += hide_symbols

#Version

TTKMusicPlayer = 2.9.0.0
QMMP_VERSION = 0.12.3

#Comment/uncomment this if you want to change plugins list

CONFIG += JACK_PLUGIN
CONFIG += FLAC_PLUGIN
CONFIG += MUSEPACK_PLUGIN
CONFIG += FFAP_PLUGIN
CONFIG += FFMPEG_PLUGIN
CONFIG += MODPLUG_PLUGIN
CONFIG += WILDMIDI_PLUGIN
CONFIG += GME_PLUGIN
CONFIG += XMP_PLUGIN
CONFIG += OPUS_PLUGIN
CONFIG += ALSA_PLUGIN
CONFIG += AAC_PLUGIN
CONFIG += CDAUDIO_PLUGIN
CONFIG += BS2B_PLUGIN
CONFIG += SOXR_PLUGIN
CONFIG += LADSPA_PLUGIN
CONFIG += SRC_PLUGIN
CONFIG += SID_PLUGIN
CONFIG += OPTIMFROG_PLUGIN

#additional features

CONFIG += WITH_MAD
CONFIG += WITH_ENCA

CONFIG -= $$DISABLED_PLUGINS
