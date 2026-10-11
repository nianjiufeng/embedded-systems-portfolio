add_library(ld2412 STATIC
    src/ld2412.c
)

target_include_directories(ld2412 PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(ld2412 PUBLIC c_std_99)

if(MSVC)
    target_compile_options(ld2412 PRIVATE /W4)
else()
    target_compile_options(ld2412 PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(BUILD_TESTING)
    add_executable(ld2412_tests tests/test_ld2412.c)
    target_link_libraries(ld2412_tests PRIVATE ld2412)

    if(MSVC)
        target_compile_options(ld2412_tests PRIVATE /W4)
    else()
        target_compile_options(ld2412_tests PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    add_test(NAME ld2412_tests COMMAND ld2412_tests)
endif()
