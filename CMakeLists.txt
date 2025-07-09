cmake_minimum_required(VERSION 3.28.3)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
project(Chat)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

find_package(Qt6 REQUIRED COMPONENTS Widgets Network)

include_directories(${Qt6Core_INCLUDE_DIRS} ${Qt6Gui_INCLUDE_DIRS})
include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${MD4C_INCLUDE_DIRS})


file(GLOB_RECURSE SOURCES "src/*.cpp")
file(GLOB_RECURSE HEADERS "include/*.h")


add_executable(Test ${SOURCES} ${HEADERS})

# Configure macOS app bundle for Release builds
if(APPLE AND CMAKE_BUILD_TYPE STREQUAL "Release")
    set_target_properties(Test PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/Info.plist
        MACOSX_BUNDLE_BUNDLE_NAME "Chat"
    )

    # This allows the app to be selfe contained, not needed for us
    # add_custom_command(TARGET chat POST_BUILD
    #    COMMAND macdeployqt chat.app
    #    COMMAND ${CMAKE_COMMAND} -E make_directory chat.app/Contents/Frameworks
    #    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:md4c::md4c> chat.app/Contents/Frameworks/
    #    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:md4c::md4c-html> chat.app/Contents/Frameworks/
    #    COMMAND install_name_tool -change $<TARGET_FILE:md4c::md4c> @executable_path/../Frameworks/$<TARGET_FILE_NAME:md4c::md4c> chat.app/Contents/MacOS/chat
    #    COMMAND install_name_tool -change $<TARGET_FILE:md4c::md4c-html> @executable_path/../Frameworks/$<TARGET_FILE_NAME:md4c::md4c-html> chat.app/Contents/MacOS/chat
    #    COMMAND install_name_tool -change $<TARGET_FILE:md4c::md4c> @loader_path/$<TARGET_FILE_NAME:md4c::md4c> chat.app/Contents/Frameworks/$<TARGET_FILE_NAME:md4c::md4c-html>
    #    COMMAND codesign --force --deep --sign - chat.app
    #    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    #)

endif()

target_link_libraries(Test PRIVATE
    Qt6::Widgets
    Qt6::Network
)
