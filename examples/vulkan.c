/*===========================================================================/

  GLV Vulkan example.
  Copyright (C) 2016  Karl Robillard

  This example shows how to open a GLView in a desktop window.
  Press the escape key or click on the window close widget to exit.

# gcc -DVK_USE_PLATFORM_XLIB_KHR -I../x11 -I$VULKAN_SDK/include vulkan.c -L../x11 -lglv-vk -L$VULKAN_SDK/lib -lvulkan -g -Wall
  gcc -DVK_USE_PLATFORM_XLIB_KHR -I../x11 vulkan.c -L../x11 -lglv-vk -lvulkan -g -Wall

/===========================================================================*/


#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "glv.h"
#include "glv_keys.h"


typedef struct
{
    VkImage image;
    VkCommandBuffer cmd;
    VkImageView view;
}
SwapchainBuffers;


typedef struct
{
    VkSampler sampler;
    VkImage image;
    VkImageLayout imageLayout;
    VkDeviceMemory mem;
    VkImageView view;
    int32_t texWidth;
    int32_t texHeight;
}
TextureObject;


typedef struct
{
    VkBuffer buf;
    VkDeviceMemory mem;
    VkPipelineVertexInputStateCreateInfo vi;
    VkVertexInputBindingDescription vi_bindings[1];
    VkVertexInputAttributeDescription vi_attrs[2];
}
Vertices;


#define DEMO_TEXTURE_COUNT      1
#define VERTEX_BUFFER_BIND_ID   0

typedef struct
{
    VkDevice device;    // Copy of GLView device.

    VkFormat format;
    VkColorSpaceKHR colorSpace;
    VkPhysicalDeviceMemoryProperties memProp;
    VkCommandPool cpool;
    VkCommandBuffer setup;
    VkCommandBuffer draw;
    VkPipelineLayout pipelineLayout;
    VkDescriptorSetLayout descLayout;
    VkPipelineCache pipelineCache;
    VkRenderPass renderPass;
    VkPipeline pipeline;

    VkDescriptorPool descPool;
    VkDescriptorSet descSet;

    VkFramebuffer* framebuffers;

    VkSwapchainKHR swapchain;
    SwapchainBuffers* buffers;
    uint32_t swapchainImageCount;
    uint32_t currentBuffer;
    int  surfWidth;
    int  surfHeight;
    char useStagingBuffer;
    char prepared;
    char doRepaint;
    char quit;

    float depthStencil;
    float depthIncrement;

    struct
    {
        VkFormat format;
        VkImage image;
        VkDeviceMemory mem;
        VkImageView view;
    }
    depth;

    TextureObject textures[ DEMO_TEXTURE_COUNT ];
    Vertices vertices;

    PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR   SurfaceCapabilities;
    PFN_vkGetPhysicalDeviceSurfaceFormatsKHR        SurfaceFormats;
    //PFN_vkGetPhysicalDeviceSurfacePresentModesKHR   SurfacePresentModes;
    PFN_vkCreateSwapchainKHR        CreateSwapchain;
    PFN_vkDestroySwapchainKHR       DestroySwapchain;
    PFN_vkGetSwapchainImagesKHR     SwapchainImages;
    PFN_vkAcquireNextImageKHR       AcquireNextImage;
    PFN_vkQueuePresentKHR           QueuePresent;

    PFN_vkDestroyDebugReportCallbackEXT DestroyDebugReport;
    VkDebugReportCallbackEXT report;
}
VulkanState;


enum DoRepaint
{
    DO_REPAINT_IGNORE,
    DO_REPAINT_DRAW,
    DO_REPAINT_RESIZE
};


#if 0
static const char* vert_shader =
    "#version 400\n"
    "#extension GL_ARB_separate_shader_objects : enable\n"
    "#extension GL_ARB_shading_language_420pack : enable\n"
    "layout (location = 0) in vec4 pos;\n"
    "layout (location = 1) in vec2 attr;\n"
    "layout (location = 0) out vec2 texcoord;\n"
    "out gl_PerVertex {\n"
    "    vec4 gl_Position;\n"
    "};\n"
    "void main() {\n"
    "    texcoord = attr;\n"
    "    gl_Position = pos;\n"
    "}\n";

static const char* frag_shader =
    "#version 400\n"
    "#extension GL_ARB_separate_shader_objects : enable\n"
    "#extension GL_ARB_shading_language_420pack : enable\n"
    "layout (binding = 0) uniform sampler2D tex;\n"
    "layout (location = 0) in vec2 texcoord;\n"
    "layout (location = 0) out vec4 uFragColor;\n"
    "void main() {\n"
    "    uFragColor = texture(tex, texcoord);\n"
    "}\n";
#endif


//----------------------------------------------------------------------------


static VkBool32 debugCallback(
    VkDebugReportFlagsEXT flags,
    VkDebugReportObjectTypeEXT objType,
    uint64_t object, size_t location, int32_t msgCode,
    const char* layerPrefix, const char* message, void* user )
{
    const char* sev = "Report";
    (void) objType;
    (void) object;
    (void) location;
    (void) user;

    if( flags & VK_DEBUG_REPORT_ERROR_BIT_EXT )
        sev = "ERROR";
    else if( flags & VK_DEBUG_REPORT_WARNING_BIT_EXT )
        sev = "Warning";
    else if( flags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT )
        sev = "Info";

    fprintf( stderr, "%s: [%s %d] %s\n",
             sev, layerPrefix, msgCode, message );
    return VK_FALSE;
}


/*
    Search memtypes to find first index with those properties
*/
static int memoryType( VkPhysicalDeviceMemoryProperties* memProp,
                       uint32_t typeBits,
                       VkFlags requirements_mask, uint32_t* typeIndex )
{
    uint32_t i;
    for( i = 0; i < VK_MAX_MEMORY_TYPES; i++ )
    {
        if( (typeBits & 1) == 1 )
        {
            // Type is available, does it match user properties?
            if( (memProp->memoryTypes[i].propertyFlags & requirements_mask)
                == requirements_mask )
            {
                *typeIndex = i;
                return 1;
            }
        }
        typeBits >>= 1;
    }
    return 0;   // No memory types matched, return failure
}


