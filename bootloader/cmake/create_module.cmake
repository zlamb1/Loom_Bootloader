set(LOOM_MODULES)
set(LOOM_MOD_BIN_DIR "${CMAKE_BINARY_DIR}/modules")
set(LOOM_MOD_SRC_DIR "${LOOM_SRC_DIR}/mods")

function(create_module NAME)
    set(SRCS ${ARGN})
    set(MODULE_OBJS "module_${NAME}_objs")
    set(MODULE_LIB "${LOOM_MOD_BIN_DIR}/${NAME}.lib")
    set(MODULE_BIN "${LOOM_MOD_BIN_DIR}/${NAME}.mod")

    list(APPEND LOOM_MODULES "${MODULE_BIN}")
    set(LOOM_MODULES "${LOOM_MODULES}" PARENT_SCOPE)

    add_library(${MODULE_OBJS} OBJECT ${SRCS})

    message(STATUS "Creating Module : '${NAME}''")

    target_compile_definitions(${MODULE_OBJS} PRIVATE LOOM_MODULE ${LOOM_ENDIAN_DEFINITION})
    target_compile_options(${MODULE_OBJS} PRIVATE $<$<COMPILE_LANGUAGE:C>:${LOOM_C_FLAGS}>)
    target_include_directories(${MODULE_OBJS} PRIVATE ${LOOM_INCLUDE_DIRS})

    make_directory(${LOOM_MOD_BIN_DIR})

    set(FLAGS)
    if (DEFINED CMAKE_C_COMPILER_TARGET)
        set(FLAGS "--target=${CMAKE_C_COMPILER_TARGET}")
    endif()

    add_custom_command(
        OUTPUT ${MODULE_LIB} ${MODULE_BIN}
        COMMAND_EXPAND_LISTS
        COMMAND ${CMAKE_C_COMPILER} ${FLAGS} ${LOOM_LINK_FLAGS} -r $<TARGET_OBJECTS:${MODULE_OBJS}> -o ${MODULE_LIB}
        COMMAND ${CMAKE_OBJCOPY} --strip-unneeded ${MODULE_LIB} ${MODULE_BIN}
        DEPENDS ${MODULE_OBJS}
        VERBATIM
    )

    add_custom_target("module_${NAME}_bin" ALL DEPENDS ${MODULE_BIN})
endfunction()