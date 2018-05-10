
add_executable (
	Paint

	main.cpp
)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(Paint glad)
target_link_libraries(Paint glfw)