void vert_init( VulkanState* vs, Vertices* vobj, const float* vattr,
                uint32_t byteSize, uint32_t byteStride )
{
    VkBufferCreateInfo bc;
    VkMemoryAllocateInfo ma;
    VkMemoryRequirements mem_reqs;
    VkResult err;
    int pass;
    void* data;


    memset( vobj, 0, sizeof(Vertices) );

    bc.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bc.pNext = NULL;
    bc.size  = byteSize;
    bc.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    bc.flags = 0;

    err = vkCreateBuffer( vs->device, &bc, NULL, &vobj->buf );
    assert(!err);

    vkGetBufferMemoryRequirements( vs->device, vobj->buf, &mem_reqs );

    ma.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    ma.pNext = NULL;
    ma.allocationSize = mem_reqs.size;
    ma.memoryTypeIndex = 0;

    pass = memoryType( &vs->memProp, mem_reqs.memoryTypeBits,
                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                       VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                       &ma.memoryTypeIndex );
    assert(pass);

    err = vkAllocateMemory( vs->device, &ma, NULL, &vobj->mem );
    assert(!err);

    err = vkMapMemory( vs->device, vobj->mem, 0, ma.allocationSize, 0, &data );
    assert(!err);

    memcpy( data, vattr, byteSize );

    vkUnmapMemory( vs->device, vobj->mem );

    err = vkBindBufferMemory( vs->device, vobj->buf, vobj->mem, 0 );
    assert(!err);

    vobj->vi.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vobj->vi.pNext = NULL;
    vobj->vi.vertexBindingDescriptionCount = 1;
    vobj->vi.pVertexBindingDescriptions = vobj->vi_bindings;
    vobj->vi.vertexAttributeDescriptionCount = 2;
    vobj->vi.pVertexAttributeDescriptions = vobj->vi_attrs;

    vobj->vi_bindings[0].binding   = VERTEX_BUFFER_BIND_ID;
    vobj->vi_bindings[0].stride    = byteStride;
    vobj->vi_bindings[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    vobj->vi_attrs[0].binding  = VERTEX_BUFFER_BIND_ID;
    vobj->vi_attrs[0].location = 0;
    vobj->vi_attrs[0].format   = VK_FORMAT_R32G32B32_SFLOAT;
    vobj->vi_attrs[0].offset   = 0;

    vobj->vi_attrs[1].binding  = VERTEX_BUFFER_BIND_ID;
    vobj->vi_attrs[1].location = 1;
    vobj->vi_attrs[1].format   = VK_FORMAT_R32G32_SFLOAT;
    vobj->vi_attrs[1].offset   = sizeof(float) * 3;
}


void vert_free( VkDevice device, Vertices* vobj )
{
    vkDestroyBuffer( device, vobj->buf, NULL );
    vkFreeMemory( device, vobj->mem, NULL );
}


//----------------------------------------------------------------------------


static void flushInitCmd( VkQueue queue, VulkanState* vs )
{
    VkSubmitInfo si;
    VkResult err;

    if( vs->setup == VK_NULL_HANDLE )
        return;

    err = vkEndCommandBuffer( vs->setup );
    assert(!err);

    si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    si.pNext = NULL;
    si.waitSemaphoreCount = 0;
    si.pWaitSemaphores = NULL;
    si.pWaitDstStageMask = NULL;
    si.commandBufferCount = 1;
    si.pCommandBuffers = &vs->setup;
    si.signalSemaphoreCount = 0;
    si.pSignalSemaphores = NULL;

    err = vkQueueSubmit( queue, 1, &si, VK_NULL_HANDLE );
    assert(!err);

    err = vkQueueWaitIdle( queue );
    assert(!err);

    vkFreeCommandBuffers( vs->device, vs->cpool, 1, &vs->setup );
    vs->setup = VK_NULL_HANDLE;
}


static const VkImageSubresourceRange ISR_color1 = {
    VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1
};

static void setupBuffers( const GLView* view, VulkanState* vs )
{
    VkResult err;
    VkSurfaceCapabilitiesKHR scap;
    VkSwapchainKHR oldSwapchain = vs->swapchain;
    //VkPresentModeKHR* presentModes;
    VkExtent2D swapchainExtent;
    //uint32_t presentModeCount;
    uint32_t desiredSwapImages;


    // Check the surface capabilities and formats
    err = vs->SurfaceCapabilities( view->gpu, view->surface, &scap );
    assert(!err);

#if 0
    err = vs->SurfacePresentModes( view->gpu, view->surface,
                                   &presentModeCount, NULL);
    assert(!err);
    presentModes = (VkPresentModeKHR*)
                malloc(presentModeCount * sizeof(VkPresentModeKHR));
    assert(presentModes);
    err = vs->SurfacePresentModes( view->gpu, view->surface,
                                   &presentModeCount, presentModes );
    assert(!err);
    free( presentModes );
#endif

    // width and height are either both -1, or both not -1.
    if( scap.currentExtent.width == (uint32_t) -1 )
    {
        // If the surface size is undefined, the size is set to
        // the size of the images requested.
        swapchainExtent.width  = vs->surfWidth  = view->width;
        swapchainExtent.height = vs->surfHeight = view->height;
    }
    else
    {
        // If the surface size is defined, the swap chain size must match
        swapchainExtent = scap.currentExtent;
        vs->surfWidth   = scap.currentExtent.width;
        vs->surfHeight  = scap.currentExtent.height;
        // NOTE: The surface dimensions may not match GLView here as the
        // window events may not have been handled yet.
#if 0
        if( view->width != scap.currentExtent.width ||
            view->height != scap.currentExtent.height )
            printf( "KR surface: %d,%d\n      view: %d,%d\n",
                    scap.currentExtent.width, scap.currentExtent.height,
                    view->width, view->height );
#endif
    }

    // Determine the number of VkImage's to use in the swap chain (we desire to
    // own only 1 image at a time, besides the images being displayed and
    // queued for display):
    desiredSwapImages = scap.minImageCount + 1;
    if( (scap.maxImageCount > 0) && (desiredSwapImages > scap.maxImageCount) )
    {
        // Application must settle for fewer images than desired:
        desiredSwapImages = scap.maxImageCount;
    }

    {
    VkSwapchainCreateInfoKHR sc;
    VkSurfaceTransformFlagsKHR preTransform;

    if( scap.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR )
        preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    else
        preTransform = scap.currentTransform;

    sc.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    sc.pNext = NULL;
    sc.flags = 0;
    sc.surface = view->surface;
    sc.minImageCount = desiredSwapImages;
    sc.imageFormat = vs->format;
    sc.imageColorSpace = vs->colorSpace;
    sc.imageExtent.width  = swapchainExtent.width;
    sc.imageExtent.height = swapchainExtent.height;
    sc.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    sc.preTransform = preTransform;
    sc.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    sc.imageArrayLayers = 1;
    sc.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    sc.queueFamilyIndexCount = 0;
    sc.pQueueFamilyIndices = NULL;
    sc.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    sc.oldSwapchain = oldSwapchain;
    sc.clipped = VK_TRUE;

    err = vs->CreateSwapchain( view->device, &sc, NULL, &vs->swapchain );
    assert(!err);
    }

    // If we just re-created an existing swapchain, we should destroy the old
    // swapchain at this point.
    // Note: destroying the swapchain also cleans up all its associated
    // presentable images once the platform is done with them.
    if( oldSwapchain != VK_NULL_HANDLE )
        vs->DestroySwapchain( view->device, oldSwapchain, NULL );

    err = vs->SwapchainImages( view->device, vs->swapchain,
                               &vs->swapchainImageCount, NULL );
    assert(!err);

    {
    VkImageViewCreateInfo vc;
    VkImage* swapchainImages;
    uint32_t i;

    swapchainImages = (VkImage*)
                      malloc( vs->swapchainImageCount * sizeof(VkImage) );
    assert(swapchainImages);
    err = vs->SwapchainImages( view->device, vs->swapchain,
                               &vs->swapchainImageCount, swapchainImages );
    assert(!err);

    vs->buffers = (SwapchainBuffers*) malloc( sizeof(SwapchainBuffers) *
                                              vs->swapchainImageCount );
    assert(vs->buffers);

    vc.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    vc.pNext = NULL;
    vc.format = vs->format;
    vc.components.r = VK_COMPONENT_SWIZZLE_R;
    vc.components.g = VK_COMPONENT_SWIZZLE_G;
    vc.components.b = VK_COMPONENT_SWIZZLE_B;
    vc.components.a = VK_COMPONENT_SWIZZLE_A;
    vc.subresourceRange = ISR_color1;
    vc.viewType = VK_IMAGE_VIEW_TYPE_2D;
    vc.flags = 0;

    for( i = 0; i < vs->swapchainImageCount; i++ )
    {
        vs->buffers[i].image = swapchainImages[i];
        vc.image = vs->buffers[i].image;

        err = vkCreateImageView( view->device, &vc, NULL,
                                 &vs->buffers[i].view );
        assert(!err);
    }
    }
}


static void drawBuildCmd( VulkanState* vs, int width, int height )
{
    const VkCommandBufferInheritanceInfo cmd_buf_hinfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO,
        .pNext = NULL,
        .renderPass = VK_NULL_HANDLE,
        .subpass = 0,
        .framebuffer = VK_NULL_HANDLE,
        .occlusionQueryEnable = VK_FALSE,
        .queryFlags = 0,
        .pipelineStatistics = 0,
    };
    const VkCommandBufferBeginInfo cmd_buf_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = NULL,
        .flags = 0,
        .pInheritanceInfo = &cmd_buf_hinfo,
    };
    VkClearValue clear_values[2];
    VkRenderPassBeginInfo rp_begin;
    VkImageMemoryBarrier imb;
    VkResult err;


    clear_values[0].color.float32[0] = 0.2f;
    clear_values[0].color.float32[1] = 0.2f;
    clear_values[0].color.float32[2] = 0.2f;
    clear_values[0].color.float32[3] = 0.2f;
    clear_values[1].depthStencil.depth   = vs->depthStencil;
    clear_values[1].depthStencil.stencil = 0;

    rp_begin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rp_begin.pNext = NULL;
    rp_begin.renderPass  = vs->renderPass;
    rp_begin.framebuffer = vs->framebuffers[ vs->currentBuffer ];
    rp_begin.renderArea.offset.x = 0;
    rp_begin.renderArea.offset.y = 0;
    rp_begin.renderArea.extent.width  = width;
    rp_begin.renderArea.extent.height = height;
    rp_begin.clearValueCount = 2;
    rp_begin.pClearValues = clear_values;

    err = vkBeginCommandBuffer(vs->draw, &cmd_buf_info);
    assert(!err);

    // We can use LAYOUT_UNDEFINED as a wildcard here because we don't care what
    // happens to the previous contents of the image
    imb.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    imb.pNext = NULL;
    imb.srcAccessMask = 0;
    imb.dstAccessMask = 0;  // VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
    imb.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imb.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    imb.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imb.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imb.image = vs->buffers[ vs->currentBuffer ].image;
    imb.subresourceRange = ISR_color1;

    vkCmdPipelineBarrier( vs->draw, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                          VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, NULL, 0,
                          NULL, 1, &imb );
    vkCmdBeginRenderPass(vs->draw, &rp_begin, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(vs->draw, VK_PIPELINE_BIND_POINT_GRAPHICS,
                      vs->pipeline);
    vkCmdBindDescriptorSets(vs->draw, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            vs->pipelineLayout, 0, 1, &vs->descSet, 0,
                            NULL);
    {
    VkViewport viewport;

    viewport.x =
    viewport.y = 0.0f;
    viewport.width  = (float) width;
    viewport.height = (float) height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    vkCmdSetViewport( vs->draw, 0, 1, &viewport );
    }

    {
    VkRect2D scissor;

    scissor.offset.x =
    scissor.offset.y = 0;
    scissor.extent.width  = width;
    scissor.extent.height = height;

    vkCmdSetScissor( vs->draw, 0, 1, &scissor );
    }

    VkDeviceSize offsets[1] = {0};
    vkCmdBindVertexBuffers(vs->draw, VERTEX_BUFFER_BIND_ID, 1,
                           &vs->vertices.buf, offsets);

    vkCmdDraw(vs->draw, 3, 1, 0, 0);
    vkCmdEndRenderPass(vs->draw);

    VkImageMemoryBarrier prePresentBarrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .pNext = NULL,
        .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_MEMORY_READ_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}
    };
    prePresentBarrier.image = vs->buffers[ vs->currentBuffer ].image;

    vkCmdPipelineBarrier( vs->draw, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                          VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, NULL, 0,
                          NULL, 1, &prePresentBarrier );

    err = vkEndCommandBuffer(vs->draw);
    assert(!err);
}


