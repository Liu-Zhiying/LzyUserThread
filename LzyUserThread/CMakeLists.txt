project("LzyUserThread")

if (!WIN32)

message("This Library do not support non Windows system.")

else()

enable_language(ASM_MASM)

if (CMAKE_SIZEOF_VOID_P EQUAL 8)
add_library(LzyUserThread STATIC UserThread.cpp ContextFunc_x64.asm)
else()
add_library(LzyUserThread STATIC UserThread.cpp ContextFunc_x86.asm)
endif()

add_executable(LzyUserThreadTest main.cpp)
target_link_libraries(LzyUserThreadTest PRIVATE LzyUserThread)

endif()