#include "VulkanRenderer.h"

#if defined(GECROSS_HAS_VULKAN)
#include <vulkan/vulkan.h>
#endif
#if defined(GECROSS_HAS_SDL2)
#include <SDL.h>
#include <SDL_vulkan.h>
#endif

#include <memory>
#include <cstring>
#include <iostream>

struct VulkanRenderer::Impl
{
#if defined(GECROSS_HAS_VULKAN)
    VkInstance instance{ VK_NULL_HANDLE };
    VkPhysicalDevice phys{ VK_NULL_HANDLE };
    VkDevice device{ VK_NULL_HANDLE };
    VkQueue queue{ VK_NULL_HANDLE };
    uint32_t queueFamily{ 0 };
    VkSurfaceKHR surface{ VK_NULL_HANDLE };
    VkSwapchainKHR swapchain{ VK_NULL_HANDLE };
    std::vector<VkImage> swapImages;
    std::vector<VkImageView> swapViews;
    VkFormat swapFormat{ VK_FORMAT_B8G8R8A8_UNORM };
    VkExtent2D swapExtent{ 0,0 };
    VkRenderPass renderPass{ VK_NULL_HANDLE };
    std::vector<VkFramebuffer> framebuffers;
    VkCommandPool cmdPool{ VK_NULL_HANDLE };
    std::vector<VkCommandBuffer> cmdBuffers;
    VkSemaphore imageAvail{ VK_NULL_HANDLE };
    VkSemaphore renderDone{ VK_NULL_HANDLE };
    VkFence inFlight{ VK_NULL_HANDLE };
    VkImage stagingImage{ VK_NULL_HANDLE };
    VkDeviceMemory stagingMem{ VK_NULL_HANDLE };
    VkExtent2D stagingExtent{ 0,0 };
#endif
#if defined(GECROSS_HAS_SDL2)
    SDL_Window* window{ nullptr };
#endif
    uint32_t width{ 0 }, height{ 0 };
};

