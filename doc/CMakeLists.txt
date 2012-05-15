FIND_PACKAGE(Doxygen)
IF(DOXYGEN_FOUND)
    # click+jump in Emacs and Visual Studio (for Doxyfile) (jw)
    if (CMAKE_BUILD_TOOL MATCHES "(msdev|devenv)")
        SET(DOXY_WARN_FORMAT "\"\$file(\$line) : \$text \"")
    else  ()
        SET(DOXY_WARN_FORMAT "\"\$file:\$line: \$text \"")
    endif ()

    if(DOXYGEN_DOT_EXECUTABLE)
        SET(DOXY_HAVE_DOT "YES")
    else()
        SET(DOXY_HAVE_DOT "NO")
    endif()

    message(STATUS "configured Doxyfile")
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in
        ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
        @ONLY
    )

    add_custom_target(Documentation ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/${target})
endif()