void vs_initBuffers( const GLView* view, VulkanState* vs );
void vs_freeBuffers( VulkanState* vs, int freeSwapchain );

void repaint( GLView* view )
{
    static const VkSemaphoreCreateInfo sc = {
        VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, NULL, 0
    };
    VkSemaphore presentComplete;
    VulkanState* vs = (VulkanState*) view->user;
    VkResult err;

    if( vs->doRepaint == DO_REPAINT_IGNORE )
        return;
    if( vs->doRepaint == DO_REPAINT_RESIZE )
    {
        // In order to properly handle window resizing, we must re-create the
        // swapchain AND redo the command buffers, etc.
        if( vs->prepared )
            vs_freeBuffers( vs, 0 );
        vs_initBuffers( view, vs );
    }

    err = vkCreateSemaphore( view->device, &sc, NULL, &presentComplete );
    assert(!err);

    // Get the index of the next available swapchain image:
    err = vs->AcquireNextImage( view->device, vs->swapchain, UINT64_MAX,
                                presentComplete,
                                (VkFence) 0,    // TODO: Show use of fence
                                &vs->currentBuffer );
    if( err == VK_ERROR_OUT_OF_DATE_KHR )
    {
        // vs->swapchain is out of date (e.g. the window was resized) and
        // must be recreated:
resize:
        vs->doRepaint = DO_REPAINT_RESIZE;
        repaint( view );
        vkDestroySemaphore( view->device, presentComplete, NULL );
        return;
    }
    else if (err == VK_SUBOPTIMAL_KHR)
    {
        // vs->swapchain is not as optimal as it could be, but the platform's
        // presentation engine will still present the image correctly.
    }
    else
    {
        assert(!err);
    }

    flushInitCmd( view->queue, vs );

    // Wait for the present complete semaphore to be signaled to ensure
    // that the image won't be rendered to until the presentation
    // engine has fully released ownership to the application, and it is
    // okay to render to the image.

    drawBuildCmd( vs, vs->surfWidth, vs->surfHeight );

    {
    VkPipelineStageFlags pipe_stage_flags =
        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    VkSubmitInfo submit_info;
    VkPresentInfoKHR present;

    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext = NULL;
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = &presentComplete;
    submit_info.pWaitDstStageMask = &pipe_stage_flags;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &vs->draw;
    submit_info.signalSemaphoreCount = 0;
    submit_info.pSignalSemaphores = NULL;

    err = vkQueueSubmit( view->queue, 1, &submit_info, VK_NULL_HANDLE );
    assert(!err);

    present.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present.pNext = NULL;
    present.waitSemaphoreCount = 0;
    present.pWaitSemaphores = NULL;
    present.swapchainCount = 1;
    present.pSwapchains    = &vs->swapchain;
    present.pImageIndices  = &vs->currentBuffer;
    present.pResults = NULL;

    // TBD/TODO: SHOULD THE "present" PARAMETER BE "const" IN THE HEADER?
    err = vs->QueuePresent( view->queue, &present );
    if( err == VK_ERROR_OUT_OF_DATE_KHR )
    {
        // vs->swapchain is out of date (e.g. the window was resized) and
        // must be recreated:
        goto resize;
    }
    else if( err == VK_SUBOPTIMAL_KHR )
    {
        // vs->swapchain is not as optimal as it could be, but the platform's
        // presentation engine will still present the image correctly.
    }
    else
    {
        assert(!err);
    }
    }

    err = vkQueueWaitIdle( view->queue );
    assert(err == VK_SUCCESS);

    vkDestroySemaphore( view->device, presentComplete, NULL );
    vs->doRepaint = DO_REPAINT_IGNORE;
}


