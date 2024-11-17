#include "vkutilities.h"

void ERR_EXIT(const char* err_msg, const char* err_class)
{
    MessageBox(NULL, err_msg, err_class, MB_OK);
    exit(1);
}

VKAPI_ATTR VkBool32 VKAPI_CALL
dbgFunc(VkFlags msgFlags, VkDebugReportObjectTypeEXT objType,
        uint64_t srcObject, size_t location, int32_t msgCode,
        const char *pLayerPrefix, const char *pMsg, void *pUserData) {
    char *message = (char *)malloc(strlen(pMsg) + 100);

    assert(message);

    if (msgFlags & VK_DEBUG_REPORT_ERROR_BIT_EXT) {
        sprintf(message, "ERROR: [%s] Code %d : %s", pLayerPrefix, msgCode,
                pMsg);
    } else if (msgFlags & VK_DEBUG_REPORT_WARNING_BIT_EXT) {
        // We know that we're submitting queues without fences, ignore this
        // warning
        if (strstr(pMsg,
                   "vkQueueSubmit parameter, VkFence fence, is null pointer")) {
            return false;
        }
        sprintf(message, "WARNING: [%s] Code %d : %s", pLayerPrefix, msgCode,
                pMsg);
    } else {
        return false;
    }

    MessageBox(NULL, message, "Alert", MB_OK);
    free(message);

    /*
     * false indicates that layer should not bail-out of an
     * API call that had validation failures. This may mean that the
     * app dies inside the driver due to invalid parameter(s).
     * That's what would happen without validation layers, so we'll
     * keep that behavior here.
     */
    return false;
}

/*
 * Return 1 (true) if all layer names specified in check_names
 * can be found in given layer properties.
 */
static VkBool32 demo_check_layers(uint32_t check_count, char **check_names,
                                  uint32_t layer_count,
                                  VkLayerProperties *layers) {
    for (uint32_t i = 0; i < check_count; i++) {
        VkBool32 found = 0;
        for (uint32_t j = 0; j < layer_count; j++) {
            if (!strcmp(check_names[i], layers[j].layerName)) {
                found = 1;
                break;
            }
        }
        if (!found) {
            fprintf(stderr, "Cannot find layer: %s\n", check_names[i]);
            return 0;
        }
    }
    return 1;
}

/**
 * 对vulkan的一些必要项目进行初始化和配置检查
 */
