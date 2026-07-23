#define GLFW_INCLUDE_VULKAN
#include "vulkan/vulkan.h"
#include <GLFW/glfw3.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "vk_engine.h"

static void _vulakn_init();
static void _vulkan_swapchain();
static void _vulkan_initcommands();
static void _vulkan_sync_structures();

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
}



VkSwapchainKHR pSwapchain;
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
    printf("%s\n",extensions[1]);

    const char **ppEnabledExtensionNames = malloc((enabledExtensionCount + 1) * sizeof(const char *));
    memcpy(ppEnabledExtensionNames, extensions, enabledExtensionCount * sizeof(const char*)); 
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
    instance_createinfo.enabledExtensionCount = enabledExtensionCount;
    instance_createinfo.ppEnabledExtensionNames = ppEnabledExtensionNames;
    instance_createinfo.enabledLayerCount = enabledLayerCount;
    instance_createinfo.ppEnabledLayerNames = ppEnabledLayerNames;


    VkResult instanceResult = vkCreateInstance(&instance_createinfo, NULL, &vkContext.instance);
    _VkResultsCheck(instanceResult, "INSTANCE_CREATE");
    _vulakn_PhysicalDevices();

    VkResult CreateSurfaceResult = glfwCreateWindowSurface(vkContext.instance, window, NULL, &vkContext.surface);
    _VkResultsCheck(CreateSurfaceResult, "Create surface");
    
    VkExtent2D vksurface_dimension = {640, 480};
    VkSwapchainCreateInfoKHR swapchain_createInfo = {0};
    swapchain_createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchain_createInfo.surface = vkContext.surface;
    swapchain_createInfo.minImageCount = 4;
    swapchain_createInfo.imageFormat = VK_FORMAT_R8G8B8A8_SRGB; //might break on some dispaly things (fuck you wayland)
    swapchain_createInfo.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    swapchain_createInfo.imageExtent = vksurface_dimension; //look at vkGetPhysicalDeviceSurfaceCapabilitiesKHR later
    swapchain_createInfo.imageArrayLayers = 1;
    swapchain_createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchain_createInfo.compositeAlpha = 0x00000001; //VkCompositeAlphaFlagBitsKHR.VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR
    swapchain_createInfo.preTransform = 0x00000001; //VkSurfaceTransformFlagBitsKHR.VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR
    swapchain_createInfo.presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;

    VkResult createSwapResult = vkCreateSwapchainKHR(pDeviceHandel, &swapchain_createInfo, NULL, &pSwapchain);
    _VkResultsCheck(createSwapResult, "Create Swap chain");

    free(ppEnabledExtensionNames);
    free(ppEnabledLayerNames);

}

static void _vulkan_swapchain()
{

}
static void _vulkan_initcommands()
{

}
static void _vulkan_sync_structures()
{

}