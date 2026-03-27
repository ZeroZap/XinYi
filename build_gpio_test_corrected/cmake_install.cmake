# Install script for directory: /home/eugene/zerozap/XinYi

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "1")
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

# Set default install directory permissions.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/usr/bin/objdump")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/xy_framework" TYPE DIRECTORY FILES "/home/eugene/zerozap/XinYi/components/" FILES_MATCHING REGEX "/[^/]*\\.h$" REGEX "/test[^/]*$" EXCLUDE REGEX "/tests$" EXCLUDE)
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/xy_framework/unity" TYPE DIRECTORY FILES "/home/eugene/zerozap/XinYi/third_party/unity/" FILES_MATCHING REGEX "/[^/]*\\.h$")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("/home/eugene/zerozap/XinYi/build_gpio_test_corrected/third_party/cmake_install.cmake")
  include("/home/eugene/zerozap/XinYi/build_gpio_test_corrected/components/ADDC/cmake_install.cmake")
  include("/home/eugene/zerozap/XinYi/build_gpio_test_corrected/components/charger/cmake_install.cmake")
  include("/home/eugene/zerozap/XinYi/build_gpio_test_corrected/components/clib/cmake_install.cmake")
  include("/home/eugene/zerozap/XinYi/build_gpio_test_corrected/components/clib_disabled2/cmake_install.cmake")
  include("/home/eugene/zerozap/XinYi/build_gpio_test_corrected/components/crypto/cmake_install.cmake")
  include("/home/eugene/zerozap/XinYi/build_gpio_test_corrected/components/device/cmake_install.cmake")
  include("/home/eugene/zerozap/XinYi/build_gpio_test_corrected/components/driver/cmake_install.cmake")
  include("/home/eugene/zerozap/XinYi/build_gpio_test_corrected/components/drivers/cmake_install.cmake")
  include("/home/eugene/zerozap/XinYi/build_gpio_test_corrected/components/fota/cmake_install.cmake")
  include("/home/eugene/zerozap/XinYi/build_gpio_test_corrected/components/fuel_gauge/cmake_install.cmake")
  include("/home/eugene/zerozap/XinYi/build_gpio_test_corrected/components/hal/cmake_install.cmake")
  include("/home/eugene/zerozap/XinYi/build_gpio_test_corrected/components/ipc/cmake_install.cmake")
  include("/home/eugene/zerozap/XinYi/build_gpio_test_corrected/components/kernel/cmake_install.cmake")
  include("/home/eugene/zerozap/XinYi/build_gpio_test_corrected/components/mux/cmake_install.cmake")
  include("/home/eugene/zerozap/XinYi/build_gpio_test_corrected/components/net/cmake_install.cmake")
  include("/home/eugene/zerozap/XinYi/build_gpio_test_corrected/components/pid/cmake_install.cmake")
  include("/home/eugene/zerozap/XinYi/build_gpio_test_corrected/components/pm/cmake_install.cmake")
  include("/home/eugene/zerozap/XinYi/build_gpio_test_corrected/components/sys/cmake_install.cmake")
  include("/home/eugene/zerozap/XinYi/build_gpio_test_corrected/components/third_party/cmake_install.cmake")
  include("/home/eugene/zerozap/XinYi/build_gpio_test_corrected/components/trace/cmake_install.cmake")
  include("/home/eugene/zerozap/XinYi/build_gpio_test_corrected/components/kernel/osal/cmake_install.cmake")
  include("/home/eugene/zerozap/XinYi/build_gpio_test_corrected/components/kernel/misc/cmake_install.cmake")
  include("/home/eugene/zerozap/XinYi/build_gpio_test_corrected/components/clib/xy_clib/cmake_install.cmake")
  include("/home/eugene/zerozap/XinYi/build_gpio_test_corrected/tests/cmake_install.cmake")
  include("/home/eugene/zerozap/XinYi/build_gpio_test_corrected/examples/at_server_example/cmake_install.cmake")

endif()

if(CMAKE_INSTALL_COMPONENT)
  set(CMAKE_INSTALL_MANIFEST "install_manifest_${CMAKE_INSTALL_COMPONENT}.txt")
else()
  set(CMAKE_INSTALL_MANIFEST "install_manifest.txt")
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
file(WRITE "/home/eugene/zerozap/XinYi/build_gpio_test_corrected/${CMAKE_INSTALL_MANIFEST}"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
