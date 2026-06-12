set(CORE_SOURCES
    core/terminal.cpp
    core/buffer.cpp
    core/event.cpp
    core/renderer.cpp
    core/color.cpp
)

set(DESKTOP_SOURCES
    desktop/desktop.cpp
    desktop/window.cpp
    desktop/taskbar.cpp
    desktop/launcher.cpp
    desktop/workspace.cpp
    desktop/command_panel.cpp
)

set(AUTH_SOURCES
    auth/login.cpp
    auth/pam_auth.cpp
    auth/shadow_auth.cpp
)

set(INPUT_SOURCES
    input/keyboard.cpp
    input/mouse.cpp
    input/term_detect.cpp
)

set(WIDGET_SOURCES
    widgets/button.cpp
    widgets/textbox.cpp
    widgets/listbox.cpp
    widgets/menu.cpp
)

set(NOTIFICATION_SOURCES
    notification/notify.cpp
    notification/dbus_listener.cpp
)

set(UTIL_SOURCES
    utils/config.cpp
    utils/logger.cpp
    utils/helpers.cpp
)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

add_executable(tdesktop
    main.cpp
    ${CORE_SOURCES}
    ${DESKTOP_SOURCES}
    ${AUTH_SOURCES}
    ${INPUT_SOURCES}
    ${WIDGET_SOURCES}
    ${NOTIFICATION_SOURCES}
    ${UTIL_SOURCES}
)

target_include_directories(tdesktop PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(tdesktop PRIVATE PAM::PAM crypt)

if(USE_NCURSES)
    find_package(Curses REQUIRED)
    target_compile_definitions(tdesktop PRIVATE USE_NCURSES)
    target_link_libraries(tdesktop PRIVATE ${CURSES_LIBRARIES})
endif()

if(USE_LIBNOTIFY)
    find_package(PkgConfig)
    pkg_check_modules(NOTIFY REQUIRED libnotify)
    target_compile_definitions(tdesktop PRIVATE USE_LIBNOTIFY)
    target_include_directories(tdesktop PRIVATE ${NOTIFY_INCLUDE_DIRS})
    target_link_libraries(tdesktop PRIVATE ${NOTIFY_LIBRARIES})
endif()

if(USE_LIBX11)
    find_package(X11 REQUIRED)
    target_compile_definitions(tdesktop PRIVATE USE_LIBX11)
    target_include_directories(tdesktop PRIVATE ${X11_INCLUDE_DIR})
    target_link_libraries(tdesktop PRIVATE ${X11_LIBRARIES})
endif()
