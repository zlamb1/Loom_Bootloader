if (NOT DEFINED OUTFILE)
    message(FATAL_ERROR "OUTFILE not defined.")
endif()

if (NOT DEFINED HEADERS)
    message(FATAL_ERROR "HEADERS not defined.")
endif()

separate_arguments(HEADERS)

file(WRITE ${OUTFILE} "")

foreach (HEADER ${HEADERS})
    file(APPEND ${OUTFILE} "#include \"${HEADER}\"\n")
endforeach()

file(APPEND ${OUTFILE} "\nvoid loom_register_export_symbols (void) {\n")

set(I 0)

foreach (HEADER ${HEADERS})
    file(STRINGS ${HEADER} LINES)

    foreach (LINE ${LINES})

        if ("${LINE}" MATCHES "^[^#].*EXPORT(_VAR)? *\\(([A-Za-z0-9_]+)\\)")

            if ("${CMAKE_MATCH_1}" STREQUAL "_VAR")
                set(KIND "0")
                set(TAKE "&")
            else()
                set(KIND "1")
                set(TAKE)
            endif()

            set(EXPORT_NAME "${CMAKE_MATCH_2}")

            file(
                APPEND 
                ${OUTFILE} 
                "\tloom_symbol_register(\"${EXPORT_NAME}\", ${KIND}, ${TAKE}${EXPORT_NAME});\n"
            )

            math(EXPR I "${I}+1")

        endif()

    endforeach()

endforeach()

file(
    APPEND
    ${OUTFILE}
    "\tcompile_assert(${I} <= LOOM_SYMTAB_SIZE, \"Symbol table full.\");\n}"
)