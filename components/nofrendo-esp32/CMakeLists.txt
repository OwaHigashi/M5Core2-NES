file(GLOB_RECURSE SOURCES *.c)

set(include_dirs "./")

idf_component_register(SRCS ${SOURCES}
                       INCLUDE_DIRS ${include_dir}
                       REQUIRES nofrendo)