void demo_init_vk(struct cr_vk_inner *cr_vk_inner) {
    VkResult err;
    char *extension_names[64];
    uint32_t instance_extension_count = 0;
    uint32_t instance_layer_count = 0;
    uint32_t device_validation_layer_count = 0;
    uint32_t enabled_extension_count = 0;
    uint32_t enabled_layer_count = 0;

    char *instance_validation_layers[] = {
        "VK_LAYER_LUNARG_threading",      "VK_LAYER_LUNARG_mem_tracker",
        "VK_LAYER_LUNARG_object_tracker", "VK_LAYER_LUNARG_draw_state",
        "VK_LAYER_LUNARG_param_checker",  "VK_LAYER_LUNARG_swapchain",
        "VK_LAYER_LUNARG_device_limits",  "VK_LAYER_LUNARG_image",
        "VK_LAYER_GOOGLE_unique_objects",
    };

    cr_vk_inner->device_validation_layers[0] = "VK_LAYER_LUNARG_threading";
    cr_vk_inner->device_validation_layers[1] = "VK_LAYER_LUNARG_mem_tracker";
    cr_vk_inner->device_validation_layers[2] = "VK_LAYER_LUNARG_object_tracker";
    cr_vk_inner->device_validation_layers[3] = "VK_LAYER_LUNARG_draw_state";
    cr_vk_inner->device_validation_layers[4] = "VK_LAYER_LUNARG_param_checker";
    cr_vk_inner->device_validation_layers[5] = "VK_LAYER_LUNARG_swapchain";
    cr_vk_inner->device_validation_layers[6] = "VK_LAYER_LUNARG_device_limits";
    cr_vk_inner->device_validation_layers[7] = "VK_LAYER_LUNARG_image";
    cr_vk_inner->device_validation_layers[8] = "VK_LAYER_GOOGLE_unique_objects";
    device_validation_layer_count = 9;

    /* Look for validation layers */
    VkBool32 validation_found = 0;
    err = vkEnumerateInstanceLayerProperties(&instance_layer_count, NULL);
    assert(!err);

    if (instance_layer_count > 0) {
        VkLayerProperties *instance_layers =
            malloc(sizeof(VkLayerProperties) * instance_layer_count);
        err = vkEnumerateInstanceLayerProperties(&instance_layer_count,
                                                 instance_layers);
        assert(!err);

        if (cr_vk_inner->validate) {
            validation_found = demo_check_layers(
                ARRAY_SIZE(instance_validation_layers),
                instance_validation_layers, instance_layer_count,
                instance_layers);

            enabled_layer_count = ARRAY_SIZE(instance_validation_layers);
        }

        free(instance_layers);
    }

    if (cr_vk_inner->validate && !validation_found) {
        ERR_EXIT("vkEnumerateInstanceLayerProperties failed to find"
                 "required validation layer.\n\n"
                 "Please look at the Getting Started guide for additional "
                 "information.\n",
                 "vkCreateInstance Failure");
    }

    /* Look for instance extensions */
    VkBool32 surfaceExtFound = 0;
    VkBool32 platformSurfaceExtFound = 0;
    memset(extension_names, 0, sizeof(extension_names));

    err = vkEnumerateInstanceExtensionProperties(
        NULL, &instance_extension_count, NULL);
    assert(!err);

    if (instance_extension_count > 0) {
        VkExtensionProperties *instance_extensions =
            malloc(sizeof(VkExtensionProperties) * instance_extension_count);
        err = vkEnumerateInstanceExtensionProperties(
            NULL, &instance_extension_count, instance_extensions);
        assert(!err);
        for (uint32_t i = 0; i < instance_extension_count; i++) {
            if (!strcmp(VK_KHR_SURFACE_EXTENSION_NAME,
                        instance_extensions[i].extensionName)) {
                surfaceExtFound = 1;
                extension_names[enabled_extension_count++] =
                    VK_KHR_SURFACE_EXTENSION_NAME;
            }
            if (!strcmp(VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
                        instance_extensions[i].extensionName)) {
                platformSurfaceExtFound = 1;
                extension_names[enabled_extension_count++] =
                    VK_KHR_WIN32_SURFACE_EXTENSION_NAME;
            }
            if (!strcmp(VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
                        instance_extensions[i].extensionName)) {
                if (cr_vk_inner->validate) {
                    extension_names[enabled_extension_count++] =
                        VK_EXT_DEBUG_REPORT_EXTENSION_NAME;
                }
            }
            assert(enabled_extension_count < 64);
        }

        free(instance_extensions);
    }

    if (!surfaceExtFound) {
        ERR_EXIT("vkEnumerateInstanceExtensionProperties failed to find "
                 "the " VK_KHR_SURFACE_EXTENSION_NAME
                 " extension.\n\nDo you have a compatible "
                 "Vulkan installable client driver (ICD) installed?\nPlease "
                 "look at the Getting Started guide for additional "
                 "information.\n",
                 "vkCreateInstance Failure");
    }
    if (!platformSurfaceExtFound) {
        ERR_EXIT("vkEnumerateInstanceExtensionProperties failed to find "
                 "the " VK_KHR_WIN32_SURFACE_EXTENSION_NAME
                 " extension.\n\nDo you have a compatible "
                 "Vulkan installable client driver (ICD) installed?\nPlease "
                 "look at the Getting Started guide for additional "
                 "information.\n",
                 "vkCreateInstance Failure");
    }
    const VkApplicationInfo app = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext = NULL,
        .pApplicationName = APP_SHORT_NAME,
        .applicationVersion = 0,
        .pEngineName = APP_SHORT_NAME,
        .engineVersion = 0,
        .apiVersion = VK_API_VERSION,
    };
    VkInstanceCreateInfo inst_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = NULL,
        .pApplicationInfo = &app,
        .enabledLayerCount = enabled_layer_count,
        .ppEnabledLayerNames =
            (const char *const *)((cr_vk_inner->validate) ? instance_validation_layers
                                                   : NULL),
        .enabledExtensionCount = enabled_extension_count,
        .ppEnabledExtensionNames = (const char *const *)extension_names,
    };

    uint32_t gpu_count;

    err = vkCreateInstance(&inst_info, NULL, &cr_vk_inner->inst);
    if (err == VK_ERROR_INCOMPATIBLE_DRIVER) {
        ERR_EXIT("Cannot find a compatible Vulkan installable client driver "
                 "(ICD).\n\nPlease look at the Getting Started guide for "
                 "additional information.\n",
                 "vkCreateInstance Failure");
    } else if (err == VK_ERROR_EXTENSION_NOT_PRESENT) {
        ERR_EXIT("Cannot find a specified extension library"
                 ".\nMake sure your layers path is set appropriately.\n",
                 "vkCreateInstance Failure");
    } else if (err) {
        ERR_EXIT("vkCreateInstance failed.\n\nDo you have a compatible Vulkan "
                 "installable client driver (ICD) installed?\nPlease look at "
                 "the Getting Started guide for additional information.\n",
                 "vkCreateInstance Failure");
    }

    /* Make initial call to query gpu_count, then second call for gpu info*/
    err = vkEnumeratePhysicalDevices(cr_vk_inner->inst, &gpu_count, NULL);
    assert(!err && gpu_count > 0);
    
    printf("GPU COUNT %d\n", gpu_count);

    if (gpu_count > 0) {
        VkPhysicalDevice *physical_devices = malloc(sizeof(VkPhysicalDevice) * gpu_count);
        err = vkEnumeratePhysicalDevices(cr_vk_inner->inst, &gpu_count, physical_devices);
        assert(!err);
        /* For cube cr_vk_inner we just grab the first physical device */
        cr_vk_inner->gpu = physical_devices[0]; // BUG BUG BUG!!!
        free(physical_devices);
    }
    else
    {
        ERR_EXIT("vkEnumeratePhysicalDevices reported zero accessible devices.\n\n"
                 "Do you have a compatible Vulkan installable client driver (ICD) "
                 "installed?\nPlease look at the Getting Started guide for "
                 "additional information.\n",
                 "vkEnumeratePhysicalDevices Failure");
    }

    /* Look for validation layers */
    validation_found = 0;
    cr_vk_inner->enabled_layer_count = 0;
    uint32_t device_layer_count = 0;
    err = vkEnumerateDeviceLayerProperties(cr_vk_inner->gpu, &device_layer_count, NULL);
    assert(!err);

    if (device_layer_count > 0) {
        VkLayerProperties *device_layers =
            malloc(sizeof(VkLayerProperties) * device_layer_count);
        err = vkEnumerateDeviceLayerProperties(cr_vk_inner->gpu, &device_layer_count,
                                               device_layers);
        assert(!err);

        if (cr_vk_inner->validate) {
            validation_found = demo_check_layers(device_validation_layer_count,
                                                 cr_vk_inner->device_validation_layers,
                                                 device_layer_count,
                                                 device_layers);
            cr_vk_inner->enabled_layer_count = device_validation_layer_count;
        }

        free(device_layers);
    }

    if (cr_vk_inner->validate && !validation_found) {
        ERR_EXIT("vkEnumerateDeviceLayerProperties failed to find"
                 "a required validation layer.\n\n"
                 "Please look at the Getting Started guide for additional "
                 "information.\n",
                 "vkCreateDevice Failure");
    }

    /* Look for device extensions */
    uint32_t device_extension_count = 0;
    VkBool32 swapchainExtFound = 0;
    cr_vk_inner->enabled_extension_count = 0;
    memset(extension_names, 0, sizeof(extension_names));

    err = vkEnumerateDeviceExtensionProperties(cr_vk_inner->gpu, NULL,
                                               &device_extension_count, NULL);
    assert(!err);

    if (device_extension_count > 0) {
        VkExtensionProperties *device_extensions =
            malloc(sizeof(VkExtensionProperties) * device_extension_count);
        err = vkEnumerateDeviceExtensionProperties(
            cr_vk_inner->gpu, NULL, &device_extension_count, device_extensions);
        assert(!err);

        for (uint32_t i = 0; i < device_extension_count; i++) {
            if (!strcmp(VK_KHR_SWAPCHAIN_EXTENSION_NAME,
                        device_extensions[i].extensionName)) {
                swapchainExtFound = 1;
                cr_vk_inner->extension_names[cr_vk_inner->enabled_extension_count++] =
                    VK_KHR_SWAPCHAIN_EXTENSION_NAME;
            }
            assert(cr_vk_inner->enabled_extension_count < 64);
        }

        free(device_extensions);
    }

    if (!swapchainExtFound) {
        ERR_EXIT("vkEnumerateDeviceExtensionProperties failed to find "
                 "the " VK_KHR_SWAPCHAIN_EXTENSION_NAME
                 " extension.\n\nDo you have a compatible "
                 "Vulkan installable client driver (ICD) installed?\nPlease "
                 "look at the Getting Started guide for additional "
                 "information.\n",
                 "vkCreateInstance Failure");
    }

    if (cr_vk_inner->validate) {
        cr_vk_inner->CreateDebugReportCallback =
            (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(
                cr_vk_inner->inst, "vkCreateDebugReportCallbackEXT");
        cr_vk_inner->DestroyDebugReportCallback =
            (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(
                cr_vk_inner->inst, "vkDestroyDebugReportCallbackEXT");
        if (!cr_vk_inner->CreateDebugReportCallback) {
            ERR_EXIT(
                "GetProcAddr: Unable to find vkCreateDebugReportCallbackEXT\n",
                "vkGetProcAddr Failure");
        }
        if (!cr_vk_inner->DestroyDebugReportCallback) {
            ERR_EXIT(
                "GetProcAddr: Unable to find vkDestroyDebugReportCallbackEXT\n",
                "vkGetProcAddr Failure");
        }
        cr_vk_inner->DebugReportMessage =
            (PFN_vkDebugReportMessageEXT)vkGetInstanceProcAddr(
                cr_vk_inner->inst, "vkDebugReportMessageEXT");
        if (!cr_vk_inner->DebugReportMessage) {
            ERR_EXIT("GetProcAddr: Unable to find vkDebugReportMessageEXT\n",
                     "vkGetProcAddr Failure");
        }

        PFN_vkDebugReportCallbackEXT callback;

        if (!cr_vk_inner->use_break) {
            callback = dbgFunc;
        }
        else
        {
            callback = dbgFunc;
            // TODO add a break callback defined locally since there is no
            // longer
            // one included in the loader
        }
        VkDebugReportCallbackCreateInfoEXT dbgCreateInfo;
        dbgCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
        dbgCreateInfo.pNext = NULL;
        dbgCreateInfo.pfnCallback = callback;
        dbgCreateInfo.pUserData = NULL;
        dbgCreateInfo.flags =
            VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
        err = cr_vk_inner->CreateDebugReportCallback(cr_vk_inner->inst, &dbgCreateInfo, NULL,
                                              &cr_vk_inner->msg_callback);
        switch (err) {
        case VK_SUCCESS:
            break;
        case VK_ERROR_OUT_OF_HOST_MEMORY:
            ERR_EXIT("CreateDebugReportCallback: out of host memory\n",
                     "CreateDebugReportCallback Failure");
            break;
        default:
            ERR_EXIT("CreateDebugReportCallback: unknown failure\n",
                     "CreateDebugReportCallback Failure");
            break;
        }
    }
    vkGetPhysicalDeviceProperties(cr_vk_inner->gpu, &cr_vk_inner->gpu_props);

    /* Call with NULL data to get count */
    vkGetPhysicalDeviceQueueFamilyProperties(cr_vk_inner->gpu, &cr_vk_inner->queue_count,
                                             NULL);
    assert(cr_vk_inner->queue_count >= 1);

    cr_vk_inner->queue_props = (VkQueueFamilyProperties *)malloc(
        cr_vk_inner->queue_count * sizeof(VkQueueFamilyProperties));
    vkGetPhysicalDeviceQueueFamilyProperties(cr_vk_inner->gpu, &cr_vk_inner->queue_count,
                                             cr_vk_inner->queue_props);
    // Find a queue that supports gfx
    uint32_t gfx_queue_idx = 0;
    for (gfx_queue_idx = 0; gfx_queue_idx < cr_vk_inner->queue_count;
         gfx_queue_idx++) {
        if (cr_vk_inner->queue_props[gfx_queue_idx].queueFlags & VK_QUEUE_GRAPHICS_BIT)
            break;
    }
    assert(gfx_queue_idx < cr_vk_inner->queue_count);
    // Query fine-grained feature support for this device.
    //  If app has specific feature requirements it should check supported
    //  features based on this query
    VkPhysicalDeviceFeatures physDevFeatures;
    vkGetPhysicalDeviceFeatures(cr_vk_inner->gpu, &physDevFeatures);

    GET_INSTANCE_PROC_ADDR(cr_vk_inner->inst, GetPhysicalDeviceSurfaceSupportKHR);
    GET_INSTANCE_PROC_ADDR(cr_vk_inner->inst, GetPhysicalDeviceSurfaceCapabilitiesKHR);
    GET_INSTANCE_PROC_ADDR(cr_vk_inner->inst, GetPhysicalDeviceSurfaceFormatsKHR);
    GET_INSTANCE_PROC_ADDR(cr_vk_inner->inst, GetPhysicalDeviceSurfacePresentModesKHR);
    GET_INSTANCE_PROC_ADDR(cr_vk_inner->inst, GetSwapchainImagesKHR);
}

/**
 * 创建逻辑设备
 */
void demo_create_device(struct cr_vk_inner *cr_vk_inner)
{
    VkResult U_ASSERT_ONLY err;
    float queue_priorities[1] = {0.0};
    const VkDeviceQueueCreateInfo queue = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .pNext = NULL,
        .queueFamilyIndex = cr_vk_inner->graphics_queue_node_index,
        .queueCount = 1,
        .pQueuePriorities = queue_priorities
    };

    VkDeviceCreateInfo device = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = NULL,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queue,
        .enabledLayerCount = cr_vk_inner->enabled_layer_count,
        .ppEnabledLayerNames =
            (const char *const *)((cr_vk_inner->validate)
                                      ? cr_vk_inner->device_validation_layers
                                      : NULL),
        .enabledExtensionCount = cr_vk_inner->enabled_extension_count,
        .ppEnabledExtensionNames = (const char *const *)cr_vk_inner->extension_names,
        .pEnabledFeatures =
            NULL, // If specific features are required, pass them in here
    };

    err = vkCreateDevice(cr_vk_inner->gpu, &device, NULL, &cr_vk_inner->device);
    assert(!err);
}

