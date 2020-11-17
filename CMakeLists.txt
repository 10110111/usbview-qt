cmake_minimum_required(VERSION 3.10.0)
project(usbview-qt VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(${CMAKE_CXX_COMPILER_ID} MATCHES "GNU|Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror=return-type -Werror=format -Wall -Wextra")
endif()
if(${CMAKE_CXX_COMPILER_ID} MATCHES "GNU")
	set(minReqGCCVer 8.0)
	if(${CMAKE_CXX_COMPILER_VERSION} VERSION_LESS ${minReqGCCVer})
		message(FATAL "g++ older than ${minReqGCCVer} won't work")
	endif()
endif()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
find_package(Qt5 5.10 REQUIRED Core Widgets)
include(FindPkgConfig)
pkg_check_modules(LIBUDEV REQUIRED libudev)

add_executable(usbview-qt
    main.cpp
    usbids.cpp
    Device.cpp
    MainWindow.cpp
    DeviceTree.cpp
	ExtDescription.cpp
    DeviceTreeWidget.cpp
    PropertiesWidget.cpp
    HIDReportDescriptor.cpp
    )

target_link_libraries(usbview-qt Qt5::Core Qt5::Widgets ${LIBUDEV_LIBRARIES} stdc++fs)
