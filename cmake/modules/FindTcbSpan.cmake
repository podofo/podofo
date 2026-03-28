include_guard()

# Locate header
find_path(TcbSpan_INCLUDE_DIR
    NAMES "tcb/span.hpp"
)

# Standard handling
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(TcbSpan
    REQUIRED_VARS TcbSpan_INCLUDE_DIR
)

# Create imported target
if (TcbSpan_FOUND AND NOT TARGET tcbspan::tcbspan)
    add_library(tcbspan::tcbspan INTERFACE IMPORTED)
    set_target_properties(tcbspan::tcbspan PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${TcbSpan_INCLUDE_DIR}"
    )
endif()