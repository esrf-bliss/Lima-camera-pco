# Set path to additional cmake scripts
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})

find_package(Threads REQUIRED)
find_package(Tango REQUIRED)
find_package(OmniORB REQUIRED)
find_package(ZeroMQ REQUIRED)

add_executable(PcoDetector
    LimaDetector.cpp
    LimaDetectorClass.cpp
    LimaDetectorStateMachine.cpp
    ClassFactory.cpp
    main.cpp
)

target_link_libraries(PcoDetector
    Threads::Threads
    libzmq
    OmniORB::OmniORB
    Tango::Tango
    pco
)

set_property(TARGET PcoDetector PROPERTY CXX_STANDARD 14)
