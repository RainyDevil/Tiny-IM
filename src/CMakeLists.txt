set(SOURCES
    main.cpp
    Server.cpp
    Session.cpp
    BusinessHandler.cpp
    Message.cpp
)

add_executable(ChatServer ${SOURCES})


find_package(Boost 1.70 REQUIRED)
if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
    target_link_libraries(ChatServer ${Boost_LIBRARIES})
endif()
