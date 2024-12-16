find_path(TANGO_INCLUDE_DIRS tango.h PATH_SUFFIXES tango)
find_library(TANGO_LIBRARY tango)
#find_library(LOG4TANGO_LIBRARY log4tango)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Tango
    DEFAULT_MSG
    TANGO_LIBRARY
    #LOG4TANGO_LIBRARY
    TANGO_INCLUDE_DIRS)

#set (TANGO_LIBRARIES ${TANGO_LIBRARY} ${LOG4TANGO_LIBRARY})

if(TANGO_FOUND)
    # Log4Tango
    if(NOT TARGET Tango::TangoLogging)
        add_library(Tango::TangoLogging SHARED IMPORTED)
    endif()
    if(EXISTS "${TANGO_LIBRARY}")
        set_target_properties(Tango::TangoLogging PROPERTIES
            IMPORTED_LINK_INTERFACE_LANGUAGES "C"
            IMPORTED_LOCATION "${LOG4TANGO_LIBRARY}")
    endif()
    # Tango
    if(NOT TARGET Tango::Tango)
        add_library(Tango::Tango SHARED IMPORTED)
        #target_link_libraries(Tango::Tango INTERFACE Tango::TangoLogging)
    endif()
    if(TANGO_INCLUDE_DIRS)
        set_target_properties(Tango::Tango PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${TANGO_INCLUDE_DIRS}")
    endif()
    if(EXISTS "${TANGO_LIBRARY}")
        set_target_properties(Tango::Tango PROPERTIES
            IMPORTED_LINK_INTERFACE_LANGUAGES "C"
            IMPORTED_LOCATION "${TANGO_LIBRARY}")
    endif()
endif()
