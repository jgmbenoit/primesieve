file(GLOB files "*.cpp")
foreach(file ${files})
    get_filename_component(binary_name ${file} NAME_WE)
    add_executable(${binary_name} ${file})
    target_link_libraries(${binary_name} libprimesieve)
endforeach()
