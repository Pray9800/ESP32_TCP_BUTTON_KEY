# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
 


cmake_minimum_required(VERSION 3.22)
# 1. 告诉系统：我除了 main 之外，还有 APP 和 BSP 这两个自建的组件目录
set(EXTRA_COMPONENT_DIRS "APP" "BSP")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(JT004-MARKER-ESP32-V1.0)
