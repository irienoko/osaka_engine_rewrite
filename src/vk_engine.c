#define GLFW_INCLUDE_VULKAN
#include "../misc/vulkan/vulkan.h"
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
    VkPhysicalDeviceProperties2 pProperties;
    pProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    pProperties.pNext = NULL;

    //https://docs.vulkan.org/spec/latest/chapters/features.html#vkGetPhysicalDeviceFeatures2
    VkPhysicalDeviceFeatures2 pFeatures;
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
    VkDeviceQueueCreateInfo pQueueCreateInfos =
    {
        VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        NULL,
        0,
        queueFamilyIndex,
        1,
        &queuePriority
    };
    
    VkPhysicalDeviceFeatures requiredDeviceFeatures;
    requiredDeviceFeatures.geometryShader = VK_TRUE;
    requiredDeviceFeatures.tessellationShader = VK_TRUE;

    VkDeviceCreateInfo pCreateInfo = 
    {
        VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        NULL,
        0,
        1,
        &pQueueCreateInfos,
        0,
        NULL,
        2,
        NULL,
        &requiredDeviceFeatures
    };


    printf("selected gpu index %u\n",SELECTED_GPU_INDEX);
    VkResult CreateDeviceResult = vkCreateDevice(pPhysicalDevices[SELECTED_GPU_INDEX], &pCreateInfo, NULL, &pDeviceHandel);
    _VkResultsCheck(CreateDeviceResult, "Create Device");
}


static void _vulakn_init()
{
    uint32_t Vulakn_apiVersion = VK_API_VERSION_1_1;
    VkApplicationInfo appInfo = {VK_STRUCTURE_TYPE_APPLICATION_INFO};
    appInfo.pApplicationName = "Potema3D";
    appInfo.applicationVersion = VK_MAKE_VERSION(0,0,1);
    appInfo.pEngineName = "Potema3D engine";
    appInfo.engineVersion = VK_MAKE_VERSION(0, 0, 1);
    appInfo.apiVersion = Vulakn_apiVersion;

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

    const char **ppEnabledExtensionNames = malloc((enabledExtensionCount + 1) * sizeof(const char *));
    memcpy(ppEnabledExtensionNames, extensions, enabledExtensionCount * sizeof(const char*)); 
    uint32_t enabledLayerCount = 0;
    #ifdef VGRAPHICS_VERBOSE
        enabledLayerCount++;
    #endif
    
    const char** ppEnabledLayerNames = NULL;
    if(enabledLayerCount > 0) ppEnabledLayerNames = malloc(enabledLayerCount * sizeof(const char*));

    VkInstanceCreateInfo instanceCreateInfo = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    instanceCreateInfo.pNext = NULL;
    instanceCreateInfo.flags = 0x00000001; //VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR flag from [VkInstanceCreateFlagBits]
    instanceCreateInfo.pApplicationInfo = &appInfo;
    instanceCreateInfo.enabledExtensionCount = enabledExtensionCount;
    instanceCreateInfo.ppEnabledExtensionNames = ppEnabledExtensionNames;
    instanceCreateInfo.enabledLayerCount = enabledLayerCount;
    instanceCreateInfo.ppEnabledLayerNames = ppEnabledLayerNames;


    VkResult instanceResult = vkCreateInstance(&instanceCreateInfo, NULL, &vkContext.instance);
    _VkResultsCheck(instanceResult, "INSTANCE_CREATE");
    _vulakn_PhysicalDevices();

    VkResult CreateSurfaceResult = glfwCreateWindowSurface(vkContext.instance, window, NULL, &vkContext.surface);
    _VkResultsCheck(CreateSurfaceResult, "Create surface");

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