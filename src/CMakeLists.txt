include_directories(${PYTHON_INCLUDE_DIRS})
include_directories(${EXTERNAL_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/extensions)

set(CMAKE_SHARED_MODULE_PREFIX)

set(modules_SRC "")
file(GLOB_RECURSE modules_SRC "*.cpp")

foreach(module_file ${modules_SRC})
    get_filename_component (name_without_extension ${module_file} NAME_WE)
    add_library(${name_without_extension} MODULE
            ${module_file})

    target_link_libraries(${name_without_extension}
            ${PYTHON_LIBRARIES}
            ${CMAKE_THREAD_LIBS_INIT}
            ${RT_LIBRARY})

    add_dependencies(${name_without_extension}
            pybind11)

    list(APPEND modules ${name_without_extension})
endforeach()

add_custom_target(src_dist
        COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/setup.py sdist --dist-dir=${CMAKE_BINARY_DIR} --formats=zip --version 'debug' --src-dir ${CMAKE_CURRENT_SOURCE_DIR} --deps-dirs ${ALLOCATORS_BUFFERS_INCLUDE_DIRS} ${CIRCULAR_BUFFERS_INCLUDE_DIRS} ${RENDEZVOUS_INCLUDE_DIRS} ${PYBIND11_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}/extensions)

add_dependencies(src_dist
        ${modules})