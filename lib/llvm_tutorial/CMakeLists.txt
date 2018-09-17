add_custom_target(llvm_tutorial)
set_target_properties(llvm_tutorial PROPERTIES FOLDER Examples)


macro(add_kaleidoscope_chapter name)
    add_dependencies(llvm_tutorial ${name})
    add_llvm_example(${name} ${ARGN})
endmacro(add_kaleidoscope_chapter name)

add_subdirectory(src/BuildingAJIT)
add_subdirectory(src/Chapter02)
add_subdirectory(src/Chapter03)
add_subdirectory(src/Chapter04)