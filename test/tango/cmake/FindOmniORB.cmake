find_path(OMNIORB_INCLUDE_DIRS CORBA.h PATH_SUFFIXES omniORB4)
find_library(OMNIORB_LIBRARY omniORB4)
find_library(OMNIORB_THREAD_LIBRARY omnithread)
find_library(OMNIORB_DYNAMIC_LIBRARY omniDynamic4)
find_library(OMNIORB_COS_LIBRARY COS4)
find_library(OMNIORB_COS_DYNAMIC_LIBRARY COSDynamic4)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(OmniORB
    DEFAULT_MSG
    OMNIORB_LIBRARY
    OMNIORB_THREAD_LIBRARY
    OMNIORB_DYNAMIC_LIBRARY
    OMNIORB_COS_LIBRARY
    OMNIORB_COS_DYNAMIC_LIBRARY
    OMNIORB_INCLUDE_DIRS)

if(OMNIORB_FOUND)
    # Thread
    if(NOT TARGET OmniORB::Thread)
        add_library(OmniORB::Thread SHARED IMPORTED)
    endif()
    if(EXISTS "${OMNIORB_THREAD_LIBRARY}")
        set_target_properties(OmniORB::Thread PROPERTIES
            IMPORTED_LINK_INTERFACE_LANGUAGES "C"
            IMPORTED_LOCATION "${OMNIORB_THREAD_LIBRARY}")
    endif()
    # Dynamic
    if(NOT TARGET OmniORB::Dynamic)
        add_library(OmniORB::Dynamic SHARED IMPORTED)
    endif()
    if(EXISTS "${OMNIORB_DYNAMIC_LIBRARY}")
        set_target_properties(OmniORB::Dynamic PROPERTIES
            IMPORTED_LINK_INTERFACE_LANGUAGES "C"
            IMPORTED_LOCATION "${OMNIORB_DYNAMIC_LIBRARY}")
    endif()
    # COS
    if(NOT TARGET OmniORB::COS)
        add_library(OmniORB::COS SHARED IMPORTED)
    endif()
    if(EXISTS "${OMNIORB_COS_LIBRARY}")
        set_target_properties(OmniORB::COS PROPERTIES
            IMPORTED_LINK_INTERFACE_LANGUAGES "C"
            IMPORTED_LOCATION "${OMNIORB_COS_LIBRARY}")
    endif()
    # DynamicCOS
    if(NOT TARGET OmniORB::COSDynamic)
        add_library(OmniORB::COSDynamic SHARED IMPORTED)
    endif()
    if(EXISTS "${OMNIORB_COS_DYNAMIC_LIBRARY}")
        set_target_properties(OmniORB::COSDynamic PROPERTIES
            IMPORTED_LINK_INTERFACE_LANGUAGES "C"
            IMPORTED_LOCATION "${OMNIORB_COS_DYNAMIC_LIBRARY}")
    endif()
    # OmniORB
    if(NOT TARGET OmniORB::OmniORB)
        add_library(OmniORB::OmniORB SHARED IMPORTED)
        target_link_libraries(OmniORB::OmniORB INTERFACE OmniORB::Thread OmniORB::Dynamic OmniORB::COS OmniORB::COSDynamic)
    endif()
    if(OMNIORB_INCLUDE_DIRS)
        set_target_properties(OmniORB::OmniORB PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${OMNIORB_INCLUDE_DIRS}")
    endif()
    if(EXISTS "${OMNIORB_LIBRARY}")
        set_target_properties(OmniORB::OmniORB PROPERTIES
            IMPORTED_LINK_INTERFACE_LANGUAGES "C"
            IMPORTED_LOCATION "${OMNIORB_LIBRARY}")
    endif()
endif()
