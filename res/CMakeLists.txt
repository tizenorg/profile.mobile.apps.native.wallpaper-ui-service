message("!!! ADD_CUSTOM_TARGET")
ADD_CUSTOM_TARGET(popup-wallpaper.edj
		COMMAND edje_cc -id ${PROJECT_SOURCE_DIR}/edje/images
		-sd ${PROJECT_SOURCE_DIR}/edje/sounds
		${CMAKE_CURRENT_SOURCE_DIR}/edje/popup-wallpaper.edc
		${CMAKE_CURRENT_BINARY_DIR}/edje/popup-wallpaper.edj
		DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/edje/popup-wallpaper.edc
)
message("!!! ADD_DEPENDENCIES")
ADD_DEPENDENCIES(${PROJECT_NAME} popup-wallpaper.edj)

INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/edje/popup-wallpaper.edj DESTINATION ${EDJDIR})

ADD_CUSTOM_TARGET(button_customized_theme.edj
		COMMAND edje_cc -id ${PROJECT_SOURCE_DIR}/edje/images
		-sd ${PROJECT_SOURCE_DIR}/edje/sounds
		${CMAKE_CURRENT_SOURCE_DIR}/edje/button_customized_theme.edc
		${CMAKE_CURRENT_BINARY_DIR}/edje/button_customized_theme.edj
		DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/edje/button_customized_theme.edc
)
ADD_DEPENDENCIES(${PROJECT_NAME} button_customized_theme.edj)

INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/edje/button_customized_theme.edj DESTINATION ${EDJDIR})

INSTALL(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/icons DESTINATION ${RESDIR} FILES_MATCHING PATTERN "*.png")
INSTALL(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/icons DESTINATION ${RESDIR} FILES_MATCHING PATTERN "*.PNG")
INSTALL(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/icons DESTINATION ${RESDIR} FILES_MATCHING PATTERN "*.gif")


