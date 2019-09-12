function(BUILD_EXAMPLE EXPNAME)
    # Parse arguments
    set(options)
    set(oneValueArgs FOLDER)
    set(multiValueArgs)
    cmake_parse_arguments(BUILD_EXAMPLE "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # Glob source files
    file(GLOB SOURCE_FILES "${EXPNAME}/*.cpp" "${EXPNAME}/*.h")
    file(GLOB SHADER_FILES "${EXPNAME}/shaders/*.vert"
                           "${EXPNAME}/shaders/*.frag")

    # Output directory
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${EXPNAME}/bin)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/${EXPNAME}/bin)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/${EXPNAME}/bin)
    set(CMAKE_DEBUG_POSTFIX "-debug")

    # Set output executable
    include_directories(${GLM_INCLUDE_DIRS}
                        ${GLFW3_INCLUDE_DIRS}
                        ${VULKAN_INCLUDE_DIRS})
    add_executable(${EXPNAME} ${SOURCE_FILES} ${SHADER_FILES})
    target_link_libraries(${EXPNAME} ${OPENGL_LIBRARIES} ${GLFW3_LIBRARIES} ${VULKAN_LIBRARIES})
    set_target_properties(${EXPNAME} PROPERTIES DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX})

    source_group("Source Files" FILES ${SOURCE_FILES})
    source_group("Shader Files" FILES ${SHADER_FILES})

    # Shader compilation
    set(OUTPUT_SHADERS "")
    set(SHADER_OUTPUT_DIR "${CMAKE_BINARY_DIR}/${EXPNAME}/shaders")

    foreach (SHADER IN LISTS SHADER_FILES)
        get_filename_component(BASE_NAME ${SHADER} NAME)
        set(OUTPUT_SHADER ${SHADER_OUTPUT_DIR}/${BASE_NAME}.spv)

        add_custom_command(OUTPUT ${OUTPUT_SHADER}
                           COMMAND ${CMAKE_COMMAND}
                           ARGS -E remove "${OUTPUT_SHADER}"
                           COMMAND ${CMAKE_COMMAND}
                           ARGS -E make_directory  "${SHADER_OUTPUT_DIR}"
                           COMMAND glslangValidator
                           ARGS -i "${SHADER}" -V -o "${OUTPUT_SHADER}"
                           DEPENDS ${SHADER})

        add_custom_target(GLSLANG_${EXPNAME}_${BASE_NAME} ALL SOURCES ${OUTPUT_SHADER})
        add_dependencies(${EXPNAME} GLSLANG_${EXPNAME}_${BASE_NAME})
    endforeach()

    if (MSVC)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zi")
        set_property(TARGET ${EXPNAME} APPEND PROPERTY LINK_FLAGS "/DEBUG /PROFILE")
    endif()

endfunction(BUILD_EXAMPLE)