/**
 * 创建交换链
 */
void demo_init_vk_swapchain(struct cr_vk_inner *cr_vk_inner) {
    VkResult U_ASSERT_ONLY err;
    uint32_t i;

// Create a WSI surface for the window:
    VkWin32SurfaceCreateInfoKHR createInfo;
    createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    createInfo.pNext = NULL;
    createInfo.flags = 0;
    createInfo.hinstance = cr_vk_inner->connection;
    createInfo.hwnd = cr_vk_inner->window;

    err =
        vkCreateWin32SurfaceKHR(cr_vk_inner->inst, &createInfo, NULL, &cr_vk_inner->surface);

    // Iterate over each queue to learn whether it supports presenting:
    VkBool32 *supportsPresent =
        (VkBool32 *)malloc(cr_vk_inner->queue_count * sizeof(VkBool32));
    for (i = 0; i < cr_vk_inner->queue_count; i++) {
        cr_vk_inner->fpGetPhysicalDeviceSurfaceSupportKHR(cr_vk_inner->gpu, i, cr_vk_inner->surface,
                                                   &supportsPresent[i]);
    }

    // Search for a graphics and a present queue in the array of queue
    // families, try to find one that supports both
    uint32_t graphicsQueueNodeIndex = UINT32_MAX;
    uint32_t presentQueueNodeIndex = UINT32_MAX;
    for (i = 0; i < cr_vk_inner->queue_count; i++) {
        if ((cr_vk_inner->queue_props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0) {
            if (graphicsQueueNodeIndex == UINT32_MAX) {
                graphicsQueueNodeIndex = i;
            }

            if (supportsPresent[i] == VK_TRUE) {
                graphicsQueueNodeIndex = i;
                presentQueueNodeIndex = i;
                break;
            }
        }
    }
    if (presentQueueNodeIndex == UINT32_MAX) {
        // If didn't find a queue that supports both graphics and present, then
        // find a separate present queue.
        for (uint32_t i = 0; i < cr_vk_inner->queue_count; ++i) {
            if (supportsPresent[i] == VK_TRUE) {
                presentQueueNodeIndex = i;
                break;
            }
        }
    }
    free(supportsPresent);

    // Generate error if could not find both a graphics and a present queue
    if (graphicsQueueNodeIndex == UINT32_MAX ||
        presentQueueNodeIndex == UINT32_MAX) {
        ERR_EXIT("Could not find a graphics and a present queue\n",
                 "Swapchain Initialization Failure");
    }

    // TODO: Add support for separate queues, including presentation,
    //       synchronization, and appropriate tracking for QueueSubmit.
    // NOTE: While it is possible for an application to use a separate graphics
    //       and a present queues, this cr_vk_inner program assumes it is only using
    //       one:
    if (graphicsQueueNodeIndex != presentQueueNodeIndex) {
        ERR_EXIT("Could not find a common graphics and a present queue\n",
                 "Swapchain Initialization Failure");
    }

    cr_vk_inner->graphics_queue_node_index = graphicsQueueNodeIndex;

    demo_create_device(cr_vk_inner);

    GET_DEVICE_PROC_ADDR(cr_vk_inner->device, CreateSwapchainKHR);
    GET_DEVICE_PROC_ADDR(cr_vk_inner->device, DestroySwapchainKHR);
    GET_DEVICE_PROC_ADDR(cr_vk_inner->device, GetSwapchainImagesKHR);
    GET_DEVICE_PROC_ADDR(cr_vk_inner->device, AcquireNextImageKHR);
    GET_DEVICE_PROC_ADDR(cr_vk_inner->device, QueuePresentKHR);

    vkGetDeviceQueue(cr_vk_inner->device, cr_vk_inner->graphics_queue_node_index, 0,
                     &cr_vk_inner->queue);

    // Get the list of VkFormat's that are supported:
    uint32_t formatCount;
    err = cr_vk_inner->fpGetPhysicalDeviceSurfaceFormatsKHR(cr_vk_inner->gpu, cr_vk_inner->surface,
                                                     &formatCount, NULL);
    assert(!err);
    VkSurfaceFormatKHR *surfFormats =
        (VkSurfaceFormatKHR *)malloc(formatCount * sizeof(VkSurfaceFormatKHR));
    err = cr_vk_inner->fpGetPhysicalDeviceSurfaceFormatsKHR(cr_vk_inner->gpu, cr_vk_inner->surface,
                                                     &formatCount, surfFormats);
    assert(!err);
    // If the format list includes just one entry of VK_FORMAT_UNDEFINED,
    // the surface has no preferred format.  Otherwise, at least one
    // supported format will be returned.
    if (formatCount == 1 && surfFormats[0].format == VK_FORMAT_UNDEFINED) {
        cr_vk_inner->format = VK_FORMAT_B8G8R8A8_UNORM;
    } else {
        assert(formatCount >= 1);
        cr_vk_inner->format = surfFormats[0].format;
    }
    cr_vk_inner->color_space = surfFormats[0].colorSpace;

    cr_vk_inner->quit = false;
    cr_vk_inner->curFrame = 0;

    // Get Memory information and properties
    vkGetPhysicalDeviceMemoryProperties(cr_vk_inner->gpu, &cr_vk_inner->memory_properties);
}

void demo_prepare_buffers(struct cr_vk_inner *cr_vk_inner) {
    VkResult U_ASSERT_ONLY err;
    VkSwapchainKHR oldSwapchain = cr_vk_inner->swapchain;

    // Check the surface capabilities and formats
    VkSurfaceCapabilitiesKHR surfCapabilities;
    err = cr_vk_inner->fpGetPhysicalDeviceSurfaceCapabilitiesKHR(
        cr_vk_inner->gpu, cr_vk_inner->surface, &surfCapabilities);
    assert(!err);

    uint32_t presentModeCount;
    err = cr_vk_inner->fpGetPhysicalDeviceSurfacePresentModesKHR(
        cr_vk_inner->gpu, cr_vk_inner->surface, &presentModeCount, NULL);
    assert(!err);
    VkPresentModeKHR *presentModes =
        (VkPresentModeKHR *)malloc(presentModeCount * sizeof(VkPresentModeKHR));
    assert(presentModes);
    err = cr_vk_inner->fpGetPhysicalDeviceSurfacePresentModesKHR(
        cr_vk_inner->gpu, cr_vk_inner->surface, &presentModeCount, presentModes);
    assert(!err);

    VkExtent2D swapchainExtent;
    // width and height are either both -1, or both not -1.
    if (surfCapabilities.currentExtent.width == (uint32_t)-1) {
        // If the surface size is undefined, the size is set to
        // the size of the images requested.
        swapchainExtent.width = cr_vk_inner->width;
        swapchainExtent.height = cr_vk_inner->height;
    } else {
        // If the surface size is defined, the swap chain size must match
        swapchainExtent = surfCapabilities.currentExtent;
        cr_vk_inner->width = surfCapabilities.currentExtent.width;
        cr_vk_inner->height = surfCapabilities.currentExtent.height;
    }

    // If mailbox mode is available, use it, as is the lowest-latency non-
    // tearing mode.  If not, try IMMEDIATE which will usually be available,
    // and is fastest (though it tears).  If not, fall back to FIFO which is
    // always available.
    VkPresentModeKHR swapchainPresentMode = VK_PRESENT_MODE_FIFO_KHR;
    for (size_t i = 0; i < presentModeCount; i++) {
        if (presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
            swapchainPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
            break;
        }
        if ((swapchainPresentMode != VK_PRESENT_MODE_MAILBOX_KHR) &&
            (presentModes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR)) {
            swapchainPresentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
        }
    }

    // Determine the number of VkImage's to use in the swap chain (we desire to
    // own only 1 image at a time, besides the images being displayed and
    // queued for display):
    uint32_t desiredNumberOfSwapchainImages =
        surfCapabilities.minImageCount + 1;
    if ((surfCapabilities.maxImageCount > 0) &&
        (desiredNumberOfSwapchainImages > surfCapabilities.maxImageCount)) {
        // Application must settle for fewer images than desired:
        desiredNumberOfSwapchainImages = surfCapabilities.maxImageCount;
    }

    VkSurfaceTransformFlagsKHR preTransform;
    if (surfCapabilities.supportedTransforms &
        VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) {
        preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    } else {
        preTransform = surfCapabilities.currentTransform;
    }

    const VkSwapchainCreateInfoKHR swapchain = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .pNext = NULL,
        .surface = cr_vk_inner->surface,
        .minImageCount = desiredNumberOfSwapchainImages,
        .imageFormat = cr_vk_inner->format,
        .imageColorSpace = cr_vk_inner->color_space,
        .imageExtent =
            {
             .width = swapchainExtent.width, .height = swapchainExtent.height,
            },
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .preTransform = preTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .imageArrayLayers = 1,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = NULL,
        .presentMode = swapchainPresentMode,
        .oldSwapchain = oldSwapchain,
        .clipped = true,
    };
    uint32_t i;

    err = cr_vk_inner->fpCreateSwapchainKHR(cr_vk_inner->device, &swapchain, NULL,
                                     &cr_vk_inner->swapchain);
    assert(!err);

    // If we just re-created an existing swapchain, we should destroy the old
    // swapchain at this point.
    // Note: destroying the swapchain also cleans up all its associated
    // presentable images once the platform is done with them.
    if (oldSwapchain != VK_NULL_HANDLE) {
        cr_vk_inner->fpDestroySwapchainKHR(cr_vk_inner->device, oldSwapchain, NULL);
    }

    err = cr_vk_inner->fpGetSwapchainImagesKHR(cr_vk_inner->device, cr_vk_inner->swapchain,
                                        &cr_vk_inner->swapchainImageCount, NULL);
    assert(!err);

    VkImage *swapchainImages =
        (VkImage *)malloc(cr_vk_inner->swapchainImageCount * sizeof(VkImage));
    assert(swapchainImages);
    err = cr_vk_inner->fpGetSwapchainImagesKHR(cr_vk_inner->device, cr_vk_inner->swapchain,
                                        &cr_vk_inner->swapchainImageCount,
                                        swapchainImages);
    assert(!err);

    cr_vk_inner->buffers = (SwapchainBuffers *)malloc(sizeof(SwapchainBuffers) *
                                               cr_vk_inner->swapchainImageCount);
    assert(cr_vk_inner->buffers);

    for (i = 0; i < cr_vk_inner->swapchainImageCount; i++) {
        VkImageViewCreateInfo color_image_view = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext = NULL,
            .format = cr_vk_inner->format,
            .components =
                {
                 .r = VK_COMPONENT_SWIZZLE_R,
                 .g = VK_COMPONENT_SWIZZLE_G,
                 .b = VK_COMPONENT_SWIZZLE_B,
                 .a = VK_COMPONENT_SWIZZLE_A,
                },
            .subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                                 .baseMipLevel = 0,
                                 .levelCount = 1,
                                 .baseArrayLayer = 0,
                                 .layerCount = 1},
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .flags = 0,
        };

        cr_vk_inner->buffers[i].image = swapchainImages[i];

        // Render loop will expect image to have been used before and in
        // VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
        // layout and will change to COLOR_ATTACHMENT_OPTIMAL, so init the image
        // to that state
        demo_set_image_layout(
            cr_vk_inner, cr_vk_inner->buffers[i].image, VK_IMAGE_ASPECT_COLOR_BIT,
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

        color_image_view.image = cr_vk_inner->buffers[i].image;

        err = vkCreateImageView(cr_vk_inner->device, &color_image_view, NULL,
                                &cr_vk_inner->buffers[i].view);
        assert(!err);
    }

    if (NULL != presentModes) {
        free(presentModes);
    }
}

