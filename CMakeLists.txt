cmake_minimum_required(VERSION 3.25)

# This tells cmake we have goodies in the /cmake folder
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include (PamplejuceVersion)

# Modern concise way to add dependencies to your project
include (CPM)

# Configures universal binaries and decides which version of macOS to support
include(PamplejuceMacOS)

# Couple tweaks that IMO should be JUCE defaults
include(JUCEDefaults)

# Change me!
# This is the internal name of the project and the name of JUCE's shared code target
# Note: This cannot have spaces (it may be 2024, but you can't have it all!)
# Worry not, JUCE's PRODUCT_NAME can have spaces (and is what DAWs display)
set(PROJECT_NAME "Peachy")

# Worry not, JUCE's PRODUCT_NAME can have spaces (and is what DAWs will display)
# You can also just have it be the same thing as PROJECT_NAME
# You may want to append the major version on the end of this (and PROJECT_NAME) ala:
#   set(PROJECT_NAME "MyPlugin_v${MAJOR_VERSION}")
# Doing so enables major versions to show up in IDEs and DAWs as separate plugins
# allowing you to change parameters and behavior without breaking existing user projects
set(PRODUCT_NAME "Peachy")

# Change me! Used for the MacOS bundle name and Installers
set(COMPANY_NAME "Nayan")

# Change me! Used for the MacOS bundle identifier (and signing)
set(BUNDLE_ID "com.nayan.peachy")

# Change me! Set the plugin formats you want built
# Valid choices: AAX Unity VST VST3 AU AUv3 Standalone
set(FORMATS Standalone AU VST3 AUv3)

# Force JUCE to work with sccache
# instruct MSVC to embed debug info (/Z7) instead of emitting a .pdb
set(CMAKE_POLICY_DEFAULT_CMP0141        NEW      CACHE STRING "" FORCE)
set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT Embedded CACHE STRING "" FORCE)

# For simplicity, the name of the CMake project is also the name of the target
project(${PROJECT_NAME} VERSION ${CURRENT_VERSION})

# Log build environment info (after project() so compiler info is available)
include(PamplejuceLog)

# JUCE is setup as a submodule in the /JUCE folder
# Locally, you must run `git submodule update --init --recursive` once
# and later `git submodule update --remote --merge` to keep it up to date
# On Github Actions, this is done as a part of actions/checkout
add_subdirectory(JUCE)

# Add CLAP format
add_subdirectory(modules/clap-juce-extensions EXCLUDE_FROM_ALL)

# Add any other modules you want modules here, before the juce_add_plugin call
# juce_add_module(modules/my_module)

# This adds the melatonin inspector module
add_subdirectory (modules/melatonin_inspector)

# melatonin_blur (MIT): fast, cached drop/inner shadows and glows used by
# PeachyLookAndFeel. Added as a git submodule (like melatonin_inspector) because
# JUCE's juce_add_module requires the module folder basename to match its header
# name — which CPM's "<name>-src" checkout dir would break.
add_subdirectory (modules/melatonin_blur)

# See `docs/CMake API.md` in the JUCE repo for all config options
juce_add_plugin("${PROJECT_NAME}"
    # Standalone app + plugin bundle icon. JUCE (juceaide) bakes the native
    # .icns/.ico from this PNG; a distribution-ready packaging/Icon.icns is also
    # generated from the same source for the macOS installer/DMG.
    ICON_BIG "${CMAKE_CURRENT_SOURCE_DIR}/assets/logo.png"
    ICON_SMALL "${CMAKE_CURRENT_SOURCE_DIR}/assets/logo.png"

    # Change me!
    COMPANY_NAME "${COMPANY_NAME}"
    BUNDLE_ID "${BUNDLE_ID}"

    # On MacOS, plugin is copied to /Users/yourname/Library/Audio/Plug-Ins/
    COPY_PLUGIN_AFTER_BUILD TRUE

    # Change me!
    # A four-character plugin id
    # First character MUST be uppercase for AU formats
    PLUGIN_MANUFACTURER_CODE Nayn

    # Change me!
    # A unique four-character plugin id
    # Note: this must have at least one upper-case character
    PLUGIN_CODE Pchy
    FORMATS "${FORMATS}"

    # The name of your final executable
    # This is how it's listed in the DAW
    # This can be different from PROJECT_NAME and can have spaces!
    # You might want to use v${MAJOR_VERSION} here once you go to v2...
    PRODUCT_NAME "${PRODUCT_NAME}")

# This lets us use our code in both the JUCE targets and our Test target
# Without running into ODR violations
add_library(SharedCode INTERFACE)

clap_juce_extensions_plugin(TARGET "${PROJECT_NAME}"
    CLAP_ID "${BUNDLE_ID}"
    CLAP_FEATURES audio-effect)

# Enable fast math, C++20 and a few other target defaults
include(SharedCodeDefaults)


