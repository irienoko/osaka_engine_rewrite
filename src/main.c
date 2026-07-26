#include "vk_engine.h"

int main()
{
    VKEngine_init();
    VkEngine_run();

    
    VKEngine_draw();
    VkEngine_cleanup();
    return 0;
}