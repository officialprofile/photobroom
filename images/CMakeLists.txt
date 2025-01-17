
if(WIN32)
    if(NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/tools/ImageMagick-7.1.0-portable-Q16-x64.zip)
        message("Downloading ImageMagick")
        file(DOWNLOAD
            https://imagemagick.org/archive/binaries/ImageMagick-7.1.0-portable-Q16-x64.zip ${CMAKE_CURRENT_BINARY_DIR}/tools/ImageMagick-7.1.0-portable-Q16-x64.zip
            SHOW_PROGRESS
        )
        file(ARCHIVE_EXTRACT
            INPUT ${CMAKE_CURRENT_BINARY_DIR}/tools/ImageMagick-7.1.0-portable-Q16-x64.zip
            DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/tools/ImageMagick
        )
    endif()

    find_program(Magick magick REQUIRED
        HINTS ${CMAKE_CURRENT_BINARY_DIR}/tools/ImageMagick
    )

    add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/images/main_ico.ico
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/images
        COMMAND ${Magick} convert ${CMAKE_SOURCE_DIR}/images/photo_broom.svg -resize 32x32 ${CMAKE_BINARY_DIR}/images/main_ico.ico
        DEPENDS ${CMAKE_SOURCE_DIR}/images/photo_broom.svg
    )

    add_custom_target(Images
        DEPENDS ${CMAKE_BINARY_DIR}/images/main_ico.ico
    )

endif()
