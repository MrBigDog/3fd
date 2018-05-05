include(vcpkg_common_functions)

# Check architecture:
if (VCPKG_TARGET_ARCHITECTURE MATCHES "x86")
    set(BUILD_ARCH "Win32")
elseif (VCPKG_TARGET_ARCHITECTURE MATCHES "x64")
    set(BUILD_ARCH "x64")
elseif (VCPKG_TARGET_ARCHITECTURE MATCHES "arm")
    set(BUILD_ARCH "ARM")
else()
    message(FATAL_ERROR "Unsupported architecture: ${VCPKG_TARGET_ARCHITECTURE}")
endif()

# Check library linkage:
if (NOT VCPKG_LIBRARY_LINKAGE MATCHES "static")
    message(FATAL_ERROR "Unsupported library linkage: ${VCPKG_LIBRARY_LINKAGE}. 3FD can only be built as a static library!")
endif()

# Check CRT linkage:
if (NOT VCPKG_CRT_LINKAGE MATCHES "dynamic")
    message(FATAL_ERROR "Unsupported CRT linkage: ${VCPKG_CRT_LINKAGE}. 3FD can only be built with dynamic linkage to CRT!")
endif()

# Get source code:
set(SOURCE_PATH ${CURRENT_BUILDTREES_DIR}/src/3fd-2.5.4)
vcpkg_from_github(
	OUT_SOURCE_PATH SOURCE_PATH
    REPO faburaya/3FD
	HEAD_REF work
	REF work
	SHA512 1
)

# Build:
message(STATUS "Building...")
if (MSVC) # Microsoft Visual C++:
	if (VCPKG_CMAKE_SYSTEM_NAME MATCHES "WindowsStore")
		vcpkg_build_msbuild(
			USE_VCPKG_INTEGRATION
			PROJECT_PATH ${SOURCE_PATH}/3FD/3FD.WinRT.UWP.vcxproj
			PLATFORM ${BUILD_ARCH}
		)
	else()
		vcpkg_build_msbuild(
			USE_VCPKG_INTEGRATION
			PROJECT_PATH ${SOURCE_PATH}/3FD/3FD.vcxproj
			PLATFORM ${BUILD_ARCH}
			TARGET Build
		)
	endif()
else() # POSIX:
	vcpkg_configure_cmake(
		SOURCE_PATH ${SOURCE_PATH}/3FD
		PREFER_NINJA
		OPTIONS -DCMAKE_DEBUG_POSTFIX=d
				-DCMAKE_C_COMPILER=clang
				-DCMAKE_CXX_COMPILER=clang++
				-DCMAKE_CXX_FLAGS="-std=c++11"
	)

	vcpkg_install_cmake()
endif (MSVC)

# Install:
message(STATUS "Installing...")

file(GLOB HEADER_FILES LIST_DIRECTORIES false "${SOURCE_PATH}/3FD/*.h")
file(INSTALL
	${HEADER_FILES}
	DESTINATION ${CURRENT_PACKAGES_DIR}/include/3FD/3FD
	PATTERN "*_impl*.h" EXCLUDE
)

file(INSTALL ${SOURCE_PATH}/btree  DESTINATION ${CURRENT_PACKAGES_DIR}/include/3FD)
file(INSTALL ${SOURCE_PATH}/OpenCL DESTINATION ${CURRENT_PACKAGES_DIR}/include/3FD)

file(MAKE_DIRECTORY ${CURRENT_PACKAGES_DIR}/share/3FD)
file(INSTALL
	${SOURCE_PATH}/3FD/3fd-config-template.xml
	DESTINATION ${CURRENT_PACKAGES_DIR}/share/3FD
)

if (VCPKG_CMAKE_SYSTEM_NAME MATCHES "WindowsStore") # Visual C++, UWP app:
    file(INSTALL
		${SOURCE_PATH}/3FD/${BUILD_ARCH}/Debug/3FD.WinRT.UWP.lib
		${SOURCE_PATH}/3FD/${BUILD_ARCH}/Debug/3FD.WinRT.UWP.pdb
		${SOURCE_PATH}/3FD/${BUILD_ARCH}/Debug/_3FD_WinRT_UWP.pri
		DESTINATION ${CURRENT_PACKAGES_DIR}/debug/lib
	)
    file(INSTALL
		${SOURCE_PATH}/3FD/${BUILD_ARCH}/Release/3FD.WinRT.UWP.lib
		${SOURCE_PATH}/3FD/${BUILD_ARCH}/Release/3FD.WinRT.UWP.pdb
		${SOURCE_PATH}/3FD/${BUILD_ARCH}/Release/_3FD_WinRT_UWP.pri
		DESTINATION ${CURRENT_PACKAGES_DIR}/lib
	)
elseif (MSVC) # Visual C++, Win32 app:
    file(INSTALL
		${SOURCE_PATH}/3FD/${BUILD_ARCH}/Debug/3FD.lib
		${SOURCE_PATH}/3FD/${BUILD_ARCH}/Debug/3FD.pdb
		DESTINATION ${CURRENT_PACKAGES_DIR}/debug/lib
	)
    file(INSTALL
		${SOURCE_PATH}/3FD/${BUILD_ARCH}/Release/3FD.lib
		${SOURCE_PATH}/3FD/${BUILD_ARCH}/Release/3FD.pdb
		DESTINATION ${CURRENT_PACKAGES_DIR}/lib
	)
else() # POSIX:
    file(INSTALL
		${SOURCE_PATH}/build/lib/lib3FDd.a
		DESTINATION ${CURRENT_PACKAGES_DIR}/debug/lib
	)
    file(INSTALL
		${SOURCE_PATH}/build/lib/lib3FD.a
		DESTINATION ${CURRENT_PACKAGES_DIR}/lib
	)
endif()

# Handle copyright
file(INSTALL ${SOURCE_PATH}/LICENSE DESTINATION ${CURRENT_PACKAGES_DIR}/share/3fd RENAME copyright)
file(INSTALL ${SOURCE_PATH}/Acknowledgements.txt DESTINATION ${CURRENT_PACKAGES_DIR}/share/3fd)
