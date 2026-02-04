set(LOOM_CORE_ELF "core_elf")
set(LOOM_CORE_ELF_ABS "${CMAKE_BINARY_DIR}/${LOOM_CORE_ELF}")
set(LOOM_CORE_BIN "${CMAKE_BINARY_DIR}/core-bin")

add_executable(
    ${LOOM_CORE_ELF}
    ${LOOM_SRCS}
)

add_dependencies(${LOOM_CORE_ELF} loom_gensyms)

target_compile_definitions(${LOOM_CORE_ELF} PRIVATE LOOM_ARCH=${LOOM_ARCH})
target_compile_definitions(${LOOM_CORE_ELF} PRIVATE "${LOOM_ENDIAN_DEFINITION}")
target_compile_options(${LOOM_CORE_ELF} PRIVATE $<$<COMPILE_LANGUAGE:C>:${LOOM_C_FLAGS}>)
target_include_directories(${LOOM_CORE_ELF} PRIVATE ${LOOM_INCLUDE_DIRS})
target_link_options(${LOOM_CORE_ELF} PRIVATE ${LOOM_LINK_FLAGS})
target_link_options(${LOOM_CORE_ELF} PRIVATE LINKER:--build-id=none)

if (DEFINED LOOM_LINKER_SCRIPT)
    target_link_options(
        ${LOOM_CORE_ELF} PRIVATE "-T${LOOM_LINKER_SCRIPT}"
    )
endif()

add_custom_command(
    OUTPUT ${LOOM_CORE_BIN}
    COMMAND ${CMAKE_OBJCOPY} -O binary ${LOOM_CORE_ELF} ${LOOM_CORE_BIN}
    DEPENDS ${LOOM_CORE_ELF}
)

add_custom_target(core_bin ALL DEPENDS ${LOOM_CORE_BIN})