void vs_signalResize( VulkanState* vs )
{
    vs->doRepaint = DO_REPAINT_RESIZE;
}


void vs_signalRepaint( VulkanState* vs )
{
    // Don't overwrite DO_REPAINT_RESIZE.
    if( vs->doRepaint == DO_REPAINT_IGNORE )
        vs->doRepaint = DO_REPAINT_DRAW;
}


void eventHandler( GLView* view, GLViewEvent* event )
{
    VulkanState* vs = (VulkanState*) view->user;

    switch( event->type )
    {
        case GLV_EVENT_RESIZE:
            if( view->width == vs->surfWidth &&
                view->height == vs->surfHeight )
            {
                printf( "testResize %d %d (SKIP; surface already updated)\n",
                        event->x, event->y );
                return;
            }
            printf( "testResize %d %d\n", event->x, event->y );
            vs_signalResize( vs );
            break;

        case GLV_EVENT_EXPOSE:
            printf( "testExpose\n" );
            vs_signalRepaint( vs );
            break;

        case GLV_EVENT_MOTION:
            vs_signalRepaint( vs );
            break;

        case GLV_EVENT_KEY_DOWN:
            if( event->code == KEY_Escape )
                vs->quit = 1;
            break;

        case GLV_EVENT_CLOSE:
            printf( "testClose\n" );
            vs->quit = 1;
            break;
    }
}


void printPhysicalDevice( VkPhysicalDevice gpu )
{
    VkPhysicalDeviceProperties dp;
    VkPhysicalDeviceFeatures df;

#define REPORT_PROP(val)   printf("    %s:\t%u\n", #val, dp.limits.val);
#define REPORT_PROP_F(val) printf("    %s:\t%f\n", #val, dp.limits.val);
#define REPORT_FEAT(val)   printf("    %s:\t%s\n", #val, df.val ? "yes" : "no");


    vkGetPhysicalDeviceProperties( gpu, &dp );
    printf( "device: \"%s\"\napi: %d.%d.%d\ndriver: 0x%08X\n",
            dp.deviceName,
            VK_VERSION_MAJOR( dp.apiVersion ),
            VK_VERSION_MINOR( dp.apiVersion ),
            VK_VERSION_PATCH( dp.apiVersion ),
            dp.driverVersion );
    printf( "limits: [\n" );
    REPORT_PROP( maxImageDimension1D );
    REPORT_PROP( maxImageDimension2D );
    REPORT_PROP( maxImageDimension3D );
    REPORT_PROP( maxImageDimensionCube );
    //...
    REPORT_PROP( maxDescriptorSetSamplers );
    //...
    REPORT_PROP( maxViewportDimensions[0] );
    REPORT_PROP( maxViewportDimensions[1] );
    //...
    REPORT_PROP_F( pointSizeRange[0] );
    REPORT_PROP_F( pointSizeRange[1] );
    //...
    printf( "]\n" );


    vkGetPhysicalDeviceFeatures( gpu, &df );
    printf( "features: [\n" );
    //...
    REPORT_FEAT( largePoints );
    REPORT_FEAT( alphaToOne );
    REPORT_FEAT( multiViewport );
    //...
    REPORT_FEAT( shaderClipDistance );
    REPORT_FEAT( shaderCullDistance );
    REPORT_FEAT( shaderFloat64 );
    REPORT_FEAT( shaderInt64 );
    REPORT_FEAT( shaderInt16 );
    //...
    printf( "]\n" );
}


void vs_init( GLView* view, VulkanState* vs )
{
    VkInstance inst = view->inst;
    VkSurfaceFormatKHR* surfFormats;
    VkResult err;
    uint32_t fcount;


    vs->device      = view->device;
    vs->setup       = VK_NULL_HANDLE;
    vs->draw        = VK_NULL_HANDLE;
    vs->swapchain   = VK_NULL_HANDLE;
    vs->currentBuffer = 0;
    vs->surfWidth   =
    vs->surfHeight  = 0;
    vs->useStagingBuffer = 0;
    vs->prepared = 0;
    vs->doRepaint = DO_REPAINT_IGNORE;
    vs->quit = 0;

    vs->depthStencil = 1.0;
    vs->depthIncrement = -0.01f;

#define IP(name)    (PFN_##name) vkGetInstanceProcAddr(inst, #name)

    vs->SurfaceCapabilities = IP( vkGetPhysicalDeviceSurfaceCapabilitiesKHR );
    vs->SurfaceFormats      = IP( vkGetPhysicalDeviceSurfaceFormatsKHR );
    //vs->SurfacePresentModes = IP( vkGetPhysicalDeviceSurfacePresentModesKHR );
    vs->CreateSwapchain     = IP( vkCreateSwapchainKHR );
    vs->DestroySwapchain    = IP( vkDestroySwapchainKHR );
    vs->SwapchainImages     = IP( vkGetSwapchainImagesKHR );
    vs->AcquireNextImage    = IP( vkAcquireNextImageKHR );
    vs->QueuePresent        = IP( vkQueuePresentKHR );


    {
    VkDebugReportCallbackCreateInfoEXT dc;
    PFN_vkCreateDebugReportCallbackEXT createReportFunc;

    createReportFunc        = IP( vkCreateDebugReportCallbackEXT );
    vs->DestroyDebugReport  = IP( vkDestroyDebugReportCallbackEXT );

    dc.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
    dc.pNext = NULL;
    dc.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
#if 0
    //if( verbose )
        dc.flags |= VK_DEBUG_REPORT_INFORMATION_BIT_EXT |
                    VK_DEBUG_REPORT_DEBUG_BIT_EXT;
#endif
    dc.pfnCallback = debugCallback;
    dc.pUserData = NULL;

    err = createReportFunc( view->inst, &dc, NULL, &vs->report );
    assert( ! err );
    }


    // Get the list of VkFormat's that are supported:
    err = vs->SurfaceFormats( view->gpu, view->surface, &fcount, NULL );
    assert( ! err );
    surfFormats = (VkSurfaceFormatKHR*)
                  malloc( fcount * sizeof(VkSurfaceFormatKHR) );
    err = vs->SurfaceFormats( view->gpu, view->surface, &fcount, surfFormats );
    assert( ! err );
    assert( fcount >= 1 );

    // If the format list includes just one entry of VK_FORMAT_UNDEFINED,
    // the surface has no preferred format.  Otherwise, at least one
    // supported format will be returned.
    if( fcount == 1 && surfFormats[0].format == VK_FORMAT_UNDEFINED )
        vs->format = VK_FORMAT_B8G8R8A8_UNORM;
    else
        vs->format = surfFormats[0].format;

    vs->colorSpace = surfFormats[0].colorSpace;

    vkGetPhysicalDeviceMemoryProperties( view->gpu, &vs->memProp );
}


