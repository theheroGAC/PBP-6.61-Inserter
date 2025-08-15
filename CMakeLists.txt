cmake_minimum_required(VERSION 3.10)  # Aggiornata la versione minima richiesta

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VITASDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
  else()
    message(FATAL_ERROR "Please define VITASDK environment variable pointing to your SDK path!")
  endif()
endif()

project(PSP_PBP_Inserter VERSION 1.0 LANGUAGES C CXX)

include("${VITASDK}/share/vita.cmake" REQUIRED)

set(VITA_APP_NAME "PBP 6.61 Inserter")
set(VITA_TITLEID "PBPIN6610")
set(VITA_VERSION "01.00")

# Flags C e C++
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wl,-q -Wall -fno-lto")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

include_directories(${PROJECT_SOURCE_DIR}/src)

# Solo main.c esistente
add_executable(${PROJECT_NAME}
  src/main.c
)

# Link VitaSDK libraries
target_link_libraries(${PROJECT_NAME}
  vita2d
  z
  c
  m
  png
  SceGxm_stub
  SceSysmodule_stub
  ScePgf_stub
  ScePvf_stub
  SceCtrl_stub
  SceCommonDialog_stub
  ScePower_stub
  SceLibKernel_stub
  SceIofilemgr_stub
  SceAppMgr_stub
  SceDisplay_stub
)

# Crea EBOOT
vita_create_self(eboot.bin ${PROJECT_NAME} UNSAFE)

# Crea VPK e includi 661.PBP e i file livearea
vita_create_vpk(${PROJECT_NAME}.vpk ${VITA_TITLEID} eboot.bin
  VERSION ${VITA_VERSION}
  NAME ${VITA_APP_NAME}

  FILE sce_sys/icon0.png sce_sys/icon0.png
  FILE sce_sys/livearea/contents/bg.png sce_sys/livearea/contents/bg.png
  FILE sce_sys/livearea/contents/template.xml sce_sys/livearea/contents/template.xml
  FILE resources/661.PBP 661.PBP
)
