file(GLOB_RECURSE SOURCES *.c)

set(include_dirs ".")
list(APPEND include_dirs "libsnss")
list(APPEND include_dirs "cpu")
list(APPEND include_dirs "nes")
list(APPEND include_dirs "sndhrdw")

idf_component_register(SRCS ${SOURCES} 
                       INCLUDE_DIRS ${include_dirs})

#target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-error=char-subscripts -Wno-error=attributes -DNOFRENDO_DEBUG)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-error=char-subscripts -Wno-error=attributes -Wno-stringop-truncation -Wno-implicit-function-declaration -Wno-misleading-indentation)

