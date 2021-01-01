QT       -= gui

TARGET = QLogger
TEMPLATE = lib
CONFIG += static
DEFINES += QT_DEPRECATED_WARNINGS

include(QLogger.pri)

QMAKE_CXXFLAGS += -std=c++14
