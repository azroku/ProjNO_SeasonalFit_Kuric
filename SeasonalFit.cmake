set(SEASONALFIT_NAME SeasonalFit)

#collect source files
file(GLOB SEASONALFIT_SOURCES  ${CMAKE_CURRENT_LIST_DIR}/src/*.cpp)
file(GLOB SEASONALFIT_INCS     ${CMAKE_CURRENT_LIST_DIR}/src/*.h)
set(SEASONALFIT_PLIST          ${CMAKE_CURRENT_LIST_DIR}/src/Info.plist)

#SDK headers (only so they show up in the IDE tree)
file(GLOB SEASONALFIT_INC_TD   ${NATID_SDK_INC}/td/*.h)
file(GLOB SEASONALFIT_INC_GUI  ${NATID_SDK_INC}/gui/*.h)

# add executable
add_executable(${SEASONALFIT_NAME}
	${SEASONALFIT_INCS} ${SEASONALFIT_SOURCES}
	${SEASONALFIT_INC_TD} ${SEASONALFIT_INC_GUI})

target_compile_features(${SEASONALFIT_NAME} PRIVATE cxx_std_20)

source_group("inc"			FILES ${SEASONALFIT_INCS})
source_group("src"			FILES ${SEASONALFIT_SOURCES})
source_group("inc\\td"		FILES ${SEASONALFIT_INC_TD})
source_group("inc\\gui"		FILES ${SEASONALFIT_INC_GUI})

target_link_libraries(${SEASONALFIT_NAME} debug ${MU_LIB_DEBUG} debug ${NATGUI_LIB_DEBUG}
										optimized ${MU_LIB_RELEASE} optimized ${NATGUI_LIB_RELEASE})

# The application builds the CSV path from PROJECT_DATA_DIR (see MainView.h),
# so point it at this project's data/ folder.
target_compile_definitions(${SEASONALFIT_NAME} PUBLIC PROJECT_DATA_DIR="${CMAKE_CURRENT_LIST_DIR}/data")

setTargetPropertiesForGUIApp(${SEASONALFIT_NAME} ${SEASONALFIT_PLIST})

setIDEPropertiesForGUIExecutable(${SEASONALFIT_NAME} ${CMAKE_CURRENT_LIST_DIR})

setPlatformDLLPath(${SEASONALFIT_NAME})
