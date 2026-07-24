cmake_minimum_required(VERSION 3.16)
project(mdxinput LANGUAGES C CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC OFF)

find_package(Fooyin REQUIRED)

# Build mdxmini library
set(MDXMINI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/mdxmini")

add_library(mdxmini_static STATIC
    ${MDXMINI_DIR}/src/mdxmini.c
    ${MDXMINI_DIR}/src/mdx2151.c
    ${MDXMINI_DIR}/src/mdxmml_ym2151.c
    ${MDXMINI_DIR}/src/pdxfile.c
    ${MDXMINI_DIR}/src/mdxfile.c
    ${MDXMINI_DIR}/src/pcm8.c
    ${MDXMINI_DIR}/src/ym2151.c
    ${MDXMINI_DIR}/src/nlg.c
)

target_include_directories(mdxmini_static PUBLIC ${MDXMINI_DIR}/src)
set_target_properties(mdxmini_static PROPERTIES C_STANDARD 99)

# Create Fooyin plugin
create_fooyin_plugin(
    mdxinput
    JSON_IN mdxinput.json.in
    DEPENDS
        Fooyin::Core
        Fooyin::Gui
        mdxmini_static
    SOURCES
        mdxplugin.cpp
        mdxplugin.h
        mdxinput.cpp
        mdxinput.h
        mdxmini_wrapper.h
        mdxsettingswidget.cpp
        mdxsettingswidget.h
)