//-------------------------------------

void demo_set_image_layout(
    struct cr_vk_inner *cr_vk_inner, VkImage image,
    VkImageAspectFlags aspectMask,
    VkImageLayout old_image_layout,
    VkImageLayout new_image_layout
)
{
    VkResult U_ASSERT_ONLY err;

    if (cr_vk_inner->cmd == VK_NULL_HANDLE) {
        const VkCommandBufferAllocateInfo cmd = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .pNext = NULL,
            .commandPool = cr_vk_inner->cmd_pool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1,
        };

        err = vkAllocateCommandBuffers(cr_vk_inner->device, &cmd, &cr_vk_inner->cmd);
        assert(!err);

        VkCommandBufferInheritanceInfo cmd_buf_hinfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO,
            .pNext = NULL,
            .renderPass = VK_NULL_HANDLE,
            .subpass = 0,
            .framebuffer = VK_NULL_HANDLE,
            .occlusionQueryEnable = VK_FALSE,
            .queryFlags = 0,
            .pipelineStatistics = 0,
        };
        VkCommandBufferBeginInfo cmd_buf_info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .pNext = NULL,
            .flags = 0,
            .pInheritanceInfo = &cmd_buf_hinfo,
        };
        err = vkBeginCommandBuffer(cr_vk_inner->cmd, &cmd_buf_info);
        assert(!err);
    }

    VkImageMemoryBarrier image_memory_barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .pNext = NULL,
        .srcAccessMask = 0,
        .dstAccessMask = 0,
        .oldLayout = old_image_layout,
        .newLayout = new_image_layout,
        .image = image,
        .subresourceRange = {aspectMask, 0, 1, 0, 1}};

    if (new_image_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        /* Make sure anything that was copying from this image has completed */
        image_memory_barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    }

    if (new_image_layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
        image_memory_barrier.dstAccessMask =
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    }

    if (new_image_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        image_memory_barrier.dstAccessMask =
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    }

    if (new_image_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        /* Make sure any Copy or CPU writes to image are flushed */
        image_memory_barrier.dstAccessMask =
            VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
    }

    VkImageMemoryBarrier *pmemory_barrier = &image_memory_barrier;

    VkPipelineStageFlags src_stages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    VkPipelineStageFlags dest_stages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

    vkCmdPipelineBarrier(cr_vk_inner->cmd, src_stages, dest_stages, 0, 0, NULL, 0,
                         NULL, 1, pmemory_barrier);
}

