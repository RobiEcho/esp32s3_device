idf_build_get_property(target IDF_TARGET)

file(GLOB_RECURSE LVGL_SOURCES "${CMAKE_CURRENT_LIST_DIR}/src/*.c")

idf_component_register(
    SRCS ${LVGL_SOURCES}
    INCLUDE_DIRS "." "src"
)

target_compile_definitions(${COMPONENT_LIB} PUBLIC LV_CONF_INCLUDE_SIMPLE)
