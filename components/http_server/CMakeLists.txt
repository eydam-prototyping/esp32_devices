idf_component_register(
    SRCS "http_api_device_info.c" "http_api_wifi_scan.c" "http_api_wifi_status.c" "http_api_wifi_config.c" "http_api.c" "http_handlers.c" "http_server.c" "spiffs.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_http_server network_manager spiffs nvs_flash json esp_timer esp_system esp_hw_support esp_driver_gpio
    PRIV_REQUIRES vfs spi_flash app_update driver
)

# Git version detection (optional, fallback to defaults if Git not available)
find_package(Git QUIET)

if(GIT_FOUND)
    # Get Git information
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --abbrev-ref HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_BRANCH
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_COMMIT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    
    execute_process(
        COMMAND ${GIT_EXECUTABLE} log -1 --format=%cd --date=iso
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_COMMIT_DATE
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    
    execute_process(
        COMMAND ${GIT_EXECUTABLE} diff --quiet
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        RESULT_VARIABLE GIT_DIRTY
        ERROR_QUIET
    )
    
    if(NOT GIT_DIRTY EQUAL 0)
        set(GIT_DIRTY_FLAG "-dirty")
    else()
        set(GIT_DIRTY_FLAG "")
    endif()
    
    set(GIT_VERSION "${GIT_COMMIT_HASH}${GIT_DIRTY_FLAG}")
    
    message(STATUS "Git Info - Branch: ${GIT_BRANCH}, Commit: ${GIT_VERSION}, Date: ${GIT_COMMIT_DATE}")
    
    # Set compile definitions with Git info
    target_compile_definitions(${COMPONENT_LIB} PRIVATE
        "GIT_BRANCH=\"${GIT_BRANCH}\""
        "GIT_COMMIT_HASH=\"${GIT_COMMIT_HASH}\""
        "GIT_COMMIT_DATE=\"${GIT_COMMIT_DATE}\""
        "GIT_VERSION=\"${GIT_VERSION}\""
        "GIT_DIRTY_FLAG=\"${GIT_DIRTY_FLAG}\""
    )
else()
    message(STATUS "Git not found - using default version info")
endif()