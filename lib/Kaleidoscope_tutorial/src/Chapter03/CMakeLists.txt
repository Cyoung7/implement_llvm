set(LLVM_LINK_COMPONENTS  Analysis
        Core
        Support
        )

add_kaleidoscope_chapter(Kaleidoscope-Ch3  toy.cpp)
#export_executable_symbols(Kaleidoscope-Ch3)