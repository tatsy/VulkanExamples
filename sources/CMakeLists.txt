function(BUILD_EXAMPLE EXPNAME)
  set(TARGET_EXAMPLE_DIR "${CMAKE_CURRENT_LIST_DIR}/${EXPNAME}")
  configure_file("${CMAKE_CURRENT_LIST_DIR}/${EXPNAME}/common.h.in"
                 "${CMAKE_CURRENT_LIST_DIR}/${EXPNAME}/common.h" @ONLY)

  file(GLOB SOURCE_FILES "${EXPNAME}/*.cpp" "${EXPNAME}/*.h")
  file(GLOB SHADER_FILES "${EXPNAME}/shaders/*.vert" "${EXPNAME}/shaders/*.frag")

  include_directories("${CMAKE_CURRENT_LIST_DIR}/external"
                      ${GLM_INCLUDE_DIR}
                      ${GLFW3_INCLUDE_DIR}
                      ${VULKAN_INCLUDE_DIR})
  add_executable(${EXPNAME} ${SOURCE_FILES} ${SHADER_FILES})
  target_link_libraries(${EXPNAME} ${OPENGL_LIBRARIES}
                        ${GLFW3_LIBRARIES} ${VULKAN_LIBRARIES})

  source_group("Source Files" FILES ${SOURCE_FILES})
  source_group("Shader Files" FILES ${SHADER_FILES})

  if (MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zi")
    set_property(TARGET ${EXPNAME} APPEND PROPERTY LINK_FLAGS "/DEBUG /PROFILE")
  endif()


  if (MSVC)
    foreach(GLSL_FILE IN LISTS SHADER_FILES)
      get_filename_component(BASENAME ${GLSL_FILE} NAME_WE)
      add_custom_command(TARGET ${EXPNAME}
                         DEPENDS ${GLSL_FILE}
                         WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/${EXPNAME}/shaders"
                         COMMAND glslangValidator
                         ARGS -V "\"${GLSL_FILE}\"" -o "\"${BASENAME}.spv\"")
    endforeach()
  endif()
endfunction(BUILD_EXAMPLE)

file(GLOB SUBDIR_LIST RELATIVE ${CMAKE_CURRENT_LIST_DIR} "*")
foreach(SUBDIR ${SUBDIR_LIST})
  if (NOT ${SUBDIR} STREQUAL "external")
    if (IS_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/${SUBDIR}")
      BUILD_EXAMPLE(${SUBDIR})
    endif()
  endif()
endforeach()
