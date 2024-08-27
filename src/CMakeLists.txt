set(SOURCES
    main.cpp
    Server.cpp
    Session.cpp
    BusinessHandler.cpp
    Message.cpp
    Database.cpp
)

add_executable(ChatServer ${SOURCES})

find_package(OpenSSL REQUIRED)

find_package(SQLite3 REQUIRED)

find_package(Boost 1.70 REQUIRED)

target_link_libraries(ChatServer SQLite::SQLite3)
target_link_libraries(ChatServer OpenSSL::Crypto)

if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
    target_link_libraries(ChatServer ${Boost_LIBRARIES})
endif()
