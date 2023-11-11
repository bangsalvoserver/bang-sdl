add_subdirectory(gamescene)
add_subdirectory(scenes)
add_subdirectory(widgets)

target_sources(bangclient PRIVATE
    config.cpp
    image_serial.cpp
    intl.cpp
    entrypoint.cpp
    manager.cpp
    chat_ui.cpp
    media_pak.cpp
    sounds_pak.cpp
    os_api.cpp
    wsconnection.cpp
)