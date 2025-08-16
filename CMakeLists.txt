cmake_minimum_required(VERSION 3.28.3)
project(LatexLabel VERSION 1.0.0)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Build options
option(BUILD_EXAMPLES "Build example applications" ON)
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)

# Fetch MicroTeX using FetchContent
include(FetchContent)

FetchContent_Declare(
    MicroTeX
    GIT_REPOSITORY https://github.com/NanoMichael/MicroTeX.git
    GIT_TAG master
)

# Make MicroTeX available
FetchContent_MakeAvailable(MicroTeX)

find_package(PkgConfig REQUIRED)
find_package(md4c CONFIG REQUIRED)

pkg_check_modules(tinyxml2 REQUIRED IMPORTED_TARGET tinyxml2)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

find_package(Qt6 REQUIRED COMPONENTS Widgets Network)
find_package(Fontconfig REQUIRED)

# Copy MicroTeX resources - IMPORTANT for font/symbol loading
file(COPY ${microtex_SOURCE_DIR}/res DESTINATION ${CMAKE_BINARY_DIR})

file(GLOB LatexSources
    # atom folder
    ${microtex_SOURCE_DIR}/src/atom/atom_basic.cpp
    ${microtex_SOURCE_DIR}/src/atom/atom_char.cpp
    ${microtex_SOURCE_DIR}/src/atom/atom_impl.cpp
    ${microtex_SOURCE_DIR}/src/atom/atom_matrix.cpp
    ${microtex_SOURCE_DIR}/src/atom/atom_row.cpp
    ${microtex_SOURCE_DIR}/src/atom/atom_space.cpp
    ${microtex_SOURCE_DIR}/src/atom/colors_def.cpp
    ${microtex_SOURCE_DIR}/src/atom/unit_conversion.cpp
    # box folder
    ${microtex_SOURCE_DIR}/src/box/box.cpp
    ${microtex_SOURCE_DIR}/src/box/box_factory.cpp
    ${microtex_SOURCE_DIR}/src/box/box_group.cpp
    ${microtex_SOURCE_DIR}/src/box/box_single.cpp
    # core folder
    ${microtex_SOURCE_DIR}/src/core/core.cpp
    ${microtex_SOURCE_DIR}/src/core/formula.cpp
    ${microtex_SOURCE_DIR}/src/core/formula_def.cpp
    ${microtex_SOURCE_DIR}/src/core/glue.cpp
    ${microtex_SOURCE_DIR}/src/core/localized_num.cpp
    ${microtex_SOURCE_DIR}/src/core/macro.cpp
    ${microtex_SOURCE_DIR}/src/core/macro_def.cpp
    ${microtex_SOURCE_DIR}/src/core/macro_impl.cpp
    ${microtex_SOURCE_DIR}/src/core/parser.cpp
    # fonts folder
    ${microtex_SOURCE_DIR}/src/fonts/alphabet.cpp
    ${microtex_SOURCE_DIR}/src/fonts/font_basic.cpp
    ${microtex_SOURCE_DIR}/src/fonts/font_info.cpp
    ${microtex_SOURCE_DIR}/src/fonts/fonts.cpp
    # utils folder
    ${microtex_SOURCE_DIR}/src/utils/string_utils.cpp
    ${microtex_SOURCE_DIR}/src/utils/utf.cpp
    ${microtex_SOURCE_DIR}/src/utils/utils.cpp
    # res folder
    ${microtex_SOURCE_DIR}/src/res/builtin/formula_mappings.res.cpp
    ${microtex_SOURCE_DIR}/src/res/builtin/symbol_mapping.res.cpp
    ${microtex_SOURCE_DIR}/src/res/builtin/tex_param.res.cpp
    ${microtex_SOURCE_DIR}/src/res/builtin/tex_symbols.res.cpp
    ${microtex_SOURCE_DIR}/src/res/font/bi10.def.cpp
    ${microtex_SOURCE_DIR}/src/res/font/bx10.def.cpp
    ${microtex_SOURCE_DIR}/src/res/font/cmbsy10.def.cpp
    ${microtex_SOURCE_DIR}/src/res/font/cmbx10.def.cpp
    ${microtex_SOURCE_DIR}/src/res/font/cmbxti10.def.cpp
    ${microtex_SOURCE_DIR}/src/res/font/cmex10.def.cpp
    ${microtex_SOURCE_DIR}/src/res/font/cmmi10.def.cpp
    ${microtex_SOURCE_DIR}/src/res/font/cmmi10_unchanged.def.cpp
    ${microtex_SOURCE_DIR}/src/res/font/cmmib10.def.cpp
    ${microtex_SOURCE_DIR}/src/res/font/cmmib10_unchanged.def.cpp
    ${microtex_SOURCE_DIR}/src/res/font/cmr10.def.cpp
    ${microtex_SOURCE_DIR}/src/res/font/cmss10.def.cpp
    ${microtex_SOURCE_DIR}/src/res/font/cmssbx10.def.cpp
    ${microtex_SOURCE_DIR}/src/res/font/cmssi10.def.cpp
    ${microtex_SOURCE_DIR}/src/res/font/cmsy10.def.cpp
    ${microtex_SOURCE_DIR}/src/res/font/cmti10.def.cpp
    ${microtex_SOURCE_DIR}/src/res/font/cmti10_unchanged.def.cpp
    ${microtex_SOURCE_DIR}/src/res/font/cmtt10.def.cpp
    ${microtex_SOURCE_DIR}/src/res/font/dsrom10.def.cpp
    ${microtex_SOURCE_DIR}/src/res/font/eufb10.def.cpp
    ${microtex_SOURCE_DIR}/src/res/font/eufm10.def.cpp
    ${microtex_SOURCE_DIR}/src/res/font/i10.def.cpp
    ${microtex_SOURCE_DIR}/src/res/font/moustache.def.cpp
    ${microtex_SOURCE_DIR}/src/res/font/msam10.def.cpp
    ${microtex_SOURCE_DIR}/src/res/font/msbm10.def.cpp
    ${microtex_SOURCE_DIR}/src/res/font/r10.def.cpp
    ${microtex_SOURCE_DIR}/src/res/font/r10_unchanged.def.cpp
    ${microtex_SOURCE_DIR}/src/res/font/rsfs10.def.cpp
    ${microtex_SOURCE_DIR}/src/res/font/sb10.def.cpp
    ${microtex_SOURCE_DIR}/src/res/font/sbi10.def.cpp
    ${microtex_SOURCE_DIR}/src/res/font/si10.def.cpp
    ${microtex_SOURCE_DIR}/src/res/font/special.def.cpp
    ${microtex_SOURCE_DIR}/src/res/font/ss10.def.cpp
    ${microtex_SOURCE_DIR}/src/res/font/stmary10.def.cpp
    ${microtex_SOURCE_DIR}/src/res/font/tt10.def.cpp
    ${microtex_SOURCE_DIR}/src/res/parser/font_parser.cpp
    ${microtex_SOURCE_DIR}/src/res/parser/formula_parser.cpp
    ${microtex_SOURCE_DIR}/src/res/reg/builtin_font_reg.cpp
    ${microtex_SOURCE_DIR}/src/res/reg/builtin_syms_reg.cpp
    ${microtex_SOURCE_DIR}/src/res/sym/amsfonts.def.cpp
    ${microtex_SOURCE_DIR}/src/res/sym/amssymb.def.cpp
    ${microtex_SOURCE_DIR}/src/res/sym/base.def.cpp
    ${microtex_SOURCE_DIR}/src/res/sym/stmaryrd.def.cpp
    ${microtex_SOURCE_DIR}/src/res/sym/symspecial.def.cpp

    ${microtex_SOURCE_DIR}/src/latex.cpp
    ${microtex_SOURCE_DIR}/src/render.cpp

    ${microtex_SOURCE_DIR}/src/platform/qt/graphic_qt.cpp
)