bool memory_type_from_properties(
    struct cr_vk_inner *cr_vk_inner, uint32_t typeBits,
    VkFlags requirements_mask,
    uint32_t *typeIndex
)
{
    // Search memtypes to find first index with those properties
    for (uint32_t i = 0; i < 32; i++) {
        if ((typeBits & 1) == 1) {
            // Type is available, does it match user properties?
            if ((cr_vk_inner->memory_properties.memoryTypes[i].propertyFlags &
                 requirements_mask) == requirements_mask) {
                *typeIndex = i;
                return true;
            }
        }
        typeBits >>= 1;
    }
    // No memory types matched, return failure
    return false;
}

void demo_prepare_depth(struct cr_vk_inner *cr_vk_inner)
{
    const VkFormat depth_format = VK_FORMAT_D16_UNORM;
    const VkImageCreateInfo image = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext = NULL,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = depth_format,
        .extent = {cr_vk_inner->width, cr_vk_inner->height, 1},
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        .flags = 0,
    };

    VkImageViewCreateInfo view = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .pNext = NULL,
        .image = VK_NULL_HANDLE,
        .format = depth_format,
        .subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
                             .baseMipLevel = 0,
                             .levelCount = 1,
                             .baseArrayLayer = 0,
                             .layerCount = 1},
        .flags = 0,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
    };

    VkMemoryRequirements mem_reqs;
    VkResult U_ASSERT_ONLY err;
    bool U_ASSERT_ONLY pass;

    cr_vk_inner->depth.format = depth_format;

    /* create image */
    err = vkCreateImage(cr_vk_inner->device, &image, NULL, &cr_vk_inner->depth.image);
    assert(!err);

    vkGetImageMemoryRequirements(cr_vk_inner->device, cr_vk_inner->depth.image, &mem_reqs);
    assert(!err);

    cr_vk_inner->depth.mem_alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    cr_vk_inner->depth.mem_alloc.pNext = NULL;
    cr_vk_inner->depth.mem_alloc.allocationSize = mem_reqs.size;
    cr_vk_inner->depth.mem_alloc.memoryTypeIndex = 0;

    pass = memory_type_from_properties(cr_vk_inner, mem_reqs.memoryTypeBits,
                                       0, /* No requirements */
                                       &cr_vk_inner->depth.mem_alloc.memoryTypeIndex);
    assert(pass);

    /* allocate memory */
    err = vkAllocateMemory(cr_vk_inner->device, &cr_vk_inner->depth.mem_alloc, NULL,
                           &cr_vk_inner->depth.mem);
    assert(!err);

    /* bind memory */
    err =
        vkBindImageMemory(cr_vk_inner->device, cr_vk_inner->depth.image, cr_vk_inner->depth.mem, 0);
    assert(!err);

    demo_set_image_layout(cr_vk_inner, cr_vk_inner->depth.image, VK_IMAGE_ASPECT_DEPTH_BIT,
                          VK_IMAGE_LAYOUT_UNDEFINED,
                          VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

    /* create image view */
    view.image = cr_vk_inner->depth.image;
    err = vkCreateImageView(cr_vk_inner->device, &view, NULL, &cr_vk_inner->depth.view);
    assert(!err);
}

