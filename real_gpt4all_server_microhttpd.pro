QT += core
QT -= gui

CONFIG += c++17
CONFIG += console
CONFIG -= app_bundle

TARGET = real_gpt4all_server_microhttpd

TEMPLATE = app

# Real GPT4All AI Server with microhttpd
SOURCES += real_gpt4all_server_microhttpd.cpp

# Include GPT4All backend paths
INCLUDEPATH += ./gpt4all-backend/include
INCLUDEPATH += ./gpt4all-backend/include/gpt4all-backend

# Link with microhttpd
LIBS += -lmicrohttpd

# Link with GPT4All backend - use the actual built library
LIBS += -L./build/bin -lllmodel
LIBS += -Wl,-rpath,./build/bin

# Additional system libraries for real AI inference
LIBS += -ldl -lpthread

# Compiler flags for real AI integration
QMAKE_CXXFLAGS += -fPIC
QMAKE_CXXFLAGS += -Wall -Wextra

# Define GPT4ALL_BACKEND for real inference
DEFINES += GPT4ALL_BACKEND

# For debug builds, add more verbose output
CONFIG(debug, debug|release) {
    QMAKE_CXXFLAGS += -g -O0
    DEFINES += QT_QML_DEBUG
}

# For release builds, optimize
CONFIG(release, debug|release) {
    QMAKE_CXXFLAGS += -O2
}
