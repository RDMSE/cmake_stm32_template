# STM32 HAL driver sources of the target family (HAL_DIR comes from the main CMakeLists.txt)
file(GLOB HAL_SOURCES ${HAL_DIR}/Src/*.c)
# ST ships *_template.c files next to the drivers to be copied into the
# application; building them as-is overrides HAL_InitTick() with a TIM6 time base
list(FILTER HAL_SOURCES EXCLUDE REGEX "_template\\.c$")
