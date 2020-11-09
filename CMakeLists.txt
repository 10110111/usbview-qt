cmake_minimum_required(VERSION 3.10.0)
project(usbview-qt VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(${CMAKE_CXX_COMPILER_ID} MATCHES "GNU|Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror=return-type -Werror=format -Wall -Wextra")
endif()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
find_package(Qt5 5.9 REQUIRED Core Widgets)

add_executable(usbview-qt
    main.cpp
    Device.cpp
    DeviceTreeWidget.cpp
    PropertiesWidget.cpp
    )

target_link_libraries(usbview-qt Qt5::Core Qt5::Widgets)