#define GLFW_INCLUDE_VULKAN
#include "vulkan/vulkan.h"
#include <GLFW/glfw3.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "vk_engine.h"

#define FRAME_OVERLAP 2


//Internal vulakn  functions
static void _vulakn_init();
static void _vulkan_swapchain();
static void _vulkan_initcommands();
static void _vulkan_sync_structures();
static void _vulkan_cleanup();
static void _vulakn_draw();

/*###############################################################################################################################*
*---------------------------------------------------------Vulkan External--------------------------------------------------------*
*################################################################################################################################*/

GLFWwindow* window;
void VKEngine_init()
{
    printf("size: %lu\n", sizeof(VkDevice));
    if(!glfwInit())
    {
        printf("failed to initialise glfw :( quitting");
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    window = glfwCreateWindow(640, 480, "Window Title", NULL, NULL);
    if(!window)printf("failed to create window \n");

    _vulakn_init();
    _vulkan_swapchain();
    _vulkan_initcommands();
    _vulkan_sync_structures();
}

void VkEngine_cleanup()
{
    if(glfwWindowShouldClose(window))
    {
        _vulkan_cleanup();
        glfwDestroyWindow(window);
        glfwTerminate();
    }
}

void VKEngine_draw()
{
    
}

void VkEngine_run()
{
    while(!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        _vulakn_draw();
    }
}


/*###############################################################################################################################*
*---------------------------------------------------------Vulkan internal--------------------------------------------------------*
*################################################################################################################################*/


/*###############################################################################################################################*
*---------------------------------------------------------Vulkan Debug-----------------------------------------------------------*
*################################################################################################################################*/

static VKAPI_ATTR VkBool32 VKAPI_CALL vulkanDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT Severity, VkDebugUtilsMessageTypeFlagsEXT sType, const VkDebugUtilsMessengerCallbackDataEXT* CallbackData, void*UserData)
{
    printf("Validaiton layer: %s\n", CallbackData->pMessage);
    return VK_FALSE;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL vulkanDebugReportCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, uint64_t object, size_t location, int32_t messageCode, const char* pLayerPrefix, const char*pMessage, void *UserData)
{
    printf("Debug callback [%s]: %s\n", pLayerPrefix, pMessage);
    return VK_FALSE;
}

PFN_vkCreateDebugUtilsMessengerEXT  pfnVkCreateDebugUtilsMessengerEXT;
PFN_vkDestroyDebugUtilsMessengerEXT pfnVkDestroyDebugUtilsMessengerEXT;
PFN_vkCreateDebugReportCallbackEXT  pfnCreateDebugReportCallbackEXT;
PFN_vkDestroyDebugReportCallbackEXT pfnDestroyDebugReportCallbackEXT;