static void texobj_freeImage( VkDevice device, TextureObject* obj )
{
    vkDestroyImage( device, obj->image, NULL );
    vkFreeMemory( device, obj->mem, NULL );
}


void vs_freeBuffers( VulkanState* vs, int freeSwapchain )
{
    VkDevice device = vs->device;
    uint32_t i;

    vs->prepared = 0;
    vkDeviceWaitIdle( device );

    for( i = 0; i < vs->swapchainImageCount; ++i )
        vkDestroyFramebuffer( device, vs->framebuffers[i], NULL );
    free( vs->framebuffers );
    vkDestroyDescriptorPool( device, vs->descPool, NULL );

    if( vs->setup )
        vkFreeCommandBuffers( device, vs->cpool, 1, &vs->setup );
    vkFreeCommandBuffers( device, vs->cpool, 1, &vs->draw );
    vkDestroyCommandPool( device, vs->cpool, NULL );

    vkDestroyPipeline( device, vs->pipeline, NULL );
    vkDestroyRenderPass( device, vs->renderPass, NULL );
    vkDestroyPipelineLayout( device, vs->pipelineLayout, NULL );
    vkDestroyDescriptorSetLayout( device, vs->descLayout, NULL );

    vert_free( device, &vs->vertices );

    for( i = 0; i < DEMO_TEXTURE_COUNT; ++i )
    {
        vkDestroyImageView( device, vs->textures[i].view, NULL );
        texobj_freeImage( device, &vs->textures[i] );
        vkDestroySampler( device, vs->textures[i].sampler, NULL );
    }

    for( i = 0; i < vs->swapchainImageCount; ++i )
        vkDestroyImageView( device, vs->buffers[i].view, NULL );

    vkDestroyImageView( device, vs->depth.view, NULL );
    vkDestroyImage( device, vs->depth.image, NULL );
    vkFreeMemory( device, vs->depth.mem, NULL );

    if( freeSwapchain )
        vs->DestroySwapchain( device, vs->swapchain, NULL );
    free( vs->buffers );
}


void vs_free( VkInstance inst, VulkanState* vs )
{
    if( vs->prepared )
        vs_freeBuffers( vs, 1 );
    vs->DestroyDebugReport( inst, vs->report, NULL );
}


static void vs_setImageLayout( VulkanState* vs, VkImage image,
                               VkImageAspectFlags aspectMask,
                               VkImageLayout oldLayout,
                               VkImageLayout newLayout,
                               VkAccessFlagBits srcAccessMask )
{
    VkResult err;

    if( vs->setup == VK_NULL_HANDLE )
    {
        VkCommandBufferAllocateInfo cmd;
        VkCommandBufferInheritanceInfo ii;
        VkCommandBufferBeginInfo bi;

        cmd.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmd.pNext = NULL;
        cmd.commandPool = vs->cpool;
        cmd.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cmd.commandBufferCount = 1;

        err = vkAllocateCommandBuffers( vs->device, &cmd, &vs->setup );
        assert(!err);

        ii.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
        ii.pNext = NULL;
        ii.renderPass = VK_NULL_HANDLE;
        ii.subpass = 0;
        ii.framebuffer = VK_NULL_HANDLE;
        ii.occlusionQueryEnable = VK_FALSE;
        ii.queryFlags = 0;
        ii.pipelineStatistics = 0;

        bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        bi.pNext = NULL;
        bi.flags = 0;
        bi.pInheritanceInfo = &ii;

        err = vkBeginCommandBuffer( vs->setup, &bi );
        assert(!err);
    }

    {
    VkImageMemoryBarrier mb;
    VkPipelineStageFlags src_stages  = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    VkPipelineStageFlags dest_stages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

    mb.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    mb.pNext = NULL;
    mb.srcAccessMask = srcAccessMask;
    mb.dstAccessMask = 0;
    mb.oldLayout = oldLayout;
    mb.newLayout = newLayout;
    mb.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    mb.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    mb.image = image;
    mb.subresourceRange.aspectMask = aspectMask;
    mb.subresourceRange.baseMipLevel = 0;
    mb.subresourceRange.levelCount = 1;
    mb.subresourceRange.baseArrayLayer = 0;
    mb.subresourceRange.layerCount = 1;

    if (newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        /* Make sure anything that was copying from this image has completed */
        mb.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    }
    if( newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL )
        mb.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
#if 0
    // Validation error: pImageMemBarriers[0].dstAccessMask (0x400) is not
    //                   supported by dstStageMask (0x1).
    if( newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL )
        mb.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    // Validation error: pImageMemBarriers[0].dstAccessMask (0x30) is not
    //                   supported by dstStageMask (0x1).
    if (newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        /* Make sure any Copy or CPU writes to image are flushed */
        mb.dstAccessMask = VK_ACCESS_SHADER_READ_BIT |
                           VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
    }
#endif

    vkCmdPipelineBarrier( vs->setup, src_stages, dest_stages, 0, 0, NULL,
                          0, NULL, 1, &mb );
    }
}


static void setupDepth( VkDevice device, VulkanState* vs )
{
    VkImageCreateInfo ic;
    VkMemoryAllocateInfo ma;
    VkImageViewCreateInfo vc;
    VkMemoryRequirements memReq;
    VkResult err;
    int pass;
    const VkFormat dformat = VK_FORMAT_D16_UNORM;


    ic.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    ic.pNext = NULL;
    ic.flags = 0;
    ic.imageType = VK_IMAGE_TYPE_2D;
    ic.format = dformat;
    ic.extent.width  = vs->surfWidth;
    ic.extent.height = vs->surfHeight;
    ic.extent.depth  = 1;
    ic.mipLevels = 1;
    ic.arrayLayers = 1;
    ic.samples = VK_SAMPLE_COUNT_1_BIT;
    ic.tiling = VK_IMAGE_TILING_OPTIMAL;
    ic.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    ic.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    ic.queueFamilyIndexCount = 0;
    ic.pQueueFamilyIndices = NULL;
    ic.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    ma.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    ma.pNext = NULL;
    ma.allocationSize = 0;
    ma.memoryTypeIndex = 0;

    vc.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    vc.pNext = NULL;
    vc.image = VK_NULL_HANDLE;
    vc.format = dformat;
    vc.components.r = VK_COMPONENT_SWIZZLE_R;
    vc.components.g = VK_COMPONENT_SWIZZLE_G;
    vc.components.b = VK_COMPONENT_SWIZZLE_B;
    vc.components.a = VK_COMPONENT_SWIZZLE_A;
    vc.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    vc.subresourceRange.baseMipLevel = 0;
    vc.subresourceRange.levelCount = 1;
    vc.subresourceRange.baseArrayLayer = 0;
    vc.subresourceRange.layerCount = 1;
    vc.flags = 0;
    vc.viewType = VK_IMAGE_VIEW_TYPE_2D;


    vs->depth.format = dformat;

    /* create image */
    err = vkCreateImage( device, &ic, NULL, &vs->depth.image );
    assert(!err);

    /* get memory requirements for this object */
    vkGetImageMemoryRequirements( device, vs->depth.image, &memReq );

    /* select memory size and type */
    ma.allocationSize = memReq.size;
    pass = memoryType( &vs->memProp, memReq.memoryTypeBits,
                       0, /* No requirements */
                       &ma.memoryTypeIndex );
    assert(pass);

    /* allocate memory */
    err = vkAllocateMemory( device, &ma, NULL, &vs->depth.mem );
    assert(!err);

    /* bind memory */
    err = vkBindImageMemory( device, vs->depth.image, vs->depth.mem, 0 );
    assert(!err);
    vs_setImageLayout( vs, vs->depth.image, VK_IMAGE_ASPECT_DEPTH_BIT,
                       VK_IMAGE_LAYOUT_UNDEFINED,
                       VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                       0 );

    /* create image view */
    vc.image = vs->depth.image;
    err = vkCreateImageView( device, &vc, NULL, &vs->depth.view );
    assert(!err);
}


