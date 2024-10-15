file(GLOB_RECURSE "source" CONFIGURE_DEPENDS
    src/*.c
)
set_source_files_properties(${source} PROPERTIES LANGUAGE ${CH_LANGUAGE})

# find_package(SDL2 REQUIRED)
# find_package(GLU REQUIRED)

add_executable("ch_executor" ${source})
set_property(TARGET "ch_executor" PROPERTY CXX_STANDARD 20)

# target_link_libraries("ch_executor" SDL2)
# target_link_libraries("ch_executor" GLU)