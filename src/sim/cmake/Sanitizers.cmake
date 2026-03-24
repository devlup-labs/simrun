function(simrun_configure_sanitizers target_name)
    if(MSVC)
        # Sanitizer support differs by toolset; keep defaults on MSVC for now.
        return()
    endif()

    set(_flags "")

    if(SIMRUN_ENABLE_ASAN)
        list(APPEND _flags -fsanitize=address)
    endif()

    if(SIMRUN_ENABLE_UBSAN)
        list(APPEND _flags -fsanitize=undefined)
    endif()

    if(SIMRUN_ENABLE_TSAN)
        list(APPEND _flags -fsanitize=thread)
    endif()

    if(_flags)
        target_compile_options("${target_name}" PRIVATE ${_flags} -fno-omit-frame-pointer)
        target_link_options("${target_name}" PRIVATE ${_flags})
    endif()
endfunction()