# Get only MicroTeX headers for the library (exclude application-specific headers)
file(GLOB_RECURSE MICROTEX_HEADERS "${microtex_SOURCE_DIR}/src/*.h")
# Remove sample/example headers that might contain Q_OBJECT
list(FILTER MICROTEX_HEADERS EXCLUDE REGEX ".*samples.*")

# Application headers and sources
set(APPLICATION_HEADERS
    include/LatexLabel.h
    include/element.h
    include/CodeBlockWidget.h
)

set(APPLICATION_SOURCES
    src/LatexLabel.cpp
    src/element.cpp
    src/CodeBlockWidget.cpp
)

# Create the library target
add_library(latex-label ${LatexSources} ${MICROTEX_HEADERS} ${APPLICATION_HEADERS} ${APPLICATION_SOURCES})

# Set library properties
set_target_properties(latex-label PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
    PUBLIC_HEADER "${MICROTEX_HEADERS}"
)

# Include directories for the library
target_include_directories(latex-label PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${microtex_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:include>
    ${Qt6Core_INCLUDE_DIRS}
    ${Qt6Gui_INCLUDE_DIRS}
)

# Add compilation definitions for MicroTeX
target_compile_definitions(latex-label PUBLIC BUILD_QT)

# Link libraries to the library target
target_link_libraries(latex-label PUBLIC
    Qt6::Widgets
    Qt6::Network
    md4c::md4c
    PkgConfig::tinyxml2
)

# Configure sanitizers
if (SANITIZE)
    message(STATUS "Enabling sanitizers: ${SANITIZE}")

    # Tell CMake we want Clang; skip if you already set CC/CXX before calling CMake.
    set(CMAKE_C_COMPILER   clang   CACHE STRING "" FORCE)
    set(CMAKE_CXX_COMPILER clang++ CACHE STRING "" FORCE)

    # Common flags
    set(SAN_FLAGS "-fsanitize=${SANITIZE} -fno-omit-frame-pointer -g")
    # Compile flags
    target_compile_options(latex-label PRIVATE ${SAN_FLAGS} -O1)
    # Link flags
    target_link_options(latex-label PRIVATE ${SAN_FLAGS})

    # Sanitizers make most sense in Debug; force it for single-config
    if (NOT CMAKE_CONFIGURATION_TYPES)      # i.e. Ninja, Makefiles…
        set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "" FORCE)
    endif()
endif()

# Build examples if requested
if(BUILD_EXAMPLES)
    set(EXAMPLE_SOURCES src/main.cpp)

    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp")
        add_executable(test ${EXAMPLE_SOURCES})

        # Link the example to our library
        target_link_libraries(test PRIVATE latex-label)

        # Configure macOS app bundle for Release builds
        if(APPLE AND CMAKE_BUILD_TYPE STREQUAL "Release")
            set_target_properties(test PROPERTIES
                MACOSX_BUNDLE TRUE
                MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/Info.plist
                MACOSX_BUNDLE_BUNDLE_NAME "LatexLabelExample"
            )
        endif()

        # Apply sanitizers to example too
        if(SANITIZE)
            target_compile_options(test PRIVATE ${SAN_FLAGS} -O1)
        target_link_options(test PRIVATE ${SAN_FLAGS})
        endif()
    endif()
endif()
