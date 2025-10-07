cmake_minimum_required(VERSION 3.14)
project(LogosIrcModulePlugin LANGUAGES CXX)

include(GNUInstallDirs)

set(CMAKE_AUTOMOC ON)

option(LOGOS_IRC_MODULE_USE_VENDOR "Force use of vendored Logos dependencies" OFF)

set(_parent_liblogos "${CMAKE_SOURCE_DIR}/../logos-liblogos")
set(_parent_cpp_sdk "${CMAKE_SOURCE_DIR}/../logos-cpp-sdk")
set(_use_vendor ${LOGOS_IRC_MODULE_USE_VENDOR})
if(NOT _use_vendor)
    if(NOT EXISTS "${_parent_liblogos}/interface.h" OR NOT EXISTS "${_parent_cpp_sdk}/cpp/logos_api.h")
        set(_use_vendor ON)
    endif()
endif()
if(_use_vendor)
    set(LOGOS_LIBLOGOS_ROOT "${CMAKE_SOURCE_DIR}/vendor/logos-liblogos")
    set(LOGOS_CPP_SDK_ROOT "${CMAKE_SOURCE_DIR}/vendor/logos-cpp-sdk")
else()
    set(LOGOS_LIBLOGOS_ROOT "${_parent_liblogos}")
    set(LOGOS_CPP_SDK_ROOT "${_parent_cpp_sdk}")
endif()

if(NOT EXISTS "${LOGOS_LIBLOGOS_ROOT}/interface.h")
    message(FATAL_ERROR "logos-liblogos not found. Run git submodule update --init --recursive.")
endif()

if(NOT EXISTS "${LOGOS_CPP_SDK_ROOT}/cpp/logos_api.h")
    message(FATAL_ERROR "logos-cpp-sdk not found. Run git submodule update --init --recursive.")
endif()

get_filename_component(LOGOS_DEPS_ROOT "${LOGOS_CPP_SDK_ROOT}" DIRECTORY)

if(NOT DEFINED QT_VERSION_MAJOR)
    find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core RemoteObjects Network)
    if(Qt6_FOUND)
        set(QT_VERSION_MAJOR 6)
    else()
        set(QT_VERSION_MAJOR 5)
    endif()
endif()
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core RemoteObjects Network)

# Print dependencies from metadata.json during configure (using generator)
#set(METADATA_FILE "${CMAKE_CURRENT_SOURCE_DIR}/metadata.json")
#if(EXISTS "${METADATA_FILE}" AND EXISTS "${CMAKE_SOURCE_DIR}/../build/cpp-generator/bin/logos-cpp-generator")
#    execute_process(
#        COMMAND "${CMAKE_SOURCE_DIR}/../build/cpp-generator/bin/logos-cpp-generator" --metadata "${METADATA_FILE}"
#        OUTPUT_VARIABLE METADATA_DEPS_OUTPUT
#        ERROR_VARIABLE METADATA_DEPS_ERROR
#        OUTPUT_STRIP_TRAILING_WHITESPACE
#    )
#    if(METADATA_DEPS_OUTPUT)
#        string(REPLACE "\n" ";" METADATA_DEPS_LIST "${METADATA_DEPS_OUTPUT}")
#        foreach(dep IN LISTS METADATA_DEPS_LIST)
#            message(STATUS "${dep}")
#        endforeach()
#    endif()
#else()
#    message(WARNING "Generator or metadata.json not found; skipping dependency printout")
#endif()

# (We will include the umbrella source logos_sdk.cpp instead of globbing individual files)

# Run Logos C++ generator on metadata before compilation
set(CPP_GENERATOR_BUILD_DIR "${LOGOS_DEPS_ROOT}/build/cpp-generator")
set(CPP_GENERATOR "${CPP_GENERATOR_BUILD_DIR}/bin/logos-cpp-generator")
set(METADATA_JSON "${CMAKE_CURRENT_SOURCE_DIR}/metadata.json")
set(PLUGINS_OUTPUT_DIR "${CMAKE_BINARY_DIR}/modules")

if(NOT TARGET cpp_generator_build)
    add_custom_target(cpp_generator_build
        COMMAND bash "${LOGOS_CPP_SDK_ROOT}/cpp-generator/compile.sh"
        WORKING_DIRECTORY "${LOGOS_DEPS_ROOT}"
        COMMENT "Building logos-cpp-generator via ${LOGOS_CPP_SDK_ROOT}/cpp-generator/compile.sh"
        VERBATIM
    )
endif()

set(GENERATED_LOGOS_SDK_CPP ${LOGOS_CPP_SDK_ROOT}/cpp/generated/logos_sdk.cpp)
set_source_files_properties(${GENERATED_LOGOS_SDK_CPP} PROPERTIES GENERATED TRUE)

