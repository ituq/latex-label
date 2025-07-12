cmake_minimum_required(VERSION 3.28.3)
project(Test)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
project(Chat)

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

include_directories(${Qt6Core_INCLUDE_DIRS} ${Qt6Gui_INCLUDE_DIRS})
include_directories(${PROJECT_SOURCE_DIR}/include)

include_directories(${PROJECT_SOURCE_DIR}/include/MicroTeX/src/)

# Copy MicroTeX resources - IMPORTANT for font/symbol loading
file(COPY ${PROJECT_SOURCE_DIR}/include/MicroTeX/res DESTINATION ${CMAKE_BINARY_DIR})

file(GLOB LatexSources
    # atom folder
    include/MicroTex/src/atom/atom_basic.cpp
    include/MicroTex/src/atom/atom_char.cpp
    include/MicroTex/src/atom/atom_impl.cpp
    include/MicroTex/src/atom/atom_matrix.cpp
    include/MicroTex/src/atom/atom_row.cpp
    include/MicroTex/src/atom/atom_space.cpp
    include/MicroTex/src/atom/colors_def.cpp
    include/MicroTex/src/atom/unit_conversion.cpp
    # box folder
    include/MicroTex/src/box/box.cpp
    include/MicroTex/src/box/box_factory.cpp
    include/MicroTex/src/box/box_group.cpp
    include/MicroTex/src/box/box_single.cpp
    # core folder
    include/MicroTex/src/core/core.cpp
    include/MicroTex/src/core/formula.cpp
    include/MicroTex/src/core/formula_def.cpp
    include/MicroTex/src/core/glue.cpp
    include/MicroTex/src/core/localized_num.cpp
    include/MicroTex/src/core/macro.cpp
    include/MicroTex/src/core/macro_def.cpp
    include/MicroTex/src/core/macro_impl.cpp
    include/MicroTex/src/core/parser.cpp
    # fonts folder
    include/MicroTex/src/fonts/alphabet.cpp
    include/MicroTex/src/fonts/font_basic.cpp
    include/MicroTex/src/fonts/font_info.cpp
    include/MicroTex/src/fonts/fonts.cpp
    # utils folder
    include/MicroTex/src/utils/string_utils.cpp
    include/MicroTex/src/utils/utf.cpp
    include/MicroTex/src/utils/utils.cpp
    # res folder
    include/MicroTex/src/res/builtin/formula_mappings.res.cpp
    include/MicroTex/src/res/builtin/symbol_mapping.res.cpp
    include/MicroTex/src/res/builtin/tex_param.res.cpp
    include/MicroTex/src/res/builtin/tex_symbols.res.cpp
    include/MicroTex/src/res/font/bi10.def.cpp
    include/MicroTex/src/res/font/bx10.def.cpp
    include/MicroTex/src/res/font/cmbsy10.def.cpp
    include/MicroTex/src/res/font/cmbx10.def.cpp
    include/MicroTex/src/res/font/cmbxti10.def.cpp
    include/MicroTex/src/res/font/cmex10.def.cpp
    include/MicroTex/src/res/font/cmmi10.def.cpp
    include/MicroTex/src/res/font/cmmi10_unchanged.def.cpp
    include/MicroTex/src/res/font/cmmib10.def.cpp
    include/MicroTex/src/res/font/cmmib10_unchanged.def.cpp
    include/MicroTex/src/res/font/cmr10.def.cpp
    include/MicroTex/src/res/font/cmss10.def.cpp
    include/MicroTex/src/res/font/cmssbx10.def.cpp
    include/MicroTex/src/res/font/cmssi10.def.cpp
    include/MicroTex/src/res/font/cmsy10.def.cpp
    include/MicroTex/src/res/font/cmti10.def.cpp
    include/MicroTex/src/res/font/cmti10_unchanged.def.cpp
    include/MicroTex/src/res/font/cmtt10.def.cpp
    include/MicroTex/src/res/font/dsrom10.def.cpp
    include/MicroTex/src/res/font/eufb10.def.cpp
    include/MicroTex/src/res/font/eufm10.def.cpp
    include/MicroTex/src/res/font/i10.def.cpp
    include/MicroTex/src/res/font/moustache.def.cpp
    include/MicroTex/src/res/font/msam10.def.cpp
    include/MicroTex/src/res/font/msbm10.def.cpp
    include/MicroTex/src/res/font/r10.def.cpp
    include/MicroTex/src/res/font/r10_unchanged.def.cpp
    include/MicroTex/src/res/font/rsfs10.def.cpp
    include/MicroTex/src/res/font/sb10.def.cpp
    include/MicroTex/src/res/font/sbi10.def.cpp
    include/MicroTex/src/res/font/si10.def.cpp
    include/MicroTex/src/res/font/special.def.cpp
    include/MicroTex/src/res/font/ss10.def.cpp
    include/MicroTex/src/res/font/stmary10.def.cpp
    include/MicroTex/src/res/font/tt10.def.cpp
    include/MicroTex/src/res/parser/font_parser.cpp
    include/MicroTex/src/res/parser/formula_parser.cpp
    include/MicroTex/src/res/reg/builtin_font_reg.cpp
    include/MicroTex/src/res/reg/builtin_syms_reg.cpp
    include/MicroTex/src/res/sym/amsfonts.def.cpp
    include/MicroTex/src/res/sym/amssymb.def.cpp
    include/MicroTex/src/res/sym/base.def.cpp
    include/MicroTex/src/res/sym/stmaryrd.def.cpp
    include/MicroTex/src/res/sym/symspecial.def.cpp

    include/MicroTex/src/latex.cpp
    include/MicroTex/src/render.cpp

    include/MicroTex/src/platform/qt/graphic_qt.cpp
    include/MicroTeX/src/samples/qt_texwidget.cpp
)

# Exclude QML files from MOC processing to avoid Qt Quick dependencies
set_property(SOURCE include/MicroTeX/src/samples/qt_texqmlitem.h PROPERTY SKIP_AUTOMOC ON)
set_property(SOURCE include/MicroTeX/src/samples/qt_texqmlitem.cpp PROPERTY SKIP_AUTOMOC ON)
file(GLOB_RECURSE SOURCES "src/*.cpp")
file(GLOB_RECURSE HEADERS "include/*.h")


add_executable(Test ${SOURCES} ${LatexSources} ${HEADERS})

# Add compilation definitions for MicroTeX
target_compile_definitions(Test PRIVATE -DBUILD_QT)


# Configure macOS app bundle for Release builds
if(APPLE AND CMAKE_BUILD_TYPE STREQUAL "Release")
    set_target_properties(Test PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/Info.plist
        MACOSX_BUNDLE_BUNDLE_NAME "Chat"
    )


endif()
target_link_libraries(Test PRIVATE tinyxml2)

target_link_libraries(Test PRIVATE
    Qt6::Widgets
    Qt6::Network
    md4c::md4c
)