# Manually list all .h and .cpp files for the plugin
# If you are like me, you'll use globs for your sanity.
# Just ensure you employ CONFIGURE_DEPENDS so the build system picks up changes
# If you want to appease the CMake gods and avoid globs, manually add files like so:
# set(SourceFiles Source/PluginEditor.h Source/PluginProcessor.h Source/PluginEditor.cpp Source/PluginProcessor.cpp)
file(GLOB_RECURSE SourceFiles CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/source/*.h")
target_sources(SharedCode INTERFACE ${SourceFiles})

# Adds a BinaryData target for embedding assets into the binary.
# Custom fonts for PeachyLookAndFeel live in assets/Fonts/ (Righteous-Regular.ttf,
# ShareTechMono-Regular.ttf, Oswald-Regular.ttf) and get picked up automatically by
# the recursive glob below — no extra wiring needed here. Symbol names follow JUCE's
# BinaryData convention (hyphens dropped, '.' becomes '_'), e.g.
# BinaryData::RighteousRegular_ttf / BinaryData::RighteousRegular_ttfSize.
include(Assets)

# MacOS only: Cleans up folder and target organization on Xcode.
include(XcodePrettify)

# This is where you can set preprocessor definitions for JUCE and your plugin
target_compile_definitions(SharedCode
    INTERFACE

    # JUCE_WEB_BROWSER and JUCE_USE_CURL off by default
    JUCE_WEB_BROWSER=0  # If you set this to 1, add `NEEDS_WEB_BROWSER TRUE` to the `juce_add_plugin` call
    JUCE_USE_CURL=0     # If you set this to 1, add `NEEDS_CURL TRUE` to the `juce_add_plugin` call
    JUCE_VST3_CAN_REPLACE_VST2=0

    # Uncomment if you are paying for a an Indie/Pro license or releasing under GPLv3
    # JUCE_DISPLAY_SPLASH_SCREEN=0

    # lets the app known if we're Debug or Release
    CMAKE_BUILD_TYPE="${CMAKE_BUILD_TYPE}"
    VERSION="${CURRENT_VERSION}"

    # JucePlugin_Name is for some reason doesn't use the nicer PRODUCT_NAME
    PRODUCT_NAME_WITHOUT_VERSION="Peachy"
)

# Link to any other modules you added (with juce_add_module) here!
# Usually JUCE modules must have PRIVATE visibility
# See https://github.com/juce-framework/JUCE/blob/master/docs/CMake%20API.md#juce_add_module
# However, with Pamplejuce, you'll link modules to SharedCode with INTERFACE visibility
# This allows the JUCE plugin targets and the Tests target to link against it
target_link_libraries(SharedCode
    INTERFACE
    Assets
    melatonin_inspector
    melatonin_blur
    juce_audio_utils
    juce_audio_processors
    juce_dsp
    juce_gui_basics
    juce_gui_extra
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags)

# Link the JUCE plugin targets our SharedCode target
target_link_libraries("${PROJECT_NAME}" PRIVATE SharedCode)

# IPP support, comment out to disable
include(PamplejuceIPP)

# Run Catch test discovery at test time (PRE_TEST) rather than at build time.
# On Apple Silicon an unsigned executable is killed by the kernel before it can
# run, so POST_BUILD discovery would try to exec the binary before our ad-hoc
# codesign step (below) had a chance to sign it. PRE_TEST defers discovery until
# `ctest` runs, by which point the POST_BUILD signing has already happened.
# Must be set before including the test/benchmark modules.
set(CMAKE_CATCH_DISCOVER_TESTS_DISCOVERY_MODE "PRE_TEST" CACHE STRING "" FORCE)

# Everything related to the tests target
include(Tests)

# A separate target for Benchmarks (keeps the Tests target fast)
include(Benchmarks)

# On Apple Silicon, unsigned executables are SIGKILL'd before they can run, which
# breaks `ctest` test discovery. Ad-hoc sign the test binaries right after build.
# Kept here (parent repo) rather than in the third-party `cmake/` submodule so a
# fresh clone reproduces it without local submodule edits.
if (APPLE)
    add_custom_command(TARGET Tests POST_BUILD
        COMMAND codesign --force -s - "$<TARGET_FILE:Tests>"
        COMMENT "Ad-hoc code signing Tests so it can run for test discovery")
    add_custom_command(TARGET Benchmarks POST_BUILD
        COMMAND codesign --force -s - "$<TARGET_FILE:Benchmarks>"
        COMMENT "Ad-hoc code signing Benchmarks so it can run for test discovery")
endif ()

# Output some config for CI (like our PRODUCT_NAME)
include(GitHubENV)

if (APPLE)
    # Use our new full-bleed custom icns for the Standalone app
    # This overrides the auto-generated one from juceaide
    target_sources("${PROJECT_NAME}_Standalone" PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/packaging/Icon.icns")
    set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/packaging/Icon.icns" PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
    set_target_properties("${PROJECT_NAME}_Standalone" PROPERTIES MACOSX_BUNDLE_ICON_FILE "Icon.icns")
endif()
