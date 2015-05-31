TARGET = 3FD
TEMPLATE = lib
CONFIG += staticlib
CONFIG -= qt
CONFIG += c++11

win32:CONFIG(release, debug|release): DEFINES += NDEBUG
else:unix:!macx:CONFIG(release, debug|release): DEFINES += NDEBUG

DEFINES += \
    ENABLE_3FD_CST \
    ENABLE_3FD_ERR_IMPL_DETAILS

SOURCES += \
    callstacktracer.cpp \
    configuration.cpp \
    dependencies.cpp \
    exceptions.cpp \
    gc_addresseshashtable.cpp \
    gc_garbagecollector.cpp \
    gc_mastertable.cpp \
    gc_memblock.cpp \
    gc_messages.cpp \
    logger.cpp \
    logger_poco.cpp \
    opencl_impl.cpp \
    opencl_impl_commandtracker.cpp \
    opencl_impl_context.cpp \
    opencl_impl_device.cpp \
    opencl_impl_platform.cpp \
    opencl_impl_program.cpp \
    opencl_impl_programmanifest.cpp \
    runtime.cpp \
    sqlite_databaseconn.cpp \
    sqlite_dbconnpool.cpp \
    sqlite_prepstatement.cpp \
    sqlite_transaction.cpp \
    utils.cpp \
    sqlite3.c

HEADERS += \
    base.h \
    callstacktracer.h \
    configuration.h \
    dependencies.h \
    exceptions.h \
    gc.h \
    gc_memblock.h \
    gc_messages.h \
    logger.h \
    multilayerctnr.h \
    opencl.h \
    opencl_impl.h \
    preprocessing.h \
    runtime.h \
    sptr.h \
    sqlite.h \
    sqlite3.h \
    utils.h

unix {
    target.path = /usr/local/lib
    INSTALLS += target
    CONFIG(release, debug|release): DEFINES += NDEBUG
}

OTHER_FILES += \
    ReadMe.txt \
    config3fd-template.xml

INCLUDEPATH += \
    ../OpenCL \
    /opt/Poco-1.4.7/include \
    /opt/boost-1.55/include

DEPENDPATH += /opt/Poco-1.4.7/include