static void texobj_init( VulkanState* vs,
            const uint32_t* tex_colors,
            TextureObject* tobj, VkImageTiling tiling,
            VkImageUsageFlags usage, VkFlags required_props )
{
    VkImageCreateInfo ic;
    VkMemoryAllocateInfo ma;
    VkMemoryRequirements memReq;
    const VkFormat tex_format = VK_FORMAT_B8G8R8A8_UNORM;
    const int32_t width = 2;
    const int32_t height = 2;
    VkResult err;
    int pass;


    tobj->texWidth = width;
    tobj->texHeight = height;

    ic.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    ic.pNext = NULL;
    ic.flags = 0;
    ic.imageType = VK_IMAGE_TYPE_2D;
    ic.format = tex_format;
    ic.extent.width  = width;
    ic.extent.height = height;
    ic.extent.depth  = 1;
    ic.mipLevels = 1;
    ic.arrayLayers = 1;
    ic.samples = VK_SAMPLE_COUNT_1_BIT;
    ic.tiling = tiling;
    ic.usage = usage;
    ic.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    ic.queueFamilyIndexCount = 0;
    ic.pQueueFamilyIndices = NULL;
    ic.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;

    ma.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    ma.pNext = NULL;
    ma.allocationSize = 0;
    ma.memoryTypeIndex = 0;


    err = vkCreateImage( vs->device, &ic, NULL, &tobj->image );
    assert(!err);

    vkGetImageMemoryRequirements( vs->device, tobj->image, &memReq );

    ma.allocationSize = memReq.size;
    pass = memoryType( &vs->memProp, memReq.memoryTypeBits,
                       required_props, &ma.memoryTypeIndex );
    assert(pass);

    /* allocate memory */
    err = vkAllocateMemory(vs->device, &ma, NULL, &tobj->mem);
    assert(!err);

    /* bind memory */
    err = vkBindImageMemory(vs->device, tobj->image, tobj->mem, 0);
    assert(!err);

    if( required_props & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT )
    {
        VkImageSubresource subres;
        VkSubresourceLayout layout;
        void* data;
        int32_t x, y;

        subres.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        subres.mipLevel = 0;
        subres.arrayLayer = 0;

        vkGetImageSubresourceLayout( vs->device, tobj->image, &subres,
                                     &layout );

        err = vkMapMemory( vs->device, tobj->mem, 0,
                           ma.allocationSize, 0, &data );
        assert(!err);

        for( y = 0; y < height; y++ )
        {
            uint32_t* row = (uint32_t*) ((char*) data + layout.rowPitch * y);
            for( x = 0; x < width; x++ )
                row[x] = tex_colors[(x & 1) ^ (y & 1)];
        }

        vkUnmapMemory(vs->device, tobj->mem);
    }

    tobj->imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    vs_setImageLayout( vs, tobj->image, VK_IMAGE_ASPECT_COLOR_BIT,
                       VK_IMAGE_LAYOUT_PREINITIALIZED, tobj->imageLayout,
                       0 /*VK_ACCESS_HOST_WRITE_BIT*/ );
    /* setting the image layout does not reference the actual memory so no need
     * to add a mem ref */
}


static void setupTextures( const GLView* view, VulkanState* vs )
{
    VkSamplerCreateInfo sc;
    VkFormatProperties props;
    VkImageViewCreateInfo vc;
    VkImageCopy ic;
    const uint32_t tex_colors[DEMO_TEXTURE_COUNT][2] = {
        {0xffff0000, 0xff00ff00},
    };
    const VkFormat tex_format = VK_FORMAT_B8G8R8A8_UNORM;
    VkResult err;
    uint32_t i;


    vkGetPhysicalDeviceFormatProperties( view->gpu, tex_format, &props );

    for( i = 0; i < DEMO_TEXTURE_COUNT; ++i )
    {
        if( (props.linearTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT)
            && ! vs->useStagingBuffer )
        {
            /* Device can texture using linear textures */
            texobj_init( vs, tex_colors[i], &vs->textures[i],
                         VK_IMAGE_TILING_LINEAR,
                         VK_IMAGE_USAGE_SAMPLED_BIT,
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );
        }
        else if( props.optimalTilingFeatures &
                 VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT )
        {
            /* Must use staging buffer to copy linear texture to optimized */
            TextureObject staging;

            memset( &staging, 0, sizeof(staging) );
            texobj_init( vs, tex_colors[i], &staging,
                         VK_IMAGE_TILING_LINEAR,
                         VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );

            texobj_init( vs, tex_colors[i], &vs->textures[i],
                VK_IMAGE_TILING_OPTIMAL,
                (VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT),
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );

            vs_setImageLayout( vs, staging.image,
                               VK_IMAGE_ASPECT_COLOR_BIT,
                               staging.imageLayout,
                               VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                               0 );

            vs_setImageLayout( vs, vs->textures[i].image,
                               VK_IMAGE_ASPECT_COLOR_BIT,
                               vs->textures[i].imageLayout,
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                               0 );

            memset( &ic, 0, sizeof(ic) );
            ic.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            ic.srcSubresource.layerCount = 1;
            ic.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            ic.dstSubresource.layerCount = 1;
            ic.extent.width  = staging.texWidth;
            ic.extent.height = staging.texHeight;
            ic.extent.depth  = 1;

            vkCmdCopyImage( vs->setup, staging.image,
                VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, vs->textures[i].image,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &ic );

            vs_setImageLayout( vs, vs->textures[i].image,
                               VK_IMAGE_ASPECT_COLOR_BIT,
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                               vs->textures[i].imageLayout,
                               0 );

            flushInitCmd( view->queue, vs );

            texobj_freeImage( vs->device, &staging );
        }
        else
        {
            /* Can't support VK_FORMAT_B8G8R8A8_UNORM !? */
            assert(!"No support for B8G8R8A8_UNORM as texture image format");
        }

        sc.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        sc.pNext = NULL;
        sc.flags = 0;
        sc.magFilter = VK_FILTER_NEAREST;
        sc.minFilter = VK_FILTER_NEAREST;
        sc.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
        sc.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        sc.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        sc.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        sc.mipLodBias = 0.0f;
        sc.anisotropyEnable = VK_FALSE;
        sc.maxAnisotropy = 1;
        sc.compareEnable = VK_FALSE;
        sc.compareOp = VK_COMPARE_OP_NEVER;
        sc.minLod = 0.0f;
        sc.maxLod = 0.0f;
        sc.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
        sc.unnormalizedCoordinates = VK_FALSE;

        vc.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        vc.pNext = NULL;
        vc.image = VK_NULL_HANDLE;
        vc.viewType = VK_IMAGE_VIEW_TYPE_2D;
        vc.format = tex_format;
        vc.components.r = VK_COMPONENT_SWIZZLE_R;
        vc.components.g = VK_COMPONENT_SWIZZLE_G;
        vc.components.b = VK_COMPONENT_SWIZZLE_B;
        vc.components.a = VK_COMPONENT_SWIZZLE_A;
        vc.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        vc.subresourceRange.baseMipLevel   = 0;
        vc.subresourceRange.levelCount     = 1;
        vc.subresourceRange.baseArrayLayer = 0;
        vc.subresourceRange.layerCount     = 1;
        vc.flags = 0;

        /* create sampler */
        err = vkCreateSampler( view->device, &sc, NULL,
                               &vs->textures[i].sampler );
        assert(!err);

        /* create image view */
        vc.image = vs->textures[i].image;
        err = vkCreateImageView( view->device, &vc, NULL,
                                 &vs->textures[i].view );
        assert(!err);
    }
}


