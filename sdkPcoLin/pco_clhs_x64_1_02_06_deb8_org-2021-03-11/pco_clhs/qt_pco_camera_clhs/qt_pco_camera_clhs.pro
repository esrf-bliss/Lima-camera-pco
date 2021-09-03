TEMPLATE = app
TARGET = qt_pco_camera_clhs
DESTDIR = ../bin
QT += core widgets gui

DEFINES += QT_WIDGETS_LIB
INCLUDEPATH += ./GeneratedFiles \
    . \
    ../../pco_common/pco_include \
    ../../pco_common/pco_classes \
    ../../pco_common/qt_pco_camera \
    ./../pco_classes \
    ../pco_clhs_common

DEPENDPATH += .
MOC_DIR += ./GeneratedFiles/moc
OBJECTS_DIR += objects
UI_DIR += ./GeneratedFiles
RCC_DIR += ./GeneratedFiles
include(qt_pco_camera_clhs.pri)

DEPENDPATH += $$PWD/../../pco_common/pco_lib

unix:!macx: LD_LIBRARY_PATH += -L$${GENICAM_ROOT_V2_4}/bin/Linux64_x64/
unix:!macx: LIBS += -L/home/blaschke/siso_rt5_4_1_4/lib64
unix:!macx: LIBS += -L$$PWD/../../pco_common/pco_lib/ -lpcocnv -lpcofile -lpcolog -lpcocam_clhs -lpcoclhs -ldl

unix:!macx: PRE_TARGETDEPS += $$PWD/../../pco_common/pco_lib/libpcocnv.a

unix:!macx: PRE_TARGETDEPS += $$PWD/../../pco_common/pco_lib/libpcofile.a

unix:!macx: PRE_TARGETDEPS += $$PWD/../../pco_common/pco_lib/libpcolog.a

unix:!macx: PRE_TARGETDEPS += $$PWD/../../pco_common/pco_lib/libpcocam_clhs.a
