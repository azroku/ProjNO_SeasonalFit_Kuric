cmake_minimum_required(VERSION 3.18)

set(SOLUTION_NAME SeasonalFit)

#BEGIN common part for all solutions
project(${SOLUTION_NAME})
set(SOURCE_ROOT ${CMAKE_CURRENT_LIST_DIR})
if (WIN32)
	set(HOME_ROOT $ENV{USERPROFILE})
	string(REPLACE "\\" "/" HOME_ROOT "${HOME_ROOT}")
else()
	set(HOME_ROOT $ENV{HOME})
endif()
set(NATID_SDK_ROOT ${HOME_ROOT}/natID.SDK)
include(${NATID_SDK_ROOT}/DevEnv/Common.cmake)
#END common part for all solutions


include(${NATID_SDK_ROOT}/DevEnv/natGUI.cmake)

include(SeasonalFit.cmake)

# macOS bundle finalization for Makefiles and Ninja.
#
# natID creates a regular macOS .app bundle through CMake. Xcode expands and
# signs that bundle itself. With Makefiles/Ninja, however, the final plist and
# ad-hoc signature must be made explicit so Finder can locate and launch the
# executable on both Intel and Apple Silicon Macs.
set(GUI_TARGET ${SEASONALFIT_NAME})
if(APPLE AND NOT CMAKE_GENERATOR STREQUAL "Xcode" AND TARGET ${GUI_TARGET})
	set(PLIST_TOOL /usr/libexec/PlistBuddy)
	set(BUNDLE_PLIST $<TARGET_BUNDLE_DIR:${GUI_TARGET}>/Contents/Info.plist)
	add_custom_command(TARGET ${GUI_TARGET} POST_BUILD
		COMMAND ${PLIST_TOOL} -c "Set :CFBundleExecutable ${GUI_TARGET}" ${BUNDLE_PLIST}
		COMMAND ${PLIST_TOOL} -c "Set :CFBundleName SeasonalFit" ${BUNDLE_PLIST}
		COMMAND ${PLIST_TOOL} -c "Set :CFBundleDisplayName SeasonalFit" ${BUNDLE_PLIST}
		COMMAND ${PLIST_TOOL} -c "Set :CFBundleIdentifier ba.unsa.etf.seasonalfit" ${BUNDLE_PLIST}
		COMMAND ${PLIST_TOOL} -c "Set :CFBundlePackageType APPL" ${BUNDLE_PLIST}
		COMMAND ${PLIST_TOOL} -c "Set :CFBundleDevelopmentRegion en" ${BUNDLE_PLIST}
		COMMAND ${PLIST_TOOL} -c "Set :LSMinimumSystemVersion 12.3" ${BUNDLE_PLIST}
		COMMAND codesign --force --deep --sign - $<TARGET_BUNDLE_DIR:${GUI_TARGET}>
		VERBATIM
		COMMENT "Finalizing and ad-hoc signing the SeasonalFit macOS bundle"
	)
endif()
