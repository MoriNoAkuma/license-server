QT += core gui widgets network

CONFIG += c++17 console
CONFIG -= app_bundle

SOURCES += tests.cpp ../server/license.cpp ../server/licensestore.cpp ../server/licenseserver.cpp ../client/licenseclient.cpp
HEADERS += ../server/license.h ../server/licensestore.h ../server/licenseserver.h ../client/licenseclient.h
