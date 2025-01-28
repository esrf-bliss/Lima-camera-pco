add_executable(test_camera
	test_camera.cpp
)

target_link_libraries(test_camera
	pco
)

# Test app

add_executable(lima_pco
	test_pco.cpp
)

target_link_libraries(lima_pco PUBLIC limacore pco)

# target_compile_options(lima_pco PUBLIC -fsanitize=undefined PUBLIC -fsanitize=address)
# target_link_options(lima_pco BEFORE PUBLIC -fsanitize=undefined PUBLIC -fsanitize=address)

install(TARGETS lima_pco)

# Tango

#add_subdirectory(tango)