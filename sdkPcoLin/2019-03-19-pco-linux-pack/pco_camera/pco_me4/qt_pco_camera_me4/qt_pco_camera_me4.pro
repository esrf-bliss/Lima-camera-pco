TEMPLATE = app
TARGET = qt_pco_camera_me4
DESTDIR = ../bin
QT += core widgets gui

DEFINES += QT_WIDGETS_LIB
INCLUDEPATH += ./GeneratedFiles \
    . \
    ../../pco_common/pco_include \
    ../../pco_common/pco_classes \
    ../../pco_common/qt_pco_camera \
    ./../pco_classes \
    $(SISODIR5)/include

DEPENDPATH += .
MOC_DIR += ./GeneratedFiles/moc
OBJECTS_DIR += objects
UI_DIR += ./GeneratedFiles
RCC_DIR += ./GeneratedFiles
include(qt_pco_camera_me4.pri)

DEPENDPATH += $$PWD/../../pco_common/pco_lib

unix:!macx: LIBS += -L$(SISODIR5)/lib64/ -lclsersis -lfglib5

unix:!macx: LIBS += -L$$PWD/../../pco_common/pco_lib/ -lpcocnv -lpcofile -lpcolog -lreorderfunc -lpcocam_me4

unix:!macx: PRE_TARGETDEPS += $$PWD/../../pco_common/pco_lib/libpcocnv.a

unix:!macx: PRE_TARGETDEPS += $$PWD/../../pco_common/pco_lib/libpcofile.a

unix:!macx: PRE_TARGETDEPS += $$PWD/../../pco_common/pco_lib/libpcolog.a

unix:!macx: PRE_TARGETDEPS += $$PWD/../../pco_common/pco_lib/libreorderfunc.a

unix:!macx: PRE_TARGETDEPS += $$PWD/../../pco_common/pco_lib/libpcocam_me4.a
