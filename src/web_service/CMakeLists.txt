set(SRCS
            announce_room_json.cpp
            telemetry_json.cpp
            verify_login.cpp
            web_backend.cpp
            )

set(HEADERS
            announce_room_json.h
            telemetry_json.h
            verify_login.h
            web_backend.h
            )

create_directory_groups(${SRCS} ${HEADERS})

add_library(web_service STATIC ${SRCS} ${HEADERS})
target_link_libraries(web_service PUBLIC common cpr json-headers)