add_custom_target(generate_irc_sdk_wrappers
    COMMAND "${CPP_GENERATOR}" --metadata "${METADATA_JSON}" --module-dir "${PLUGINS_OUTPUT_DIR}"
    WORKING_DIRECTORY "${LOGOS_DEPS_ROOT}"
    DEPENDS ${METADATA_JSON}
    COMMENT "Generating umbrella ${GENERATED_LOGOS_SDK_CPP} and wrappers via logos-cpp-generator"
    VERBATIM
)
add_dependencies(generate_irc_sdk_wrappers cpp_generator_build)
# Dynamically depend on CMake targets for modules listed in metadata.json
# (Not required because generator handles ordering via --module-dir)
# if(METADATA_DEPS_LIST)
#     foreach(dep IN LISTS METADATA_DEPS_LIST)
#         if(TARGET ${dep})
#             add_dependencies(generate_irc_sdk_wrappers ${dep})
#         endif()
#     endforeach()
# endif()

# No separate per-dependency loop needed; handled by --module-dir

# Plugin sources
set(PLUGIN_SOURCES
    logos_irc_plugin.cpp
    logos_irc_plugin.h
    logos_irc_interface.h
    ircserver.cpp
    ircserver.h
    ircclient.cpp
    ircclient.h
    ${LOGOS_LIBLOGOS_ROOT}/interface.h
    ${LOGOS_CPP_SDK_ROOT}/cpp/logos_api.cpp
    ${LOGOS_CPP_SDK_ROOT}/cpp/logos_api.h
    ${LOGOS_CPP_SDK_ROOT}/cpp/logos_api_client.cpp
    ${LOGOS_CPP_SDK_ROOT}/cpp/logos_api_client.h
    ${LOGOS_CPP_SDK_ROOT}/cpp/logos_api_consumer.cpp
    ${LOGOS_CPP_SDK_ROOT}/cpp/logos_api_consumer.h
    ${LOGOS_CPP_SDK_ROOT}/cpp/logos_api_provider.cpp
    ${LOGOS_CPP_SDK_ROOT}/cpp/logos_api_provider.h
    ${LOGOS_CPP_SDK_ROOT}/cpp/token_manager.cpp
    ${LOGOS_CPP_SDK_ROOT}/cpp/token_manager.h
    ${LOGOS_CPP_SDK_ROOT}/cpp/module_proxy.cpp
    ${LOGOS_CPP_SDK_ROOT}/cpp/module_proxy.h
)

# Create the plugin library
add_library(logos_irc_plugin SHARED ${PLUGIN_SOURCES})

# Ensure generator runs before building the plugin
add_dependencies(logos_irc_plugin generate_irc_sdk_wrappers)

# Add umbrella source that includes all generated wrappers
target_sources(logos_irc_plugin PRIVATE ${GENERATED_LOGOS_SDK_CPP})

# Set output name without lib prefix
set_target_properties(logos_irc_plugin PROPERTIES
    PREFIX "")

# Link Qt libraries
target_link_libraries(logos_irc_plugin PRIVATE 
    Qt${QT_VERSION_MAJOR}::Core 
    Qt${QT_VERSION_MAJOR}::RemoteObjects
    Qt${QT_VERSION_MAJOR}::Network
)

# Include directories
target_include_directories(logos_irc_plugin PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${LOGOS_LIBLOGOS_ROOT}
    ${LOGOS_CPP_SDK_ROOT}/cpp
    ${LOGOS_CPP_SDK_ROOT}/cpp/generated
)

# Set common properties for both platforms
set_target_properties(logos_irc_plugin PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/modules"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/modules"  # For Windows .dll
    BUILD_WITH_INSTALL_RPATH TRUE
    SKIP_BUILD_RPATH FALSE)

if(APPLE)
    # macOS specific settings
    set_target_properties(logos_irc_plugin PROPERTIES
        INSTALL_RPATH "@loader_path"
        INSTALL_NAME_DIR "@rpath"
        BUILD_WITH_INSTALL_NAME_DIR TRUE)
    
    add_custom_command(TARGET logos_irc_plugin POST_BUILD
        COMMAND install_name_tool -id "@rpath/logos_irc_plugin.dylib" $<TARGET_FILE:logos_irc_plugin>
        COMMENT "Updating library paths for macOS"
    )
else()
    # Linux specific settings
    set_target_properties(logos_irc_plugin PROPERTIES
        INSTALL_RPATH "$ORIGIN"
        INSTALL_RPATH_USE_LINK_PATH FALSE)
endif()

install(TARGETS logos_irc_plugin
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/logos/modules
    RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR}/logos/modules
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}/logos/modules
)

install(FILES ${METADATA_JSON}
    DESTINATION ${CMAKE_INSTALL_DATADIR}/logos-irc-module
)

install(DIRECTORY "${PLUGINS_OUTPUT_DIR}/"
    DESTINATION ${CMAKE_INSTALL_DATADIR}/logos-irc-module/generated
    OPTIONAL
)