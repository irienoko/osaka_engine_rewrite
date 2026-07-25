#define GLFW_INCLUDE_VULKAN
#include "vulkan/vulkan.h"
#include <GLFW/glfw3.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "vk_engine.h"


static VKAPI_ATTR VkBool32 VKAPI_CALL vulkanDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT Severity, VkDebugUtilsMessageTypeFlagsEXT sType,
                                                         const VkDebugUtilsMessengerCallbackDataEXT* CallbackData, void*UserData)
{
    printf("Validaiton layer: %s\n", CallbackData->pMessage);
    return VK_FALSE;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL vulkanDebugReportCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, uint64_t object,
                                                                size_t location, int32_t messageCode, const char* pLayerPrefix, const char*pMessage, void *UserData)
{
    printf("Debug callback [%s]: %s\n", pLayerPrefix, pMessage);
    return VK_FALSE;
}


PFN_vkCreateDebugUtilsMessengerEXT  pfnVkCreateDebugUtilsMessengerEXT;
PFN_vkDestroyDebugUtilsMessengerEXT pfnVkDestroyDebugUtilsMessengerEXT;
PFN_vkCreateDebugReportCallbackEXT  pfnCreateDebugReportCallbackEXT;
PFN_vkDestroyDebugReportCallbackEXT pfnDestroyDebugReportCallbackEXT;

