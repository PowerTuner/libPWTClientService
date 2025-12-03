cmake_minimum_required(VERSION 3.23)

project(PWTClientService VERSION 1.0 DESCRIPTION "Shared library for PowerTuner clients to connect to PowerTuner daemon" LANGUAGES CXX)

option(DEV_BUILD_SETUP "Enable options to build the library stand-alone for development" OFF)

set(PROJECT_AUTHOR "kylon")
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 6.10 REQUIRED COMPONENTS Core Network)

configure_file(pwtClientService/Resources/version.h.in ${CMAKE_CURRENT_SOURCE_DIR}/version.h)

set(PROJECT_SOURCES
	pwtClientService/serviceExport.h
	pwtClientService/ClientServiceCmdTimer.h
	pwtClientService/ServiceWorker.cpp
	pwtClientService/ServiceWorker.h
	pwtClientService/ClientService.h
	pwtClientService/ClientService.cpp
)

if (WIN32)
	configure_file(pwtClientService/Resources/Windows/win.rc.in ${CMAKE_CURRENT_SOURCE_DIR}/win.rc)

	list(APPEND PROJECT_SOURCES
		win.rc
	)
endif ()

if (DEV_BUILD_SETUP)
	include(FetchContent)

	set(FETCHCONTENT_QUIET FALSE)

	FetchContent_Declare(
		libpwtshared
		GIT_REPOSITORY https://github.com/PowerTuner/libPWTShared
		GIT_TAG origin/main
		GIT_PROGRESS TRUE
	)

	FetchContent_MakeAvailable(libpwtshared)
endif ()

add_library(${PROJECT_NAME} SHARED ${PROJECT_SOURCES})
add_library("PWT::ClientService" ALIAS ${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME} PRIVATE Qt::Core Qt::Network PWT::Shared)
target_compile_definitions(${PROJECT_NAME} PRIVATE PWTCSERVICE_LIBRARY)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

include(CheckIPOSupported)
check_ipo_supported(RESULT has_ipo OUTPUT ipo_error)
if (has_ipo)
	message(STATUS "${PROJECT_NAME}: IPO/LTO enabled")
	set_property(TARGET ${PROJECT_NAME} PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif ()

set_target_properties(${PROJECT_NAME} PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION}
    CXX_VISIBLITY_PRESET hidden
    VISIBILITY_INLINE_HIDDEN TRUE
)

include(GNUInstallDirs)
install(TARGETS ${PROJECT_NAME}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
