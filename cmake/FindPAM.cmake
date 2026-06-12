# FindPAM.cmake - Find PAM library
# Uses pkg-config or traditional library search

find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(PC_PAM QUIET pam)
endif()

find_path(PAM_INCLUDE_DIR
    NAMES security/pam_appl.h pam/pam_appl.h
    HINTS ${PC_PAM_INCLUDE_DIRS} /usr/include /usr/local/include
)

find_library(PAM_LIBRARY
    NAMES pam pam32
    HINTS ${PC_PAM_LIBRARY_DIRS} /usr/lib /usr/local/lib /usr/lib64
)

find_library(PAM_MISC_LIBRARY
    NAMES pam_misc pam_misc32
    HINTS ${PC_PAM_LIBRARY_DIRS} /usr/lib /usr/local/lib /usr/lib64
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(PAM
    REQUIRED_VARS PAM_LIBRARY PAM_INCLUDE_DIR
)

if(PAM_FOUND)
    set(PAM_INCLUDE_DIRS ${PAM_INCLUDE_DIR})
    set(PAM_LIBRARIES ${PAM_LIBRARY})
    if(PAM_MISC_LIBRARY)
        list(APPEND PAM_LIBRARIES ${PAM_MISC_LIBRARY})
    endif()

    # Create imported target
    if(NOT TARGET PAM::PAM)
        add_library(PAM::PAM UNKNOWN IMPORTED)
        set_target_properties(PAM::PAM PROPERTIES
            IMPORTED_LOCATION "${PAM_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${PAM_INCLUDE_DIRS}"
        )
    endif()

    mark_as_advanced(PAM_INCLUDE_DIR PAM_LIBRARY PAM_MISC_LIBRARY)
endif()