static void setupDescLayout( VulkanState* vs )
{
    static const VkDescriptorSetLayoutBinding layout_binding = {
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = DEMO_TEXTURE_COUNT,
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        .pImmutableSamplers = NULL,
    };
    static const VkDescriptorSetLayoutCreateInfo lo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = NULL,
        .bindingCount = 1,
        .pBindings = &layout_binding,
    };
    VkPipelineLayoutCreateInfo plc;
    VkResult err;


    err = vkCreateDescriptorSetLayout( vs->device, &lo, NULL, &vs->descLayout );
    assert(!err);

    plc.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    plc.pNext = NULL;
    plc.flags = 0;
    plc.setLayoutCount = 1;
    plc.pSetLayouts = &vs->descLayout;
    plc.pushConstantRangeCount = 0;
    plc.pPushConstantRanges = NULL;

    err = vkCreatePipelineLayout( vs->device, &plc, NULL, &vs->pipelineLayout );
    assert(!err);
}


static void setupRenderPass( VulkanState* vs )
{
    const VkAttachmentDescription attachments[2] = {
        [0] = {
             .format = vs->format,
             .samples = VK_SAMPLE_COUNT_1_BIT,
             .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
             .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
             .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
             .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
             .initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
             .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        },
        [1] = {
             .format = vs->depth.format,
             .samples = VK_SAMPLE_COUNT_1_BIT,
             .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
             .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
             .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
             .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
             .initialLayout =
                 VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
             .finalLayout =
                 VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        },
    };
    const VkAttachmentReference color_reference = {
        .attachment = 0, .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };
    const VkAttachmentReference depth_reference = {
        .attachment = 1,
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    };
    const VkSubpassDescription subpass = {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .flags = 0,
        .inputAttachmentCount = 0,
        .pInputAttachments = NULL,
        .colorAttachmentCount = 1,
        .pColorAttachments = &color_reference,
        .pResolveAttachments = NULL,
        .pDepthStencilAttachment = &depth_reference,
        .preserveAttachmentCount = 0,
        .pPreserveAttachments = NULL,
    };
    const VkRenderPassCreateInfo rp_info = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .pNext = NULL,
        .attachmentCount = 2,
        .pAttachments = attachments,
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 0,
        .pDependencies = NULL,
    };
    VkResult err;

    err = vkCreateRenderPass( vs->device, &rp_info, NULL, &vs->renderPass );
    assert(!err);
}


VkShaderModule createShader( VkDevice device, const void* code, size_t size )
{
    VkShaderModuleCreateInfo sc;
    VkShaderModule module;
    VkResult err;

    sc.sType     = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    sc.pNext     = NULL;
    sc.codeSize = size;
    sc.pCode     = code;
    sc.flags     = 0;

    err = vkCreateShaderModule( device, &sc, NULL, &module );
    assert(!err);
    return module;
}


VkShaderModule loadShader( VkDevice device, const char* file )
{
    VkShaderModule m = VK_NULL_HANDLE;
    FILE* fp = fopen( file, "rb" );
    if( fp )
    {
        char* buf = malloc( 2048 );
        if( buf )
        {
            size_t size = fread( buf, 1, 2048, fp );
            if( size > 0 )
                m = createShader( device, buf, size );
            free( buf );
        }
        fclose( fp );
    }
    return m;
}


static void setupPipeline( VulkanState* vs )
{
    VkGraphicsPipelineCreateInfo pipeline;
    VkPipelineCacheCreateInfo pipelineCache;
    VkPipelineVertexInputStateCreateInfo vi;
    VkPipelineInputAssemblyStateCreateInfo ia;
    VkPipelineRasterizationStateCreateInfo rs;
    VkPipelineColorBlendStateCreateInfo cb;
    VkPipelineDepthStencilStateCreateInfo ds;
    VkPipelineViewportStateCreateInfo vp;
    VkPipelineMultisampleStateCreateInfo ms;
    VkDynamicState dynamicStateEnables[ VK_DYNAMIC_STATE_RANGE_SIZE ];
    VkPipelineDynamicStateCreateInfo dynamicState;
    VkShaderModule shadV;
    VkShaderModule shadF;
    VkResult err;


    memset(dynamicStateEnables, 0, sizeof dynamicStateEnables);
    memset(&dynamicState, 0, sizeof dynamicState);
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.pDynamicStates = dynamicStateEnables;

    vi = vs->vertices.vi;

    memset(&ia, 0, sizeof(ia));
    ia.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    memset(&rs, 0, sizeof(rs));
    rs.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rs.polygonMode = VK_POLYGON_MODE_FILL;
    rs.cullMode = VK_CULL_MODE_BACK_BIT;
    rs.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rs.depthClampEnable = VK_FALSE;
    rs.rasterizerDiscardEnable = VK_FALSE;
    rs.depthBiasEnable = VK_FALSE;
    rs.lineWidth = 1.0f;

    memset(&cb, 0, sizeof(cb));
    cb.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    VkPipelineColorBlendAttachmentState att_state[1];
    memset(att_state, 0, sizeof(att_state));
    att_state[0].colorWriteMask = 0xf;
    att_state[0].blendEnable = VK_FALSE;
    cb.attachmentCount = 1;
    cb.pAttachments = att_state;

    memset(&vp, 0, sizeof(vp));
    vp.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    vp.viewportCount = 1;
    dynamicStateEnables[dynamicState.dynamicStateCount++] =
        VK_DYNAMIC_STATE_VIEWPORT;
    vp.scissorCount = 1;
    dynamicStateEnables[dynamicState.dynamicStateCount++] =
        VK_DYNAMIC_STATE_SCISSOR;
    memset(&ds, 0, sizeof(ds));
    ds.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    ds.depthTestEnable = VK_TRUE;
    ds.depthWriteEnable = VK_TRUE;
    ds.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    ds.depthBoundsTestEnable = VK_FALSE;
    ds.back.failOp = VK_STENCIL_OP_KEEP;
    ds.back.passOp = VK_STENCIL_OP_KEEP;
    ds.back.compareOp = VK_COMPARE_OP_ALWAYS;
    ds.stencilTestEnable = VK_FALSE;
    ds.front = ds.back;

    memset(&ms, 0, sizeof(ms));
    ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    ms.pSampleMask = NULL;
    ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    // Two stages: vertex and fragment.
    VkPipelineShaderStageCreateInfo shaderStages[2];
    memset(&shaderStages, 0, 2 * sizeof(VkPipelineShaderStageCreateInfo));


#if 1
    shadV = loadShader( vs->device, "vk_shader.vert.spv" );
    shadF = loadShader( vs->device, "vk_shader.frag.spv" );
#else
    shadV = createShader( vs->device, vert_shader, strlen(vert_shader) );
    shadF = createShader( vs->device, frag_shader, strlen(frag_shader) );
#endif

    shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shaderStages[0].module = shadV;
    shaderStages[0].pName = "main";

    shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaderStages[1].module = shadF;
    shaderStages[1].pName = "main";

    memset( &pipeline, 0, sizeof(pipeline) );
    pipeline.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline.stageCount = 2;
    pipeline.pStages = shaderStages;
    pipeline.pVertexInputState   = &vi;
    pipeline.pInputAssemblyState = &ia;
    pipeline.pRasterizationState = &rs;
    pipeline.pColorBlendState    = &cb;
    pipeline.pMultisampleState   = &ms;
    pipeline.pViewportState      = &vp;
    pipeline.pDepthStencilState  = &ds;
    pipeline.pDynamicState       = &dynamicState;
    pipeline.layout     = vs->pipelineLayout;
    pipeline.renderPass = vs->renderPass;

    memset( &pipelineCache, 0, sizeof(pipelineCache) );
    pipelineCache.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;

    err = vkCreatePipelineCache( vs->device, &pipelineCache, NULL,
                                 &vs->pipelineCache );
    assert(!err);
    err = vkCreateGraphicsPipelines( vs->device, vs->pipelineCache, 1,
                                     &pipeline, NULL, &vs->pipeline );
    if( err )
        fprintf( stderr, "vkCreateGraphicsPipelines: %d\n", err );
    assert(!err);

    vkDestroyPipelineCache( vs->device, vs->pipelineCache, NULL );
    vkDestroyShaderModule( vs->device, shadV, NULL );
    vkDestroyShaderModule( vs->device, shadF, NULL );
}


