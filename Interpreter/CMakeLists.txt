add_executable(interpreter
    interpreter_main.cpp
)

target_include_directories(interpreter
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/..
)