void demo_prepare_texture_image(
    struct cr_vk_inner *cr_vk_inner, const char *filename,
    struct texture_object *tex_obj,
    VkImageTiling tiling,
    VkImageUsageFlags usage,
    VkFlags required_props
)
{
    const VkFormat tex_format = VK_FORMAT_R8G8B8A8_UNORM;
    int32_t tex_width;
    int32_t tex_height;
    VkResult U_ASSERT_ONLY err;
    bool U_ASSERT_ONLY pass;

    if (!loadTexture(filename, NULL, NULL, &tex_width, &tex_height)) {
        printf("Failed to load textures\n");
        fflush(stdout);
        exit(1);
    }

    tex_obj->tex_width = tex_width;
    tex_obj->tex_height = tex_height;

    const VkImageCreateInfo image_create_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext = NULL,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = tex_format,
        .extent = {tex_width, tex_height, 1},
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = tiling,
        .usage = usage,
        .flags = 0,
    };

    VkMemoryRequirements mem_reqs;

    err =
        vkCreateImage(cr_vk_inner->device, &image_create_info, NULL, &tex_obj->image);
    assert(!err);

    vkGetImageMemoryRequirements(cr_vk_inner->device, tex_obj->image, &mem_reqs);

    tex_obj->mem_alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    tex_obj->mem_alloc.pNext = NULL;
    tex_obj->mem_alloc.allocationSize = mem_reqs.size;
    tex_obj->mem_alloc.memoryTypeIndex = 0;

    pass = memory_type_from_properties(cr_vk_inner, mem_reqs.memoryTypeBits,
                                       required_props,
                                       &tex_obj->mem_alloc.memoryTypeIndex);
    assert(pass);

    /* allocate memory */
    err = vkAllocateMemory(cr_vk_inner->device, &tex_obj->mem_alloc, NULL,
                           &(tex_obj->mem));
    assert(!err);

    /* bind memory */
    err = vkBindImageMemory(cr_vk_inner->device, tex_obj->image, tex_obj->mem, 0);
    assert(!err);

    if (required_props & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
        const VkImageSubresource subres = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .mipLevel = 0,
            .arrayLayer = 0,
        };
        VkSubresourceLayout layout;
        void *data;

        vkGetImageSubresourceLayout(cr_vk_inner->device, tex_obj->image, &subres,
                                    &layout);

        err = vkMapMemory(cr_vk_inner->device, tex_obj->mem, 0,
                          tex_obj->mem_alloc.allocationSize, 0, &data);
        assert(!err);

        if (!loadTexture(filename, data, &layout, &tex_width, &tex_height)) {
            fprintf(stderr, "Error loading texture: %s\n", filename);
        }

        vkUnmapMemory(cr_vk_inner->device, tex_obj->mem);
    }

    tex_obj->imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    demo_set_image_layout(cr_vk_inner, tex_obj->image, VK_IMAGE_ASPECT_COLOR_BIT,
                          VK_IMAGE_LAYOUT_UNDEFINED, tex_obj->imageLayout);
    /* setting the image layout does not reference the actual memory so no need
     * to add a mem ref */
}

