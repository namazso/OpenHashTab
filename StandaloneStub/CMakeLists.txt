project(StandaloneStub)

add_executable(${PROJECT_NAME} StandaloneStub.cpp StandaloneStub.rc)

target_link_libraries(${PROJECT_NAME} OpenHashTab)
target_link_options(${PROJECT_NAME} PRIVATE /subsystem:windows)
