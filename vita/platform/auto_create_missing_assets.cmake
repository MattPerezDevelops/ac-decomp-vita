# Auto Create Missing Assets - CMake Function
# ===========================================
# Automatically creates missing asset files at build time

function(auto_create_missing_assets)
    message(STATUS "🎯 Auto-creating missing asset files...")
    
    # Find all source files
    file(GLOB_RECURSE SOURCE_FILES 
        "${CMAKE_SOURCE_DIR}/../ac-decomp-upstream/src/*.c"
        "${CMAKE_SOURCE_DIR}/../ac-decomp-upstream/src/*.c_inc"
    )
    
    set(MISSING_ASSETS "")
    
    # Scan for missing asset includes
    foreach(SOURCE_FILE ${SOURCE_FILES})
        file(READ ${SOURCE_FILE} FILE_CONTENT)
        
        # Find asset includes
        string(REGEX MATCHALL "#include \"assets/[^\"]+\\.inc\"" ASSET_INCLUDES "${FILE_CONTENT}")
        
        foreach(ASSET_INCLUDE ${ASSET_INCLUDES})
            # Extract asset path
            string(REGEX REPLACE "#include \"assets/([^\"]+)\"" "\\1" ASSET_PATH "${ASSET_INCLUDE}")
            list(APPEND MISSING_ASSETS ${ASSET_PATH})
        endforeach()
    endforeach()
    
    # Remove duplicates
    list(REMOVE_DUPLICATES MISSING_ASSETS)
    
    message(STATUS "📊 Found ${CMAKE_MATCH_COUNT} missing assets to create")
    
    # Create missing asset files
    foreach(ASSET_PATH ${MISSING_ASSETS})
        set(FULL_ASSET_PATH "${CMAKE_SOURCE_DIR}/../ac-decomp-upstream/src/assets/${ASSET_PATH}")
        
        # Create directory if needed
        get_filename_component(ASSET_DIR ${FULL_ASSET_PATH} DIRECTORY)
        file(MAKE_DIRECTORY ${ASSET_DIR})
        
        # Create placeholder file if it doesn't exist
        if(NOT EXISTS ${FULL_ASSET_PATH})
            file(WRITE ${FULL_ASSET_PATH} "0x00\n")
        endif()
    endforeach()
    
    message(STATUS "✅ Auto-created all missing asset files")
endfunction()

# Call the function during configure time
auto_create_missing_assets() 