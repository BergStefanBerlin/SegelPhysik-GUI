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

find_package(Qt6 CONFIG REQUIRED COMPONENTS Widgets Svg Xml)
find_package(VTK CONFIG REQUIRED COMPONENTS
    CommonCore
    CommonDataModel
    CommonTransforms
    FiltersCore
    FiltersGeneral
    FiltersSources
    GUISupportQt
    InteractionStyle
    IOImage
    RenderingAnnotation
    RenderingCore
    RenderingFreeType
    RenderingOpenGL2)

add_subdirectory("${SEGELPHYSIK_CORE_SOURCE_DIR}/Code"
    "${CMAKE_CURRENT_BINARY_DIR}/segelphysik-core")

add_executable(segelphysik_gui WIN32
    Code/src/navigation.cpp
    Code/src/navigation.hpp
    Code/resources/navigation.qrc
    Code/src/main.cpp
    Code/src/water_motion.cpp
    Code/src/water_motion.hpp
    Code/src/main_window.cpp
    Code/src/main_window.hpp
    Code/src/graphics_settings.cpp
    Code/src/graphics_settings.hpp
    Code/src/graphics_dialog.cpp
    Code/src/graphics_dialog.hpp
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
    Qt6::Svg Qt6::Xml
    segelphysik_core
    VTK::CommonCore
    VTK::CommonDataModel
    VTK::CommonTransforms
    VTK::FiltersCore
    VTK::FiltersGeneral
    VTK::FiltersSources
    VTK::GUISupportQt
    VTK::InteractionStyle
    VTK::IOImage
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
    add_executable(segelphysik_gui_water_motion_tests
        Code/src/water_motion.cpp Code/src/graphics_settings.cpp
        Code/src/force_display.cpp Code/tests/water_motion_tests.cpp)
    target_include_directories(segelphysik_gui_water_motion_tests PRIVATE Code/src)
    target_link_libraries(segelphysik_gui_water_motion_tests PRIVATE Qt6::Core Qt6::Gui)
    add_test(NAME gui_water_motion COMMAND segelphysik_gui_water_motion_tests)
    add_executable(segelphysik_gui_navigation_tests
        Code/src/navigation.cpp
        Code/src/force_display.cpp
        Code/resources/navigation.qrc
        Code/tests/navigation_tests.cpp)
    target_include_directories(segelphysik_gui_navigation_tests PRIVATE Code/src)
    target_link_libraries(segelphysik_gui_navigation_tests PRIVATE Qt6::Widgets Qt6::Svg Qt6::Xml)
    add_test(NAME gui_navigation COMMAND segelphysik_gui_navigation_tests)
    set_tests_properties(gui_navigation PROPERTIES TIMEOUT 20)
    if(WIN32 AND DEFINED VCPKG_INSTALLED_DIR)
        set_tests_properties(gui_navigation PROPERTIES
            ENVIRONMENT "QT_QPA_PLATFORM=minimal;QT_PLUGIN_PATH=${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/Qt6/plugins")
    else()
        set_tests_properties(gui_navigation PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    endif()
    add_executable(segelphysik_gui_graphics_tests
        Code/src/graphics_settings.cpp
        Code/tests/graphics_settings_tests.cpp)
    target_include_directories(segelphysik_gui_graphics_tests PRIVATE Code/src)
    target_link_libraries(segelphysik_gui_graphics_tests PRIVATE Qt6::Core)
    add_test(NAME gui_graphics_settings COMMAND segelphysik_gui_graphics_tests)
    add_executable(segelphysik_gui_graphics_render_tests
        Code/src/water_motion.cpp
        Code/src/navigation.cpp
        Code/resources/navigation.qrc
        Code/src/main_window.cpp
        Code/src/model_catalog.cpp
        Code/src/graphics_dialog.cpp
        Code/src/core_adapter.cpp
        Code/src/graphics_settings.cpp
        Code/src/display_geometry.cpp
        Code/src/force_display.cpp
        Code/src/vtk_scene.cpp
        Code/tests/graphics_render_tests.cpp)
    target_include_directories(segelphysik_gui_graphics_render_tests PRIVATE Code/src)
    target_link_libraries(segelphysik_gui_graphics_render_tests PRIVATE
        Qt6::Widgets Qt6::Svg Qt6::Xml segelphysik_core ${VTK_LIBRARIES})
    vtk_module_autoinit(TARGETS segelphysik_gui_graphics_render_tests MODULES
        VTK::RenderingOpenGL2 VTK::RenderingFreeType VTK::GUISupportQt)
    option(SEGELPHYSIK_GRAPHICS_GPU_TEST "Run graphics tests requiring an interactive OpenGL desktop" OFF)
    if(SEGELPHYSIK_GRAPHICS_GPU_TEST)
        add_test(NAME gui_water_motion_render COMMAND segelphysik_gui_graphics_render_tests
            --water-motion-only "${SEGELPHYSIK_MODELLE_DIR}"
            "${CMAKE_CURRENT_BINARY_DIR}/graphics-evidence/water-motion")
        set_tests_properties(gui_water_motion_render PROPERTIES
            TIMEOUT 120 RUN_SERIAL TRUE
            ENVIRONMENT "QT_QPA_PLATFORM=windows;QT_SCALE_FACTOR=1")
        if(WIN32 AND DEFINED VCPKG_INSTALLED_DIR)
            set_property(TEST gui_water_motion_render APPEND PROPERTY ENVIRONMENT
                "QT_PLUGIN_PATH=${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/Qt6/plugins")
        endif()
        add_test(NAME gui_navigation_camera COMMAND segelphysik_gui_graphics_render_tests
            --navigation-only "${SEGELPHYSIK_MODELLE_DIR}"
            "${CMAKE_CURRENT_BINARY_DIR}/graphics-evidence/navigation-camera")
        set_tests_properties(gui_navigation_camera PROPERTIES
            TIMEOUT 90 RUN_SERIAL TRUE
            ENVIRONMENT "QT_QPA_PLATFORM=windows;QT_SCALE_FACTOR=1")
        if(WIN32 AND DEFINED VCPKG_INSTALLED_DIR)
            set_property(TEST gui_navigation_camera APPEND PROPERTY ENVIRONMENT
                "QT_PLUGIN_PATH=${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/Qt6/plugins")
        endif()
        foreach(scale IN ITEMS 100 150 200)
            if(scale EQUAL 100)
                set(scale_factor "1")
            elseif(scale EQUAL 150)
                set(scale_factor "1.5")
            else()
                set(scale_factor "2")
            endif()
            add_test(NAME gui_graphics_render_${scale} COMMAND segelphysik_gui_graphics_render_tests
                "${SEGELPHYSIK_MODELLE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/graphics-evidence/${scale}")
            set_tests_properties(gui_graphics_render_${scale} PROPERTIES
                TIMEOUT 300 RUN_SERIAL TRUE
                ENVIRONMENT "QT_QPA_PLATFORM=windows;QT_SCALE_FACTOR=${scale_factor}")
            if(WIN32 AND DEFINED VCPKG_INSTALLED_DIR)
                set_property(TEST gui_graphics_render_${scale} APPEND PROPERTY ENVIRONMENT
                    "QT_PLUGIN_PATH=${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/Qt6/plugins")
            endif()
        endforeach()
    endif()
    add_executable(segelphysik_gui_graphics_dialog_tests
        Code/src/graphics_settings.cpp
        Code/src/graphics_dialog.cpp
        Code/tests/graphics_dialog_tests.cpp)
    target_include_directories(segelphysik_gui_graphics_dialog_tests PRIVATE Code/src)
    target_link_libraries(segelphysik_gui_graphics_dialog_tests PRIVATE Qt6::Widgets)
    add_test(NAME gui_graphics_dialog COMMAND segelphysik_gui_graphics_dialog_tests)
    set_tests_properties(gui_graphics_dialog PROPERTIES
        TIMEOUT 20)
    if(WIN32 AND DEFINED VCPKG_INSTALLED_DIR)
        set_tests_properties(gui_graphics_dialog PROPERTIES
            ENVIRONMENT "QT_QPA_PLATFORM=minimal;QT_PLUGIN_PATH=${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/Qt6/plugins")
    else()
        set_tests_properties(gui_graphics_dialog PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    endif()
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
        target_compile_options(segelphysik_gui_water_motion_tests PRIVATE /W4 /WX /permissive- /utf-8)
        target_compile_options(segelphysik_gui_navigation_tests PRIVATE /W4 /WX /permissive- /utf-8)
        target_compile_options(segelphysik_gui_graphics_tests PRIVATE /W4 /WX /permissive- /utf-8)
        target_compile_options(segelphysik_gui_graphics_dialog_tests PRIVATE /W4 /WX /permissive- /utf-8)
        target_compile_options(segelphysik_gui_graphics_render_tests PRIVATE /W4 /WX /permissive- /utf-8)
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
        target_compile_options(segelphysik_gui_navigation_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
        target_compile_options(segelphysik_gui_graphics_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
        target_compile_options(segelphysik_gui_graphics_dialog_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
        target_compile_options(segelphysik_gui_graphics_render_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
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