VKAPI_ATTR VkResult VKAPI_CALL vkCreateDebugUtilsMessengerEXT( VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT * pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDebugUtilsMessengerEXT *pMessenger )
{
  return pfnVkCreateDebugUtilsMessengerEXT( instance, pCreateInfo, pAllocator, pMessenger );
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateDebugReportCallbackEXT( VkInstance instance, const VkDebugReportCallbackCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugReportCallbackEXT* pCallback
){
	return pfnCreateDebugReportCallbackEXT( instance, pCreateInfo, pAllocator, pCallback );
}

VKAPI_ATTR void VKAPI_CALL vkDestroyDebugUtilsMessengerEXT( VkInstance instance, VkDebugUtilsMessengerEXT messenger, VkAllocationCallbacks const * pAllocator )
{
  return pfnVkDestroyDebugUtilsMessengerEXT( instance, messenger, pAllocator );
}

VKAPI_ATTR void VKAPI_CALL vkDestroyDebugReportCallbackEXT( VkInstance instance, VkDebugReportCallbackEXT messenger, VkAllocationCallbacks const * pAllocator )
{
  return pfnDestroyDebugReportCallbackEXT( instance, messenger, pAllocator );
}

/*###################################################################################################################################*
*---------------------------------------------------------Vulkan Debug end-----------------------------------------------------------*
*####################################################################################################################################*/

struct _vulkan_instance
{
    VkInstance instance;
    VkSurfaceKHR surface;
    VkDebugUtilsMessengerEXT pmessage;
    VkDebugReportCallbackEXT pCallback;
};
static struct _vulkan_instance vkContext = {0};

struct _vulakn_framedata
{
    VkCommandPool       _commandPool;
    VkCommandBuffer     _mainCommandBuffer;

    VkSemaphore _swapchainSemaphore, _renderSemaphore;
	VkFence _renderFence;
};
static struct _vulakn_framedata frame_data[FRAME_OVERLAP] = {0};
static int _framenumber = 0;

VkQueue _graphicsQueue;
static void _vulakn_checkresult(VkResult result, const char *from)
{
    switch(result)
    {
        case(VK_SUCCESS):
            printf("VK_SUCCESS From: [%s] \n", from);
        break;

        case(VK_ERROR_EXTENSION_NOT_PRESENT):
            printf("VK_ERROR_EXTENSION_NOT_PRESENT, From: [%s] \n", from);
            exit(1);
        break;
        case(VK_ERROR_INCOMPATIBLE_DRIVER):
            printf("VK_ERROR_EXTENSION_NOT_PRESENT, From: [%s] \n", from);
            exit(1);
        break;
        case(VK_ERROR_INITIALIZATION_FAILED):
            printf("VK_ERROR_INITIALIZATION_FAILED, From: [%s] \n", from);
            exit(1);
        break;
        case(VK_ERROR_LAYER_NOT_PRESENT):
            printf("VK_ERROR_LAYER_NOT_PRESENT, From: [%s] \n", from);
            exit(1);
        break;
        case(VK_ERROR_OUT_OF_DEVICE_MEMORY):
            printf("VK_ERROR_OUT_OF_DEVICE_MEMORY, From: [%s] \n", from);
            exit(1);
        break;
        case(VK_ERROR_UNKNOWN):
            printf("VK_ERROR_UNKNOWN: de fuck did u do, From: [%s] \n", from);
            exit(1);
        break;
        case(VK_ERROR_VALIDATION_FAILED):
            printf("VK_ERROR_VALIDATION_FAILED, From: [%s] \n", from);
            exit(1);
        break;
        default:
            printf("something else, From: [%s] \n", from);
            exit(1);
        break;
    }
}

static void transition_image(VkCommandBuffer cmd, VkImage image, VkImageLayout currLayout, VkImageLayout newLayout)
{
    VkImageMemoryBarrier2 image_barrier_createinfo = {0};
    image_barrier_createinfo.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;

    image_barrier_createinfo.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
    image_barrier_createinfo.srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT;
    image_barrier_createinfo.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
    image_barrier_createinfo.dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT;

    image_barrier_createinfo.oldLayout = currLayout;
    image_barrier_createinfo.newLayout = newLayout;

    VkImageAspectFlags aspectMask = (newLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;

    VkImageSubresourceRange image_subresource_range = {0};
    image_subresource_range.aspectMask = aspectMask;
    image_subresource_range.baseMipLevel = 0;
    image_subresource_range.levelCount = VK_REMAINING_MIP_LEVELS;
    image_subresource_range.baseArrayLayer = 0;
    image_subresource_range.layerCount = VK_REMAINING_ARRAY_LAYERS;

    image_barrier_createinfo.subresourceRange = image_subresource_range;
    image_barrier_createinfo.image = image;

    VkDependencyInfo dependency_info = {0};
    dependency_info.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dependency_info.imageMemoryBarrierCount = 1;
    dependency_info.pImageMemoryBarriers = &image_barrier_createinfo;

    vkCmdPipelineBarrier2(cmd, &dependency_info);
}


uint32_t queueFamilyIndex = 0;
uint32_t queue_graphics_family;
VkDevice _device;
VkSwapchainKHR _swapchain;
VkImage *_swapchain_image = {0};
static void _vulakn_choosedevice()
{
    uint32_t pPhysicalDeviceCount = 0;
    VkResult GetPhysicalDevices_Count = vkEnumeratePhysicalDevices(vkContext.instance, &pPhysicalDeviceCount, NULL);
    _vulakn_checkresult(GetPhysicalDevices_Count, "GetPhysicalDevices_Count");
    
    
    VkPhysicalDevice *pPhysicalDevices = malloc((pPhysicalDeviceCount+1) * sizeof(VkPhysicalDevice));
    VkResult EnumeratePhysicalDevices = vkEnumeratePhysicalDevices(vkContext.instance, &pPhysicalDeviceCount, pPhysicalDevices);
    _vulakn_checkresult(EnumeratePhysicalDevices, "PhysicalDevices");


    //https://docs.vulkan.org/spec/latest/chapters/devsandqueues.html#VkPhysicalDeviceProperties2
    VkPhysicalDeviceProperties2 pProperties = {0};
    pProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    pProperties.pNext = NULL;

    //https://docs.vulkan.org/spec/latest/chapters/features.html#vkGetPhysicalDeviceFeatures2
    VkPhysicalDeviceFeatures2 pFeatures = {0};
    pFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    pFeatures.pNext = NULL;

    //https://docs.vulkan.org/spec/latest/chapters/devsandqueues.html#vkGetPhysicalDeviceQueueFamilyProperties2

    uint32_t pQueueFamilyPropertyCount = 0;
    uint32_t SELECTED_GPU_INDEX = 0;

    VkQueueFamilyProperties2 *pQueueFamilyProperties;

    for(uint32_t i =0; i < pPhysicalDeviceCount; i++)
    {
    

        vkGetPhysicalDeviceProperties2(pPhysicalDevices[i], &pProperties);
        vkGetPhysicalDeviceFeatures2(pPhysicalDevices[i], &pFeatures);

        vkGetPhysicalDeviceQueueFamilyProperties2(pPhysicalDevices[i], &pQueueFamilyPropertyCount, NULL);
        pQueueFamilyProperties = malloc((pQueueFamilyPropertyCount) * sizeof(VkQueueFamilyProperties2));

        for(uint32_t pQ = 0; pQ < pQueueFamilyPropertyCount; pQ++)
        {
            pQueueFamilyProperties[pQ].sType = VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2;
            pQueueFamilyProperties[pQ].pNext = NULL;
        }
        vkGetPhysicalDeviceQueueFamilyProperties2(pPhysicalDevices[i], &pQueueFamilyPropertyCount, pQueueFamilyProperties);
        

        for(uint32_t Q = 0; Q < pQueueFamilyPropertyCount; Q++)
        {
            if(glfwGetPhysicalDevicePresentationSupport(vkContext.instance, pPhysicalDevices[i],Q))
            {
                if(pProperties.properties.deviceType & VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
                {
                    //printf("use device: %s by default\n", pProperties.properties.deviceName);
                    printf("VK_QUEUE_GRAPHICS_BIT, QUEUECOUNT: %u INDEX: %u DEVICE NAME: %s\n", pQueueFamilyProperties[Q].queueFamilyProperties.queueCount, Q, pProperties.properties.deviceName);

                    if(pQueueFamilyProperties[Q].queueFamilyProperties.queueFlags == VK_QUEUE_GRAPHICS_BIT)
                    {
                        queueFamilyIndex = Q;
                    }
                    SELECTED_GPU_INDEX = i;
                }else
                {

                    printf("CPU: %s should add ablitity to choose but thats for later \n", pProperties.properties.deviceName);
                }
            }
        }
        free(pQueueFamilyProperties);
        //think about adding the ablity to choose what device to use. later
    }

    VkPhysicalDeviceVulkan13Features vulkan_13_features  ={0};
    vulkan_13_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
    vulkan_13_features.synchronization2 = VK_TRUE;

    float queuePriority = 1.0f;
    VkDeviceQueueCreateInfo queue_create_info = {0};
    queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_info.queueFamilyIndex = queueFamilyIndex;
    queue_create_info.queueCount = 1;
    queue_create_info.pQueuePriorities = &queuePriority;
    


    const char* enabled_extensions[2] = {"VK_KHR_swapchain", "VK_KHR_synchronization2"};
    VkDeviceCreateInfo create_info = {0};
    create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    create_info.pNext = &vulkan_13_features;
    create_info.queueCreateInfoCount = 1;
    create_info.pQueueCreateInfos = &queue_create_info;
    create_info.enabledExtensionCount = 2;
    create_info.ppEnabledExtensionNames = enabled_extensions;


    printf("selected gpu index %u\n",SELECTED_GPU_INDEX);
    VkResult CreateDeviceResult = vkCreateDevice(pPhysicalDevices[SELECTED_GPU_INDEX], &create_info, NULL, &_device);
    _vulakn_checkresult(CreateDeviceResult, "Create Device");

    
    uint32_t pSurfaceFormatCount = 0;
    VkPhysicalDeviceSurfaceInfo2KHR physical_device_surface_info = {0};
    physical_device_surface_info.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SURFACE_INFO_2_KHR;
    physical_device_surface_info.surface = vkContext.surface;

    printf("graphics family %u\n", queue_graphics_family);
    vkGetDeviceQueue(_device, queueFamilyIndex, 0, &_graphicsQueue);
    /*
    VkResult PhysicalDeviceSurfaceFormatCountResult = vkGetPhysicalDeviceSurfaceFormats2KHR(pPhysicalDevices[SELECTED_GPU_INDEX], &physical_device_surface_info, &pSurfaceFormatCount, NULL);
    _VkResultsCheck(PhysicalDeviceSurfaceFormatCountResult, "Physical Device Surface Format count");

    pSurfaceFormats = malloc((pSurfaceFormatCount+1) * sizeof(VkSurfaceFormat2KHR));
    for(uint32_t i = 0; i <= pSurfaceFormatCount; i++)
    {
        pSurfaceFormats[i].sType =VK_STRUCTURE_TYPE_SURFACE_FORMAT_2_KHR;
        pSurfaceFormats[i].pNext = NULL;
    }

    VkResult PhysicalDeviceSurfaceFormatResult = vkGetPhysicalDeviceSurfaceFormats2KHR(pPhysicalDevices[SELECTED_GPU_INDEX], &physical_device_surface_info, &pSurfaceFormatCount, pSurfaceFormats);
    _VkResultsCheck(PhysicalDeviceSurfaceFormatResult, "Physical Device Surface Format");*/
}

static void _vulakn_init()
{
    uint32_t Vulakn_apiVersion = VK_API_VERSION_1_1;
    VkApplicationInfo application_info = {0};
    application_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO ;
    application_info.apiVersion = VK_API_VERSION_1_3;

    PFN_vkVoidFunction EnumerateInstanceVersion = vkGetInstanceProcAddr(vkContext.instance, "vkEnumerateInstanceVersion");
    if(!EnumerateInstanceVersion)
    {
        printf("VK_API_VERSION_1_0");
    }else
    {
        VkResult InstanceVersion = vkEnumerateInstanceVersion(&Vulakn_apiVersion);
        _vulakn_checkresult(InstanceVersion, "vkEnumerateInstanceVersion");
    }

    uint32_t enabledExtensionCount = 0;
    const char * const *extensions = glfwGetRequiredInstanceExtensions(&enabledExtensionCount);
    uint32_t n_enabledExtensionCount = enabledExtensionCount +3;

    const char *extensions_list[5] = {extensions[0], extensions[1], "VK_KHR_get_surface_capabilities2", "VK_EXT_debug_utils", "VK_EXT_debug_report"};
    const char **ppEnabledExtensionNames = malloc((n_enabledExtensionCount) * sizeof(const char *));
    memcpy(ppEnabledExtensionNames, extensions_list, n_enabledExtensionCount * sizeof(const char*)); 

    uint32_t enabledLayerCount = 0;
    #ifdef VGRAPHICS_VERBOSE
        enabledLayerCount++;
    #endif
    
    const char** ppEnabledLayerNames = NULL;
    if(enabledLayerCount > 0) ppEnabledLayerNames = malloc(enabledLayerCount * sizeof(const char*));

    VkInstanceCreateInfo instance_createinfo = {0};
    instance_createinfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_createinfo.pNext = NULL;
    instance_createinfo.pApplicationInfo = &application_info;
    instance_createinfo.enabledExtensionCount = n_enabledExtensionCount;
    instance_createinfo.ppEnabledExtensionNames = ppEnabledExtensionNames;
    instance_createinfo.enabledLayerCount = enabledLayerCount;
    instance_createinfo.ppEnabledLayerNames = ppEnabledLayerNames;


    VkResult instanceResult = vkCreateInstance(&instance_createinfo, NULL, &vkContext.instance);
    _vulakn_checkresult(instanceResult, "INSTANCE_CREATE");

    
    pfnVkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(vkContext.instance, "vkCreateDebugUtilsMessengerEXT");
    if(!pfnVkCreateDebugUtilsMessengerEXT)printf("failed to retrive vkCreateDebugUtilsMessengerEXT\n");

    pfnVkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(vkContext.instance, "vkDestroyDebugUtilsMessengerEXT");
    if(!pfnVkDestroyDebugUtilsMessengerEXT)printf("failed to retrive vkDestroyDebugUtilsMessengerEXT\n");

    pfnCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(vkContext.instance, "vkCreateDebugReportCallbackEXT");
    if(!pfnCreateDebugReportCallbackEXT)printf("failed to retrive vkCreateDebugReportCallbackEXT\n");

    pfnDestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(vkContext.instance, "vkDestroyDebugReportCallbackEXT");
    if(!pfnDestroyDebugReportCallbackEXT)printf("failed to retrive vkDestroyDebugReportCallbackEXT\n");

    VkDebugUtilsMessengerCreateInfoEXT debugutils_messenger_createinfo = {0};
    debugutils_messenger_createinfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debugutils_messenger_createinfo.flags = 0;
    debugutils_messenger_createinfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debugutils_messenger_createinfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debugutils_messenger_createinfo.pfnUserCallback = &vulkanDebugCallback;
    debugutils_messenger_createinfo.pUserData = NULL;

    
    VkDebugReportCallbackCreateInfoEXT debugreport_callback_createinfo = {0};
    debugreport_callback_createinfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
    debugreport_callback_createinfo.flags = VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT | VK_DEBUG_REPORT_ERROR_BIT_EXT |  VK_DEBUG_REPORT_DEBUG_BIT_EXT;
    debugreport_callback_createinfo.pfnCallback = &vulkanDebugReportCallback;
    debugreport_callback_createinfo.pUserData = NULL;
    
    VkResult createDebugUtilsResult = vkCreateDebugUtilsMessengerEXT(vkContext.instance, &debugutils_messenger_createinfo, NULL, &vkContext.pmessage);
    _vulakn_checkresult(createDebugUtilsResult, "vkCreateDebugUtilsMessenger");

    VkResult createDebugReportCallbackResult = vkCreateDebugReportCallbackEXT(vkContext.instance, &debugreport_callback_createinfo, NULL, &vkContext.pCallback);
    _vulakn_checkresult(createDebugReportCallbackResult, "vkCreateDebugReportCallbackEXT");

    VkResult CreateSurfaceResult = glfwCreateWindowSurface(vkContext.instance, window, NULL, &vkContext.surface);
    _vulakn_checkresult(CreateSurfaceResult, "Create surface");
    
    _vulakn_choosedevice();

    free(ppEnabledExtensionNames);
    free(ppEnabledLayerNames);
}

VkImageView *_imageview = {0};
uint32_t swapChainImageCount = 0;
static void _vulkan_swapchain()
{
    VkExtent2D vksurface_dimension = {640, 480};
    VkSwapchainCreateInfoKHR swapchain_createInfo = {0};
    swapchain_createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchain_createInfo.surface = vkContext.surface;
    swapchain_createInfo.minImageCount = 3;
    swapchain_createInfo.imageFormat = VK_FORMAT_B8G8R8A8_UNORM;
    swapchain_createInfo.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    swapchain_createInfo.imageExtent = vksurface_dimension; //look at vkGetPhysicalDeviceSurfaceCapabilitiesKHR later
    swapchain_createInfo.imageArrayLayers = 1;
    swapchain_createInfo.imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchain_createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchain_createInfo.compositeAlpha = 0x00000001; //VkCompositeAlphaFlagBitsKHR.VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR
    swapchain_createInfo.preTransform = 0x00000001; //VkSurfaceTransformFlagBitsKHR.VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR
    swapchain_createInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    swapchain_createInfo.oldSwapchain = VK_NULL_HANDLE;

    VkResult createSwapResult = vkCreateSwapchainKHR(_device, &swapchain_createInfo, NULL, &_swapchain);
    _vulakn_checkresult(createSwapResult, "Create Swap chain");

    VkResult swapChainImageResult = vkGetSwapchainImagesKHR(_device, _swapchain, &swapChainImageCount, NULL);
    _vulakn_checkresult(swapChainImageResult, "vkGetSwapchainImagesKHR count");

    _swapchain_image = malloc((swapChainImageCount+1) * sizeof(VkImage));
    VkResult GetswapChainImageResult = vkGetSwapchainImagesKHR(_device, _swapchain, &swapChainImageCount, _swapchain_image);
    _vulakn_checkresult(GetswapChainImageResult, "vkGetSwapchainImagesKHR");

    _imageview = malloc((swapChainImageCount+1)*sizeof(VkImageView));
    printf("%u\n",swapChainImageCount);
    
    for(uint32_t i = 0; i < swapChainImageCount; i++)
    {
        VkImageViewCreateInfo image_view_createinfo = {0};
        image_view_createinfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        image_view_createinfo.image = _swapchain_image[i];
        image_view_createinfo.viewType = 1;
        image_view_createinfo.format = VK_FORMAT_B8G8R8A8_UNORM;

        image_view_createinfo.components.r = 3;
        image_view_createinfo.components.g = 4;
        image_view_createinfo.components.b = 5;
        image_view_createinfo.components.a = 6;

        image_view_createinfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        image_view_createinfo.subresourceRange.baseMipLevel = 0;
        image_view_createinfo.subresourceRange.levelCount = 1;
        image_view_createinfo.subresourceRange.baseArrayLayer = 0;
        image_view_createinfo.subresourceRange.layerCount = 1;

        VkResult CreateImageViewResult = vkCreateImageView(_device, &image_view_createinfo, NULL, &_imageview[i]);
    }
}

static void _vulkan_initcommands()
{
    VkCommandPoolCreateInfo command_pool_createinfo = {0};
    command_pool_createinfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    command_pool_createinfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    command_pool_createinfo.queueFamilyIndex = queueFamilyIndex;

    for(uint32_t i = 0; i < FRAME_OVERLAP; i++)
    {
        VkResult createCommandPoolResult = vkCreateCommandPool(_device, &command_pool_createinfo, NULL, &frame_data[i]._commandPool);
        _vulakn_checkresult(createCommandPoolResult, "vkCreateCommandPool");

        VkCommandBufferAllocateInfo command_bufferallo_createinfo = {0};
        command_bufferallo_createinfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        command_bufferallo_createinfo.commandPool = frame_data[i]._commandPool;
        command_bufferallo_createinfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        command_bufferallo_createinfo.commandBufferCount = 1;

        VkResult allocateCommandBufferResult = vkAllocateCommandBuffers(_device, &command_bufferallo_createinfo, &frame_data[i]._mainCommandBuffer);
    }
}

static void _vulkan_sync_structures()
{
    VkFenceCreateInfo fence_createinfo = {0};
    fence_createinfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_createinfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    VkSemaphoreCreateInfo semaphore_createinfo = {0};
    semaphore_createinfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    for(uint32_t i = 0; i < FRAME_OVERLAP; i++)
    {
        VkResult createFenceResult = vkCreateFence(_device, &fence_createinfo, NULL, &frame_data[i]._renderFence);
        _vulakn_checkresult(createFenceResult, "vkCreateFence");

        VkResult createSwapchainSemaphoreResult = vkCreateSemaphore(_device, &semaphore_createinfo, NULL, &frame_data[i]._swapchainSemaphore);
        _vulakn_checkresult(createSwapchainSemaphoreResult, "createSwapchainSemaphore");

        VkResult createRenderSemaphoreResult = vkCreateSemaphore(_device, &semaphore_createinfo, NULL, &frame_data[i]._renderSemaphore);
        _vulakn_checkresult(createRenderSemaphoreResult, "createRenderSemaphore");
    }
}

static void _vulakn_draw()
{
    
    VkResult waitForFenceResult = vkWaitForFences(_device, 1, &frame_data[_framenumber % FRAME_OVERLAP]._renderFence, VK_TRUE, 1000000000);
    _vulakn_checkresult(waitForFenceResult, "vkWaitForFences");
    VkResult resetFencesResult = vkResetFences(_device, 1, &frame_data[_framenumber % FRAME_OVERLAP]._renderFence);
    _vulakn_checkresult(resetFencesResult, "vkResetFences");

    uint32_t swapchainImageIndex = 0;
    VkResult acquireNextImageResult = vkAcquireNextImageKHR(_device, _swapchain, 1000000000, frame_data[_framenumber % FRAME_OVERLAP]._swapchainSemaphore, NULL, &swapchainImageIndex);
    _vulakn_checkresult(resetFencesResult, "vkResetFences");

    VkCommandBufferBeginInfo command_bufferbegin_createinfo = {0};
    command_bufferbegin_createinfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    command_bufferbegin_createinfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    VkCommandBuffer cmd = frame_data[_framenumber % FRAME_OVERLAP]._mainCommandBuffer;
    VkResult resetCommandBufferResult = vkResetCommandBuffer(cmd, 0);
    _vulakn_checkresult(resetCommandBufferResult, "vkResetCommandBuffer");
    VkResult beginCommandBufferResult = vkBeginCommandBuffer(cmd, &command_bufferbegin_createinfo);
    _vulakn_checkresult(beginCommandBufferResult, "vkBeginCommandBuffer");

    transition_image(cmd, _swapchain_image[swapchainImageIndex], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
    float flash = sin(_framenumber / 120.0f);
    printf("flash: %f\n", flash);
    VkClearColorValue clearValue = {0};
    clearValue.float32[0] = flash;
    clearValue.float32[1] = flash;
    clearValue.float32[2] = 0.0f;
    clearValue.float32[3] = 1.0f;

    VkImageSubresourceRange clear_range = {0};
    clear_range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    clear_range.baseMipLevel = 0;
    clear_range.levelCount = VK_REMAINING_MIP_LEVELS;
    clear_range.baseArrayLayer = 0;
    clear_range.layerCount = VK_REMAINING_ARRAY_LAYERS;

    vkCmdClearColorImage(cmd, _swapchain_image[swapchainImageIndex], VK_IMAGE_LAYOUT_GENERAL, &clearValue, 1, &clear_range);
    transition_image(cmd, _swapchain_image[swapchainImageIndex], VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

    VkResult endCommandBuffer = vkEndCommandBuffer(cmd);
    _vulakn_checkresult(endCommandBuffer, "vkEndCommandBuffer");

    VkSemaphoreSubmitInfo waitsemaphore_submit_info = {0};
    waitsemaphore_submit_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
    waitsemaphore_submit_info.semaphore = frame_data[_framenumber % FRAME_OVERLAP]._swapchainSemaphore;
    waitsemaphore_submit_info.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR;
    waitsemaphore_submit_info.deviceIndex = 0;
    waitsemaphore_submit_info.value = 1;

    VkSemaphoreSubmitInfo rendersemaphore_submit_info = {0};
    rendersemaphore_submit_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
    rendersemaphore_submit_info.semaphore = frame_data[_framenumber % FRAME_OVERLAP]._renderSemaphore;
    rendersemaphore_submit_info.stageMask = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT;
    rendersemaphore_submit_info.deviceIndex = 0;
    rendersemaphore_submit_info.value = 1;


    VkCommandBufferSubmitInfo command_submit_info = {0};
    command_submit_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
    command_submit_info.commandBuffer = cmd;
    command_submit_info.deviceMask = 0;

    VkSubmitInfo2 submit_info = {0};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
    submit_info.waitSemaphoreInfoCount = &waitsemaphore_submit_info == NULL ? 0: 1;
    submit_info.pWaitSemaphoreInfos = &waitsemaphore_submit_info;

    submit_info.signalSemaphoreInfoCount = &rendersemaphore_submit_info == NULL ? 0 : 1;
    submit_info.pSignalSemaphoreInfos = &rendersemaphore_submit_info;

    submit_info.commandBufferInfoCount = 1;
    submit_info.pCommandBufferInfos = &command_submit_info;

    VkResult queue_submit = vkQueueSubmit2(_graphicsQueue, 1, &submit_info, frame_data[_framenumber % FRAME_OVERLAP]._renderFence);
    _vulakn_checkresult(queue_submit, "vkQueueSubmit2");

    VkPresentInfoKHR present_info = {0};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.pSwapchains = &_swapchain;
    present_info.swapchainCount = 1;

    present_info.pWaitSemaphores = &frame_data[_framenumber % FRAME_OVERLAP]._renderSemaphore;
    present_info.waitSemaphoreCount = 1;

    present_info.pImageIndices = &swapchainImageIndex;

    VkResult queue_present = vkQueuePresentKHR(_graphicsQueue, &present_info);
    _vulakn_checkresult(queue_present, "vkQueuePresentKHR");
    _framenumber ++;
}


static void _vulakn_destory_swapchain()
{
    vkDestroySwapchainKHR(_device, _swapchain, NULL);
    for(uint32_t i = 0; i < swapChainImageCount; i++)
    {
        vkDestroyImageView(_device, _imageview[i], NULL);
    }
    free(_imageview);
}

static void _vulkan_cleanup()
{
    vkDeviceWaitIdle(_device);
    _vulakn_destory_swapchain();
    vkDestroySurfaceKHR(vkContext.instance, vkContext.surface, NULL);

    vkDeviceWaitIdle(_device);
    for(uint32_t i = 0; i < FRAME_OVERLAP; i++)
    {
        vkDestroyFence(_device, frame_data[i]._renderFence, NULL);
        vkDestroySemaphore(_device, frame_data[i]._renderSemaphore, NULL);
        vkDestroySemaphore(_device, frame_data[i]._swapchainSemaphore, NULL);
        vkDestroyCommandPool(_device, frame_data[i]._commandPool, NULL);
    }

    vkDestroyDevice(_device, NULL);
    vkDestroyDebugUtilsMessengerEXT(vkContext.instance, vkContext.pmessage, NULL);
    vkDestroyDebugReportCallbackEXT(vkContext.instance, vkContext.pCallback, NULL);
    vkDestroyInstance(vkContext.instance, NULL);
}