static void setupDescriptorPool( VulkanState* vs )
{
    const VkDescriptorPoolSize type_count = {
        .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = DEMO_TEXTURE_COUNT,
    };
    const VkDescriptorPoolCreateInfo descriptor_pool = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext = NULL,
        .maxSets = 1,
        .poolSizeCount = 1,
        .pPoolSizes = &type_count,
    };
    VkResult err;

    err = vkCreateDescriptorPool( vs->device, &descriptor_pool, NULL,
                                  &vs->descPool );
    assert(!err);
}


static void setupDescriptorSet( VulkanState* vs )
{
    VkDescriptorImageInfo tex_descs[DEMO_TEXTURE_COUNT];
    VkWriteDescriptorSet write;
    VkDescriptorSetAllocateInfo ai;
    VkResult err;
    uint32_t i;

    ai.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    ai.pNext = NULL;
    ai.descriptorPool = vs->descPool;
    ai.descriptorSetCount = 1;
    ai.pSetLayouts = &vs->descLayout;

    err = vkAllocateDescriptorSets( vs->device, &ai, &vs->descSet );
    assert(!err);

    memset(&tex_descs, 0, sizeof(tex_descs));
    for( i = 0; i < DEMO_TEXTURE_COUNT; ++i )
    {
        tex_descs[i].sampler     = vs->textures[i].sampler;
        tex_descs[i].imageView   = vs->textures[i].view;
        tex_descs[i].imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    }

    memset(&write, 0, sizeof(write));
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = vs->descSet;
    write.descriptorCount = DEMO_TEXTURE_COUNT;
    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.pImageInfo = tex_descs;

    vkUpdateDescriptorSets( vs->device, 1, &write, 0, NULL );
}


static void setupFramebuffers( VulkanState* vs, uint32_t width, uint32_t height)
{
    VkImageView attachments[2];
    attachments[1] = vs->depth.view;
    VkFramebufferCreateInfo fc;
    VkResult err;
    uint32_t i;

    fc.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fc.pNext = NULL;
    fc.flags = 0;
    fc.renderPass = vs->renderPass;
    fc.attachmentCount = 2;
    fc.pAttachments = attachments;
    fc.width  = width;
    fc.height = height;
    fc.layers = 1;

    vs->framebuffers = (VkFramebuffer*) malloc( vs->swapchainImageCount *
                                                sizeof(VkFramebuffer) );
    assert(vs->framebuffers);

    for( i = 0; i < vs->swapchainImageCount; ++i )
    {
        attachments[0] = vs->buffers[i].view;
        err = vkCreateFramebuffer(vs->device, &fc, NULL, &vs->framebuffers[i]);
        assert(!err);
    }
}


void vs_initBuffers( const GLView* view, VulkanState* vs )
{
    VkResult err;
    VkCommandPoolCreateInfo pc;
    VkCommandBufferAllocateInfo ba;
#define FPV    5
    static const float vb[3 * FPV] =
    {   /*      position             texcoord */
        -1.0f, -1.0f,  0.25f,     0.0f, 0.0f,
         1.0f, -1.0f,  0.25f,     1.0f, 0.0f,
         0.0f,  1.0f,  1.0f,      0.5f, 1.0f,
    };


    printf( "vs_initBuffers %d,%d\n", view->width, view->height );

    pc.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
    pc.pNext = NULL,
    pc.queueFamilyIndex = view->queueFamily,
    pc.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,

    err = vkCreateCommandPool( view->device, &pc, NULL, &vs->cpool );
    assert( ! err );


    ba.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
    ba.pNext = NULL,
    ba.commandPool = vs->cpool,
    ba.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
    ba.commandBufferCount = 1,

    err = vkAllocateCommandBuffers( view->device, &ba, &vs->draw );
    assert( ! err );


    setupBuffers( view, vs );
    setupDepth( view->device, vs );
    setupTextures( view, vs );
    vert_init( vs, &vs->vertices, vb, sizeof(vb), sizeof(float) * FPV );
    setupDescLayout( vs );
    setupRenderPass( vs );
    setupPipeline( vs );

    setupDescriptorPool( vs );
    setupDescriptorSet( vs );

    setupFramebuffers( vs, vs->surfWidth, vs->surfHeight );
    vs->prepared = 1;
}


int main( int argc, char** argv )
{
    GLView* view;
    GLViewMode mode;
    VulkanState vs;
    (void) argc;
    (void) argv;

    view = glv_create( GLV_ATTRIB_DEBUG );
    if( view )
    {
        view->user = &vs;

        printPhysicalDevice( view->gpu );
        vs_init( view, &vs );

        glv_setTitle( view, "GLView Vulkan Example" );
        glv_setEventHandler( view, eventHandler );

        mode.id     = GLV_MODEID_WINDOW;
        mode.width  = 640;
        mode.height = 480;

        glv_changeMode( view, &mode );

        while( ! vs.quit )
        {
            glv_waitEvent( view );
            glv_handleEvents( view );
            repaint( view );

            if( vs.depthStencil > 0.99f )
                vs.depthIncrement = -0.001f;
            if( vs.depthStencil < 0.8f )
                vs.depthIncrement = 0.001f;
            vs.depthStencil += vs.depthIncrement;
            //vkDeviceWaitIdle( view->device );
        }

        vs_free( view->inst, &vs );
        glv_destroy( view );
    }

    return( 0 );
}


/*EOF*/
