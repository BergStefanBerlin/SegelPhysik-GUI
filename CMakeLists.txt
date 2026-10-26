cmake_minimum_required(VERSION 3.25)
project(SegelPhysikGUI LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(SEGELPHYSIK_CORE_SOURCE_DIR
    "${CMAKE_CURRENT_SOURCE_DIR}/../CORE"
    CACHE PATH "Path to the SegelPhysik CORE checkout")
if(NOT EXISTS "${SEGELPHYSIK_CORE_SOURCE_DIR}/Code/CMakeLists.txt")
    message(FATAL_ERROR
        "CORE source not found. Set SEGELPHYSIK_CORE_SOURCE_DIR to its checkout.")
endif()

set(SEGELPHYSIK_MODELLE_DIR
    "${CMAKE_CURRENT_SOURCE_DIR}/../Modelle"
    CACHE PATH "Model folder (sibling of the CORE and GUI checkouts)")

find_package(Qt6 CONFIG REQUIRED COMPONENTS Widgets)
find_package(VTK CONFIG REQUIRED COMPONENTS
    CommonCore
    CommonDataModel
    CommonTransforms
    FiltersCore
    FiltersGeneral
    FiltersSources
    GUISupportQt
    InteractionStyle
    RenderingAnnotation
    RenderingCore
    RenderingFreeType
    RenderingOpenGL2)

add_subdirectory("${SEGELPHYSIK_CORE_SOURCE_DIR}/Code"
    "${CMAKE_CURRENT_BINARY_DIR}/segelphysik-core")

add_executable(segelphysik_gui WIN32
    Code/src/main.cpp
    Code/src/main_window.cpp
    Code/src/main_window.hpp
    Code/src/model_catalog.cpp
    Code/src/model_catalog.hpp
    Code/src/core_adapter.cpp
    Code/src/core_adapter.hpp
    Code/src/display_geometry.cpp
    Code/src/display_geometry.hpp
    Code/src/force_display.cpp
    Code/src/force_display.hpp
    Code/src/vtk_scene.cpp
    Code/src/vtk_scene.hpp)
target_include_directories(segelphysik_gui PRIVATE Code/src)
target_compile_definitions(segelphysik_gui PRIVATE
    "SEGELPHYSIK_DEFAULT_MODEL_DIR=\"${SEGELPHYSIK_MODELLE_DIR}\"")
target_link_libraries(segelphysik_gui PRIVATE
    Qt6::Widgets
    segelphysik_core
    VTK::CommonCore
    VTK::CommonDataModel
    VTK::CommonTransforms
    VTK::FiltersCore
    VTK::FiltersGeneral
    VTK::FiltersSources
    VTK::GUISupportQt
    VTK::InteractionStyle
    VTK::RenderingAnnotation
    VTK::RenderingCore
    VTK::RenderingFreeType
    VTK::RenderingOpenGL2)

vtk_module_autoinit(TARGETS segelphysik_gui MODULES
    VTK::RenderingOpenGL2
    VTK::RenderingFreeType
    VTK::GUISupportQt)

# Qt findet Plugins zur Laufzeit in <exe>/<plugintyp>; ohne qwindows startet die GUI nicht.
foreach(qt_plugin IN ITEMS
        "Qt6::QWindowsIntegrationPlugin|platforms"
        "Qt6::QModernWindowsStylePlugin|styles")
    string(REPLACE "|" ";" qt_plugin_parts "${qt_plugin}")
    list(GET qt_plugin_parts 0 qt_plugin_target)
    list(GET qt_plugin_parts 1 qt_plugin_dir)
    if(TARGET ${qt_plugin_target})
        add_custom_command(TARGET segelphysik_gui POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory
                "$<TARGET_FILE_DIR:segelphysik_gui>/${qt_plugin_dir}"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "$<TARGET_FILE:${qt_plugin_target}>"
                "$<TARGET_FILE_DIR:segelphysik_gui>/${qt_plugin_dir}/"
            VERBATIM)
    endif()
endforeach()

include(CTest)
if(BUILD_TESTING)
    add_executable(segelphysik_gui_core_tests
        Code/src/core_adapter.cpp
        Code/src/core_adapter.hpp
        Code/tests/core_adapter_tests.cpp)
    target_include_directories(segelphysik_gui_core_tests PRIVATE Code/src)
    target_link_libraries(segelphysik_gui_core_tests PRIVATE
        Qt6::Core
        segelphysik_core)
    add_test(NAME gui_core_integration
        COMMAND segelphysik_gui_core_tests
            "${SEGELPHYSIK_MODELLE_DIR}/quader/modell.json")

    add_executable(segelphysik_gui_catalog_tests
        Code/src/model_catalog.cpp
        Code/src/model_catalog.hpp
        Code/tests/model_catalog_tests.cpp)
    target_include_directories(segelphysik_gui_catalog_tests PRIVATE Code/src)
    target_link_libraries(segelphysik_gui_catalog_tests PRIVATE
        Qt6::Core
        segelphysik_core)
    add_test(NAME gui_model_catalog
        COMMAND segelphysik_gui_catalog_tests
            "${SEGELPHYSIK_MODELLE_DIR}")

    add_executable(segelphysik_gui_mesh_tests
        Code/src/display_geometry.cpp
        Code/src/display_geometry.hpp
        Code/src/model_catalog.cpp
        Code/src/model_catalog.hpp
        Code/tests/display_geometry_tests.cpp)
    target_include_directories(segelphysik_gui_mesh_tests PRIVATE Code/src)
    target_link_libraries(segelphysik_gui_mesh_tests PRIVATE
        Qt6::Core
        segelphysik_core
        VTK::CommonCore
        VTK::CommonDataModel)
    add_test(NAME gui_display_mesh
        COMMAND segelphysik_gui_mesh_tests
            "${SEGELPHYSIK_MODELLE_DIR}")

    add_executable(segelphysik_gui_force_display_tests
        Code/src/force_display.cpp
        Code/src/force_display.hpp
        Code/tests/force_display_tests.cpp)
    target_include_directories(segelphysik_gui_force_display_tests PRIVATE Code/src)
    target_link_libraries(segelphysik_gui_force_display_tests PRIVATE Qt6::Core)
    add_test(NAME gui_force_display COMMAND segelphysik_gui_force_display_tests)
endif()

if(MSVC)
    target_compile_options(segelphysik_gui PRIVATE /W4 /WX /permissive- /utf-8)
    if(BUILD_TESTING)
        target_compile_options(segelphysik_gui_core_tests PRIVATE
            /W4 /WX /permissive- /utf-8)
        target_compile_options(segelphysik_gui_catalog_tests PRIVATE
            /W4 /WX /permissive- /utf-8)
        target_compile_options(segelphysik_gui_mesh_tests PRIVATE
            /W4 /WX /permissive- /utf-8)
        target_compile_options(segelphysik_gui_force_display_tests PRIVATE
            /W4 /WX /permissive- /utf-8)
    endif()
else()
    target_compile_options(segelphysik_gui PRIVATE
        -Wall -Wextra -Wpedantic -Werror)
    if(BUILD_TESTING)
        target_compile_options(segelphysik_gui_core_tests PRIVATE
            -Wall -Wextra -Wpedantic -Werror)
        target_compile_options(segelphysik_gui_catalog_tests PRIVATE
            -Wall -Wextra -Wpedantic -Werror)
        target_compile_options(segelphysik_gui_mesh_tests PRIVATE
            -Wall -Wextra -Wpedantic -Werror)
        target_compile_options(segelphysik_gui_force_display_tests PRIVATE
            -Wall -Wextra -Wpedantic -Werror)
    endif()
endif()
