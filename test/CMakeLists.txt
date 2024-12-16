add_executable(test_camera
	test_camera.cpp
)

target_link_libraries(test_camera
	pco
)

add_subdirectory(tango)