bool VulkanRenderer::Init(uint32_t width, uint32_t height, const std::string& title)
{
    m_impl = new Impl();
    m_impl->width = width;
    m_impl->height = height;

#if defined(GECROSS_HAS_VULKAN) && defined(GECROSS_HAS_SDL2)
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        std::cerr << "SDL init failed: " << SDL_GetError() << "\n";
        return false;
    }
    m_impl->window = SDL_CreateWindow(title.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        static_cast<int>(width), static_cast<int>(height), SDL_WINDOW_VULKAN | SDL_WINDOW_SHOWN);
    if (!m_impl->window)
    {
        std::cerr << "SDL window failed: " << SDL_GetError() << "\n";
        return false;
    }

    // Instance
    unsigned extCount = 0;
    if (!SDL_Vulkan_GetInstanceExtensions(m_impl->window, &extCount, nullptr))
    {
        std::cerr << "SDL_Vulkan_GetInstanceExtensions failed\n";
        return false;
    }
    std::vector<const char*> exts(extCount);
    SDL_Vulkan_GetInstanceExtensions(m_impl->window, &extCount, exts.data());

    VkApplicationInfo app{ VK_STRUCTURE_TYPE_APPLICATION_INFO };
    app.pApplicationName = "GameEngineCross";
    app.apiVersion = VK_API_VERSION_1_1;

    VkInstanceCreateInfo ci{ VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
    ci.pApplicationInfo = &app;
    ci.enabledExtensionCount = static_cast<uint32_t>(exts.size());
    ci.ppEnabledExtensionNames = exts.data();

    if (vkCreateInstance(&ci, nullptr, &m_impl->instance) != VK_SUCCESS)
    {
        std::cerr << "vkCreateInstance failed\n";
        return false;
    }

    // Surface
    if (!SDL_Vulkan_CreateSurface(m_impl->window, m_impl->instance, &m_impl->surface))
    {
        std::cerr << "SDL_Vulkan_CreateSurface failed\n";
        return false;
    }

    // Physical device pick (first)
    uint32_t gpuCount = 0;
    vkEnumeratePhysicalDevices(m_impl->instance, &gpuCount, nullptr);
    if (gpuCount == 0) { std::cerr << "No Vulkan GPU\n"; return false; }
    std::vector<VkPhysicalDevice> gpus(gpuCount);
    vkEnumeratePhysicalDevices(m_impl->instance, &gpuCount, gpus.data());
    m_impl->phys = gpus[0];

    // Queue family
    uint32_t qCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(m_impl->phys, &qCount, nullptr);
    std::vector<VkQueueFamilyProperties> qprops(qCount);
    vkGetPhysicalDeviceQueueFamilyProperties(m_impl->phys, &qCount, qprops.data());
    bool found = false;
    for (uint32_t i = 0; i < qCount; ++i)
    {
        VkBool32 present = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(m_impl->phys, i, m_impl->surface, &present);
        if ((qprops[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && present)
        {
            m_impl->queueFamily = i;
            found = true;
            break;
        }
    }
    if (!found) { std::cerr << "No graphics+present queue\n"; return false; }

    float qPrior = 1.0f;
    VkDeviceQueueCreateInfo qci{ VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
    qci.queueFamilyIndex = m_impl->queueFamily;
    qci.queueCount = 1;
    qci.pQueuePriorities = &qPrior;

    VkDeviceCreateInfo dci{ VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
    dci.queueCreateInfoCount = 1;
    dci.pQueueCreateInfos = &qci;

    if (vkCreateDevice(m_impl->phys, &dci, nullptr, &m_impl->device) != VK_SUCCESS)
    {
        std::cerr << "vkCreateDevice failed\n";
        return false;
    }
    vkGetDeviceQueue(m_impl->device, m_impl->queueFamily, 0, &m_impl->queue);

    // Swapchain
    VkSurfaceCapabilitiesKHR caps{};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_impl->phys, m_impl->surface, &caps);
    m_impl->swapExtent = caps.currentExtent;
    m_impl->swapFormat = VK_FORMAT_B8G8R8A8_UNORM;

    VkSwapchainCreateInfoKHR sci{ VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
    sci.surface = m_impl->surface;
    sci.minImageCount = std::max(2u, caps.minImageCount);
    sci.imageFormat = m_impl->swapFormat;
    sci.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    sci.imageExtent = m_impl->swapExtent;
    sci.imageArrayLayers = 1;
    sci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    sci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    sci.preTransform = caps.currentTransform;
    sci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    sci.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    sci.clipped = VK_TRUE;

    if (vkCreateSwapchainKHR(m_impl->device, &sci, nullptr, &m_impl->swapchain) != VK_SUCCESS)
    {
        std::cerr << "vkCreateSwapchain failed\n";
        return false;
    }
    uint32_t imgCount = 0;
    vkGetSwapchainImagesKHR(m_impl->device, m_impl->swapchain, &imgCount, nullptr);
    m_impl->swapImages.resize(imgCount);
    vkGetSwapchainImagesKHR(m_impl->device, m_impl->swapchain, &imgCount, m_impl->swapImages.data());

    // Image views
    m_impl->swapViews.resize(imgCount);
    for (uint32_t i = 0; i < imgCount; ++i)
    {
        VkImageViewCreateInfo ivci{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
        ivci.image = m_impl->swapImages[i];
        ivci.viewType = VK_IMAGE_VIEW_TYPE_2D;
        ivci.format = m_impl->swapFormat;
        ivci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        ivci.subresourceRange.levelCount = 1;
        ivci.subresourceRange.layerCount = 1;
        vkCreateImageView(m_impl->device, &ivci, nullptr, &m_impl->swapViews[i]);
    }

    // Render pass
    VkAttachmentDescription color{};
    color.format = m_impl->swapFormat;
    color.samples = VK_SAMPLE_COUNT_1_BIT;
    color.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorRef{ 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

    VkSubpassDescription sub{};
    sub.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    sub.colorAttachmentCount = 1;
    sub.pColorAttachments = &colorRef;

    VkRenderPassCreateInfo rpci{ VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
    rpci.attachmentCount = 1;
    rpci.pAttachments = &color;
    rpci.subpassCount = 1;
    rpci.pSubpasses = &sub;
    vkCreateRenderPass(m_impl->device, &rpci, nullptr, &m_impl->renderPass);

    // Framebuffers
    m_impl->framebuffers.resize(imgCount);
    for (uint32_t i = 0; i < imgCount; ++i)
    {
        VkImageView attachments[] = { m_impl->swapViews[i] };
        VkFramebufferCreateInfo fbci{ VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
        fbci.renderPass = m_impl->renderPass;
        fbci.attachmentCount = 1;
        fbci.pAttachments = attachments;
        fbci.width = m_impl->swapExtent.width;
        fbci.height = m_impl->swapExtent.height;
        fbci.layers = 1;
        vkCreateFramebuffer(m_impl->device, &fbci, nullptr, &m_impl->framebuffers[i]);
    }

    // Command pool + buffers
    VkCommandPoolCreateInfo cpci{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
    cpci.queueFamilyIndex = m_impl->queueFamily;
    vkCreateCommandPool(m_impl->device, &cpci, nullptr, &m_impl->cmdPool);

    m_impl->cmdBuffers.resize(imgCount);
    VkCommandBufferAllocateInfo cbai{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
    cbai.commandPool = m_impl->cmdPool;
    cbai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cbai.commandBufferCount = imgCount;
    vkAllocateCommandBuffers(m_impl->device, &cbai, m_impl->cmdBuffers.data());

    m_impl->stagingExtent = m_impl->swapExtent;
    // staging image creation deferred until first frame; recreated on size change

    // Sync
    VkSemaphoreCreateInfo sci2{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
    vkCreateSemaphore(m_impl->device, &sci2, nullptr, &m_impl->imageAvail);
    vkCreateSemaphore(m_impl->device, &sci2, nullptr, &m_impl->renderDone);
    VkFenceCreateInfo fci{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
    fci.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    vkCreateFence(m_impl->device, &fci, nullptr, &m_impl->inFlight);

    return true;
#else
    (void)width; (void)height; (void)title;
    std::cerr << "Vulkan/SDL2 not available; skipping windowed renderer\n";
    return false;
#endif
}

bool VulkanRenderer::RenderOnce(float r, float g, float b)
{
#if defined(GECROSS_HAS_VULKAN) && defined(GECROSS_HAS_SDL2)
    if (!m_impl) return false;
    vkWaitForFences(m_impl->device, 1, &m_impl->inFlight, VK_TRUE, UINT64_MAX);
    vkResetFences(m_impl->device, 1, &m_impl->inFlight);

    uint32_t imgIndex = 0;
    vkAcquireNextImageKHR(m_impl->device, m_impl->swapchain, UINT64_MAX, m_impl->imageAvail, VK_NULL_HANDLE, &imgIndex);

    VkCommandBuffer cb = m_impl->cmdBuffers[imgIndex];
    vkResetCommandBuffer(cb, 0);
    VkCommandBufferBeginInfo bi{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    vkBeginCommandBuffer(cb, &bi);

    VkClearValue clear{};
    clear.color = { { r, g, b, 1.0f } };
    VkRenderPassBeginInfo rpbi{ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
    rpbi.renderPass = m_impl->renderPass;
    rpbi.framebuffer = m_impl->framebuffers[imgIndex];
    rpbi.renderArea.extent = m_impl->swapExtent;
    rpbi.clearValueCount = 1;
    rpbi.pClearValues = &clear;
    vkCmdBeginRenderPass(cb, &rpbi, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdEndRenderPass(cb);
    vkEndCommandBuffer(cb);

    VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo si{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
    si.waitSemaphoreCount = 1;
    si.pWaitSemaphores = &m_impl->imageAvail;
    si.pWaitDstStageMask = &waitStage;
    si.commandBufferCount = 1;
    si.pCommandBuffers = &cb;
    si.signalSemaphoreCount = 1;
    si.pSignalSemaphores = &m_impl->renderDone;
    vkQueueSubmit(m_impl->queue, 1, &si, m_impl->inFlight);

    VkPresentInfoKHR pi{ VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
    pi.waitSemaphoreCount = 1;
    pi.pWaitSemaphores = &m_impl->renderDone;
    pi.swapchainCount = 1;
    pi.pSwapchains = &m_impl->swapchain;
    pi.pImageIndices = &imgIndex;
    vkQueuePresentKHR(m_impl->queue, &pi);
    return true;
#else
    (void)r; (void)g; (void)b;
    return false;
#endif
}

bool VulkanRenderer::RenderBGRAFrame(uint32_t width, uint32_t height, const uint8_t* data)
{
#if defined(GECROSS_HAS_VULKAN) && defined(GECROSS_HAS_SDL2)
    if (!m_impl || !data) return false;

    // (Re)create staging image if size changed
    if (m_impl->stagingImage == VK_NULL_HANDLE || width != m_impl->stagingExtent.width || height != m_impl->stagingExtent.height)
    {
        if (m_impl->stagingImage) vkDestroyImage(m_impl->device, m_impl->stagingImage, nullptr);
        if (m_impl->stagingMem) vkFreeMemory(m_impl->device, m_impl->stagingMem, nullptr);

        VkImageCreateInfo ici{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
        ici.imageType = VK_IMAGE_TYPE_2D;
        ici.extent = { width, height, 1 };
        ici.mipLevels = 1;
        ici.arrayLayers = 1;
        ici.format = m_impl->swapFormat;
        ici.tiling = VK_IMAGE_TILING_LINEAR;
        ici.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
        ici.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        ici.samples = VK_SAMPLE_COUNT_1_BIT;
        vkCreateImage(m_impl->device, &ici, nullptr, &m_impl->stagingImage);

        VkMemoryRequirements memReq{};
        vkGetImageMemoryRequirements(m_impl->device, m_impl->stagingImage, &memReq);
        VkPhysicalDeviceMemoryProperties memProps{};
        vkGetPhysicalDeviceMemoryProperties(m_impl->phys, &memProps);
        uint32_t chosen = UINT32_MAX;
        for (uint32_t i = 0; i < memProps.memoryTypeCount; ++i)
        {
            if ((memReq.memoryTypeBits & (1u << i)) &&
                (memProps.memoryTypes[i].propertyFlags & (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) ==
                (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
            {
                chosen = i;
                break;
            }
        }
        if (chosen == UINT32_MAX)
        {
            std::cerr << "No HOST_VISIBLE memory for staging image\n";
            return false;
        }
        VkMemoryAllocateInfo mai{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
        mai.allocationSize = memReq.size;
        mai.memoryTypeIndex = chosen;
        vkAllocateMemory(m_impl->device, &mai, nullptr, &m_impl->stagingMem);
        vkBindImageMemory(m_impl->device, m_impl->stagingImage, m_impl->stagingMem, 0);
        m_impl->stagingExtent = { width, height };
    }

    VkImageSubresource sub{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 0 };
    VkSubresourceLayout layout{};
    vkGetImageSubresourceLayout(m_impl->device, m_impl->stagingImage, &sub, &layout);
    char* mapped = nullptr;
    vkMapMemory(m_impl->device, m_impl->stagingMem, 0, VK_WHOLE_SIZE, 0, reinterpret_cast<void**>(&mapped));
    for (uint32_t y = 0; y < height; ++y)
    {
        std::memcpy(mapped + layout.offset + layout.rowPitch * y, data + width * 4 * y, width * 4);
    }
    vkUnmapMemory(m_impl->device, m_impl->stagingMem);

    vkWaitForFences(m_impl->device, 1, &m_impl->inFlight, VK_TRUE, UINT64_MAX);
    vkResetFences(m_impl->device, 1, &m_impl->inFlight);

    uint32_t imgIndex = 0;
    vkAcquireNextImageKHR(m_impl->device, m_impl->swapchain, UINT64_MAX, m_impl->imageAvail, VK_NULL_HANDLE, &imgIndex);
    VkImage target = m_impl->swapImages[imgIndex];

    VkCommandBuffer cb = m_impl->cmdBuffers[imgIndex];
    vkResetCommandBuffer(cb, 0);
    VkCommandBufferBeginInfo bi{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    vkBeginCommandBuffer(cb, &bi);

    // Transition swap image to TRANSFER_DST
    VkImageMemoryBarrier toDst{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
    toDst.srcAccessMask = 0;
    toDst.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    toDst.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    toDst.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    toDst.image = target;
    toDst.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    toDst.subresourceRange.levelCount = 1;
    toDst.subresourceRange.layerCount = 1;
    vkCmdPipelineBarrier(cb, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
        0, nullptr, 0, nullptr, 1, &toDst);

    // Transition staging to TRANSFER_SRC
    VkImageMemoryBarrier stagingBarrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
    stagingBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
    stagingBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    stagingBarrier.oldLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
    stagingBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    stagingBarrier.image = m_impl->stagingImage;
    stagingBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    stagingBarrier.subresourceRange.levelCount = 1;
    stagingBarrier.subresourceRange.layerCount = 1;
    vkCmdPipelineBarrier(cb, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
        0, nullptr, 0, nullptr, 1, &stagingBarrier);

    // Blit with scaling
    VkImageBlit blit{};
    blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blit.srcSubresource.layerCount = 1;
    blit.srcOffsets[1] = { static_cast<int32_t>(width), static_cast<int32_t>(height), 1 };
    blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blit.dstSubresource.layerCount = 1;
    blit.dstOffsets[1] = { static_cast<int32_t>(m_impl->swapExtent.width), static_cast<int32_t>(m_impl->swapExtent.height), 1 };
    vkCmdBlitImage(cb,
        m_impl->stagingImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        target, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1, &blit, VK_FILTER_LINEAR);

    // Transition swap image to PRESENT
    VkImageMemoryBarrier toPresent{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
    toPresent.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    toPresent.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    toPresent.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    toPresent.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    toPresent.image = target;
    toPresent.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    toPresent.subresourceRange.levelCount = 1;
    toPresent.subresourceRange.layerCount = 1;
    vkCmdPipelineBarrier(cb, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0,
        0, nullptr, 0, nullptr, 1, &toPresent);

    vkEndCommandBuffer(cb);

    VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    VkSubmitInfo si{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
    si.waitSemaphoreCount = 1;
    si.pWaitSemaphores = &m_impl->imageAvail;
    si.pWaitDstStageMask = &waitStage;
    si.commandBufferCount = 1;
    si.pCommandBuffers = &cb;
    si.signalSemaphoreCount = 1;
    si.pSignalSemaphores = &m_impl->renderDone;
    vkQueueSubmit(m_impl->queue, 1, &si, m_impl->inFlight);

    VkPresentInfoKHR pi{ VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
    pi.waitSemaphoreCount = 1;
    pi.pWaitSemaphores = &m_impl->renderDone;
    pi.swapchainCount = 1;
    pi.pSwapchains = &m_impl->swapchain;
    pi.pImageIndices = &imgIndex;
    vkQueuePresentKHR(m_impl->queue, &pi);
    return true;
#else
    (void)width; (void)height; (void)data;
    return false;
#endif
}
void VulkanRenderer::Shutdown()
{
#if defined(GECROSS_HAS_VULKAN)
    if (!m_impl) return;
    auto& I = *m_impl;
    vkDeviceWaitIdle(I.device);
    if (I.inFlight) vkDestroyFence(I.device, I.inFlight, nullptr);
    if (I.imageAvail) vkDestroySemaphore(I.device, I.imageAvail, nullptr);
    if (I.renderDone) vkDestroySemaphore(I.device, I.renderDone, nullptr);
    for (auto fb : I.framebuffers) if (fb) vkDestroyFramebuffer(I.device, fb, nullptr);
    if (I.cmdPool) vkDestroyCommandPool(I.device, I.cmdPool, nullptr);
    for (auto v : I.swapViews) if (v) vkDestroyImageView(I.device, v, nullptr);
    if (I.swapchain) vkDestroySwapchainKHR(I.device, I.swapchain, nullptr);
    if (I.stagingImage) vkDestroyImage(I.device, I.stagingImage, nullptr);
    if (I.stagingMem) vkFreeMemory(I.device, I.stagingMem, nullptr);
    if (I.device) vkDestroyDevice(I.device, nullptr);
    if (I.surface) vkDestroySurfaceKHR(I.instance, I.surface, nullptr);
    if (I.instance) vkDestroyInstance(I.instance, nullptr);
#endif
#if defined(GECROSS_HAS_SDL2)
    if (m_impl && m_impl->window) SDL_DestroyWindow(m_impl->window);
    SDL_Quit();
#endif
    delete m_impl;
    m_impl = nullptr;
}
