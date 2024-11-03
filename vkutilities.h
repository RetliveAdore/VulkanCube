#ifndef _INCLUDE_VK_UTILITIES_H_
#define _INCLUDE_VK_UTILITIES_H_

#define VK_USE_PLATFORM_WIN32_KHR

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <signal.h>
#include <windows.h>

// #pragma comment(linker, "/subsystem:windows")
#define APP_NAME_STR_LEN 80

#include <vulkan/vulkan.h>

// #include <vulkan/vk_sdk_platform.h>
#include "linmath.h"

#define DEMO_TEXTURE_COUNT 1
#define APP_SHORT_NAME "cube"
#define APP_LONG_NAME "The Vulkan Cube Demo Program"

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

#if defined(NDEBUG) && defined(__GNUC__)
#define U_ASSERT_ONLY __attribute__((unused))
#else
#define U_ASSERT_ONLY
#endif

#endif