VKAPI_ATTR VkResult VKAPI_CALL vkCreateDebugUtilsMessengerEXT( VkInstance                                 instance,
                                                               const VkDebugUtilsMessengerCreateInfoEXT * pCreateInfo,
                                                               const VkAllocationCallbacks *              pAllocator,
                                                               VkDebugUtilsMessengerEXT *                 pMessenger )
{
  return pfnVkCreateDebugUtilsMessengerEXT( instance, pCreateInfo, pAllocator, pMessenger );
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateDebugReportCallbackEXT(
	VkInstance instance,
	const VkDebugReportCallbackCreateInfoEXT* pCreateInfo,
	const VkAllocationCallbacks* pAllocator,
	VkDebugReportCallbackEXT* pCallback
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

static void _vulakn_init();
static void _vulkan_swapchain();
static void _vulkan_initcommands();
static void _vulkan_sync_structures();
static void _vulkan_cleanup();

struct _vulkanContext
{
    VkInstance instance;
    VkSurfaceKHR surface;
};

struct _vulkanContext vkContext = {0};

GLFWwindow* window;
void VKEngine_init()
{
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
    }
}


/*###############################################################################################################################*
*---------------------------------------------------------Vulkan internal--------------------------------------------------------*
*################################################################################################################################*/
static void _VkResultsCheck(VkResult result, const char *from)
{
    switch(result)
    {
        case(VK_SUCCESS):
            printf("VK_SUCCESS From: [%s] \n", from);
        break;

        case(VK_ERROR_EXTENSION_NOT_PRESENT):
            printf("VK_ERROR_EXTENSION_NOT_PRESENT, From: [%s] \n", from);
        break;
        case(VK_ERROR_INCOMPATIBLE_DRIVER):
            printf("VK_ERROR_EXTENSION_NOT_PRESENT, From: [%s] \n", from);
        break;
        case(VK_ERROR_INITIALIZATION_FAILED):
            printf("VK_ERROR_INITIALIZATION_FAILED, From: [%s] \n", from);
        break;
        case(VK_ERROR_LAYER_NOT_PRESENT):
            printf("VK_ERROR_LAYER_NOT_PRESENT, From: [%s] \n", from);
        break;
        case(VK_ERROR_OUT_OF_DEVICE_MEMORY):
            printf("VK_ERROR_OUT_OF_DEVICE_MEMORY, From: [%s] \n", from);
        break;
        case(VK_ERROR_UNKNOWN):
            printf("VK_ERROR_UNKNOWN: de fuck did u do, From: [%s] \n", from);
        break;
        case(VK_ERROR_VALIDATION_FAILED):
            printf("VK_ERROR_VALIDATION_FAILED, From: [%s] \n", from);
        break;
        default:
            printf("something else, From: [%s] \n", from);
        break;
    }
}


VkDevice pDeviceHandel;
VkSwapchainKHR pSwapchainHandel;
//VkSurfaceFormat2KHR *pSurfaceFormats = {0};
uint32_t queueFamilyIndex  = 0;
/*Device and Queues*/
static void _vulakn_PhysicalDevices()
{
    /*Get number of physical devices*/
    uint32_t pPhysicalDeviceCount = 0;
    VkResult GetPhysicalDevices_Count = vkEnumeratePhysicalDevices(vkContext.instance, &pPhysicalDeviceCount, NULL);
    _VkResultsCheck(GetPhysicalDevices_Count, "GetPhysicalDevices_Count");
    
    /*get physical device handel*/
    VkPhysicalDevice *pPhysicalDevices = malloc((pPhysicalDeviceCount+1) * sizeof(VkPhysicalDevice));
    VkResult EnumeratePhysicalDevices = vkEnumeratePhysicalDevices(vkContext.instance, &pPhysicalDeviceCount, pPhysicalDevices);
    _VkResultsCheck(EnumeratePhysicalDevices, "PhysicalDevices");


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
    uint32_t queueFamilyIndex  = 0;
    uint32_t queueCount = 0;

    VkQueueFamilyProperties2 *pQueueFamilyProperties;
    /* check graphics card compatibality*/
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
                    printf("use device: %s by default\n", pProperties.properties.deviceName);
                    printf("VK_QUEUE_GRAPHICS_BIT, QUEUECOUNT: %u INDEX: %u DEVICE NAME: %s\n", pQueueFamilyProperties[Q].queueFamilyProperties.queueCount, Q, pProperties.properties.deviceName);
                    queueFamilyIndex = Q;
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

    float queuePriority = 1.0f;
    VkDeviceQueueCreateInfo queue_create_info = {0};
    queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_info.queueFamilyIndex = queueFamilyIndex;
    queue_create_info.queueCount = 1;
    queue_create_info.pQueuePriorities = &queuePriority;
    


    const char* enabled_extensions[1] = {"VK_KHR_swapchain"};
    VkDeviceCreateInfo create_info = {0};
    create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    create_info.queueCreateInfoCount = 1;
    create_info.pQueueCreateInfos = &queue_create_info;
    create_info.enabledExtensionCount = 1;
    create_info.ppEnabledExtensionNames = enabled_extensions;


    printf("selected gpu index %u\n",SELECTED_GPU_INDEX);
    VkResult CreateDeviceResult = vkCreateDevice(pPhysicalDevices[SELECTED_GPU_INDEX], &create_info, NULL, &pDeviceHandel);
    _VkResultsCheck(CreateDeviceResult, "Create Device");

    
    uint32_t pSurfaceFormatCount = 0;
    VkPhysicalDeviceSurfaceInfo2KHR physical_device_surface_info = {0};
    physical_device_surface_info.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SURFACE_INFO_2_KHR;
    physical_device_surface_info.surface = vkContext.surface;

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

VkDebugUtilsMessengerEXT pmessage;
VkDebugReportCallbackEXT pCallback;
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
        _VkResultsCheck(InstanceVersion, "vkEnumerateInstanceVersion");
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
    _VkResultsCheck(instanceResult, "INSTANCE_CREATE");

    
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
    
    VkResult createDebugUtilsResult = vkCreateDebugUtilsMessengerEXT(vkContext.instance, &debugutils_messenger_createinfo, NULL, &pmessage);
    _VkResultsCheck(createDebugUtilsResult, "vkCreateDebugUtilsMessenger");

    VkResult createDebugReportCallbackResult = vkCreateDebugReportCallbackEXT(vkContext.instance, &debugreport_callback_createinfo, NULL, &pCallback);
    _VkResultsCheck(createDebugReportCallbackResult, "vkCreateDebugReportCallbackEXT");

    VkResult CreateSurfaceResult = glfwCreateWindowSurface(vkContext.instance, window, NULL, &vkContext.surface);
    _VkResultsCheck(CreateSurfaceResult, "Create surface");
    
    _vulakn_PhysicalDevices();

    free(ppEnabledExtensionNames);
    free(ppEnabledLayerNames);
}

VkImageView *pViewHandel = {0};
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

    VkResult createSwapResult = vkCreateSwapchainKHR(pDeviceHandel, &swapchain_createInfo, NULL, &pSwapchainHandel);
    _VkResultsCheck(createSwapResult, "Create Swap chain");

    VkResult swapChainImageResult = vkGetSwapchainImagesKHR(pDeviceHandel, pSwapchainHandel, &swapChainImageCount, NULL);
    _VkResultsCheck(swapChainImageResult, "vkGetSwapchainImagesKHR count");

    VkImage *pSwapchainImagesHandel = malloc((swapChainImageCount+1) * sizeof(VkImage));
    VkResult GetswapChainImageResult = vkGetSwapchainImagesKHR(pDeviceHandel, pSwapchainHandel, &swapChainImageCount, pSwapchainImagesHandel);
    _VkResultsCheck(GetswapChainImageResult, "vkGetSwapchainImagesKHR");

    pViewHandel = malloc((swapChainImageCount+1)*sizeof(VkImageView));
    printf("%u\n",swapChainImageCount);
    
    for(uint32_t i = 0; i < swapChainImageCount; i++)
    {
        VkImageViewCreateInfo image_view_createinfo = {0};
        image_view_createinfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        image_view_createinfo.image = pSwapchainImagesHandel[i];
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

        VkResult CreateImageViewResult = vkCreateImageView(pDeviceHandel, &image_view_createinfo, NULL, &pViewHandel[i]);
    }

    free(pSwapchainImagesHandel);
}

void _vulakn_destory_swapchain()
{
    vkDestroySwapchainKHR(pDeviceHandel, pSwapchainHandel, NULL);
    for(uint32_t i = 0; i < swapChainImageCount; i++)
    {
        vkDestroyImageView(pDeviceHandel, pViewHandel[i], NULL);
    }
    free(pViewHandel);
}

static void _vulkan_cleanup()
{
    _vulakn_destory_swapchain();
    vkDestroySurfaceKHR(vkContext.instance, vkContext.surface, NULL);
    vkDestroyDevice(pDeviceHandel, NULL);
    vkDestroyDebugUtilsMessengerEXT(vkContext.instance, pmessage, NULL);
    vkDestroyDebugReportCallbackEXT(vkContext.instance, pCallback, NULL);
    vkDestroyInstance(vkContext.instance, NULL);
}


static void _vulkan_initcommands()
{

}
static void _vulkan_sync_structures()
{

}