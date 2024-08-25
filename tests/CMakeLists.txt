set(TEST_SOURCES
    test_client.cpp
)

include_directories(${PROJECT_SOURCE_DIR}/inc)

add_executable(TestClient ${TEST_SOURCES})

# Á´½ÓBoost.Asio¿â
find_package(Boost 1.70 REQUIRED)
if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
    target_link_libraries(TestClient ${Boost_LIBRARIES})
endif()

add_test(NAME TestClient COMMAND TestClient)
