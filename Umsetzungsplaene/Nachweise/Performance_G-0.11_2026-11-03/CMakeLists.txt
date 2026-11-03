cmake_minimum_required(VERSION 3.25)
project(GuiPerformanceEvidence LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
foreach(PATH_VAR IN ITEMS GUI_SOURCE CORE_SOURCE CORE_LIBRARY)
    file(TO_CMAKE_PATH "${${PATH_VAR}}" ${PATH_VAR})
endforeach()
find_package(Qt6 CONFIG REQUIRED COMPONENTS Widgets Svg Xml)
find_package(VTK CONFIG REQUIRED COMPONENTS CommonCore CommonDataModel CommonTransforms
    FiltersCore FiltersGeneral FiltersSources GUISupportQt InteractionStyle IOImage
    RenderingAnnotation RenderingCore RenderingFreeType RenderingOpenGL2)
set(SOURCES navigation water_motion graphics_settings graphics_dialog model_catalog
    core_adapter display_geometry force_display vtk_scene)
if(AUDIT_CURRENT)
    list(APPEND SOURCES boat_inspection)
    file(READ "${GUI_SOURCE}/Code/src/main_window.hpp" WINDOW_HEADER)
    string(REPLACE "private:" "public:" WINDOW_HEADER "${WINDOW_HEADER}")
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/audit_main_window.hpp" "${WINDOW_HEADER}")
    file(READ "${GUI_SOURCE}/Code/src/main_window.cpp" WINDOW_SOURCE)
    string(REPLACE "\"main_window.hpp\"" "\"audit_main_window.hpp\"" WINDOW_SOURCE "${WINDOW_SOURCE}")
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/audit_main_window.cpp" "${WINDOW_SOURCE}")
endif()
set(FILES audit.cpp "${GUI_SOURCE}/Code/resources/navigation.qrc")
if(AUDIT_CURRENT)
    list(APPEND FILES "${CMAKE_CURRENT_BINARY_DIR}/audit_main_window.cpp")
endif()
foreach(SOURCE IN LISTS SOURCES)
    list(APPEND FILES "${GUI_SOURCE}/Code/src/${SOURCE}.cpp")
endforeach()
add_executable(gui_audit ${FILES})
target_include_directories(gui_audit PRIVATE "${GUI_SOURCE}/Code/src"
    "${CORE_SOURCE}/Code/include" "${CMAKE_CURRENT_BINARY_DIR}")
target_compile_definitions(gui_audit PRIVATE AUDIT_CURRENT=$<BOOL:${AUDIT_CURRENT}>
    AUDIT_OPTIMIZED=$<BOOL:${AUDIT_OPTIMIZED}>
    "AUDIT_REVISION=\"${AUDIT_REVISION}\"")
target_link_libraries(gui_audit PRIVATE Qt6::Widgets Qt6::Svg Qt6::Xml
    "${CORE_LIBRARY}" ${VTK_LIBRARIES})
vtk_module_autoinit(TARGETS gui_audit MODULES
    VTK::RenderingOpenGL2 VTK::RenderingFreeType VTK::GUISupportQt)
if(MSVC)
    target_compile_options(gui_audit PRIVATE /utf-8 /permissive-)
endif()
