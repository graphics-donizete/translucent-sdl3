# Creates C resources file from files in given directory
# https://stackoverflow.com/questions/11813271/embed-resources-eg-shader-code-images-into-executable-library-with-cmake/11814544#11814544
function(create_resources dir output)

    # Then create
    file(WRITE ${output} "")

    # Collect input files
    file(GLOB bins ${dir}/*)

    # Iterate through input files
    foreach(bin ${bins})
        # Get short filename
        string(REGEX MATCH "([^/]+)$" filename ${bin})
        # Replace filename spaces & extension separator for C compatibility
        string(REGEX REPLACE "\\.| |-" "_" filename ${filename})
        # Read hex data from file
        file(READ ${bin} filedata HEX)
        # Convert hex data for C compatibility
        string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," filedata ${filedata})
        # Append data to output file
        file(APPEND ${output} "const unsigned char ${filename}[] = {${filedata}};\nconst unsigned ${filename}_size = sizeof(${filename});\n")
    endforeach()
endfunction()

create_resources("${CMAKE_CURRENT_SOURCE_DIR}/fonts" "${CMAKE_CURRENT_SOURCE_DIR}/include/resources/font.h")

add_library(translucent-resources INTERFACE)
target_include_directories(
    translucent-resources
    INTERFACE 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> 
    $<INSTALL_INTERFACE:include>)