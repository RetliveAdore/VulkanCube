#include "vkutilities.h"

static char *tex_files[] = {"lunarg.ppm"};

#define XYZ1(_x_, _y_, _z_)         (_x_), (_y_), (_z_), 1.f
#define UV(_u_, _v_)                (_u_), (_v_), 0.f, 1.f

static const float g_vertex_buffer_data[] = {
    -1.0f,-1.0f,-1.0f,  // -X side
    -1.0f,-1.0f, 1.0f,
    -1.0f, 1.0f, 1.0f,
    -1.0f, 1.0f, 1.0f,
    -1.0f, 1.0f,-1.0f,
    -1.0f,-1.0f,-1.0f,

    -1.0f,-1.0f,-1.0f,  // -Z side
     1.0f, 1.0f,-1.0f,
     1.0f,-1.0f,-1.0f,
    -1.0f,-1.0f,-1.0f,
    -1.0f, 1.0f,-1.0f,
     1.0f, 1.0f,-1.0f,

    -1.0f,-1.0f,-1.0f,  // -Y side
     1.0f,-1.0f,-1.0f,
     1.0f,-1.0f, 1.0f,
    -1.0f,-1.0f,-1.0f,
     1.0f,-1.0f, 1.0f,
    -1.0f,-1.0f, 1.0f,

    -1.0f, 1.0f,-1.0f,  // +Y side
    -1.0f, 1.0f, 1.0f,
     1.0f, 1.0f, 1.0f,
    -1.0f, 1.0f,-1.0f,
     1.0f, 1.0f, 1.0f,
     1.0f, 1.0f,-1.0f,

     1.0f, 1.0f,-1.0f,  // +X side
     1.0f, 1.0f, 1.0f,
     1.0f,-1.0f, 1.0f,
     1.0f,-1.0f, 1.0f,
     1.0f,-1.0f,-1.0f,
     1.0f, 1.0f,-1.0f,

    -1.0f, 1.0f, 1.0f,  // +Z side
    -1.0f,-1.0f, 1.0f,
     1.0f, 1.0f, 1.0f,
    -1.0f,-1.0f, 1.0f,
     1.0f,-1.0f, 1.0f,
     1.0f, 1.0f, 1.0f,
};

static const float g_uv_buffer_data[] = {
    0.0f, 0.0f,  // -X side
    1.0f, 0.0f,
    1.0f, 1.0f,
    1.0f, 1.0f,
    0.0f, 1.0f,
    0.0f, 0.0f,

    1.0f, 0.0f,  // -Z side
    0.0f, 1.0f,
    0.0f, 0.0f,
    1.0f, 0.0f,
    1.0f, 1.0f,
    0.0f, 1.0f,

    1.0f, 1.0f,  // -Y side
    1.0f, 0.0f,
    0.0f, 0.0f,
    1.0f, 1.0f,
    0.0f, 0.0f,
    0.0f, 1.0f,

    1.0f, 1.0f,  // +Y side
    0.0f, 1.0f,
    0.0f, 0.0f,
    1.0f, 1.0f,
    0.0f, 0.0f,
    1.0f, 0.0f,

    1.0f, 1.0f,  // +X side
    0.0f, 1.0f,
    0.0f, 0.0f,
    0.0f, 0.0f,
    1.0f, 0.0f,
    1.0f, 1.0f,

    0.0f, 1.0f,  // +Z side
    0.0f, 0.0f,
    1.0f, 1.0f,
    0.0f, 0.0f,
    1.0f, 0.0f,
    1.0f, 1.0f,
};
// clang-format on

void dumpMatrix(const char *note, mat4x4 MVP) {
    int i;

    printf("%s: \n", note);
    for (i = 0; i < 4; i++) {
        printf("%f, %f, %f, %f\n", MVP[i][0], MVP[i][1], MVP[i][2], MVP[i][3]);
    }
    printf("\n");
    fflush(stdout);
}

void dumpVec4(const char *note, vec4 vector) {
    printf("%s: \n", note);
    printf("%f, %f, %f, %f\n", vector[0], vector[1], vector[2], vector[3]);
    printf("\n");
    fflush(stdout);
}

VkBool32 BreakCallback(VkFlags msgFlags, VkDebugReportObjectTypeEXT objType,
                       uint64_t srcObject, size_t location, int32_t msgCode,
                       const char *pLayerPrefix, const char *pMsg,
                       void *pUserData) {
    DebugBreak();
    return false;
}

// Forward declaration:
static void demo_resize(struct cr_vk_inner *cr_vk_inner);

static void demo_flush_init_cmd(struct cr_vk_inner *cr_vk_inner) {
    VkResult U_ASSERT_ONLY err;

    if (cr_vk_inner->cmd == VK_NULL_HANDLE)
        return;

    err = vkEndCommandBuffer(cr_vk_inner->cmd);
    assert(!err);

    const VkCommandBuffer cmd_bufs[] = {cr_vk_inner->cmd};
    VkFence nullFence = VK_NULL_HANDLE;
    VkSubmitInfo submit_info = {.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                                .pNext = NULL,
                                .waitSemaphoreCount = 0,
                                .pWaitSemaphores = NULL,
                                .pWaitDstStageMask = NULL,
                                .commandBufferCount = 1,
                                .pCommandBuffers = cmd_bufs,
                                .signalSemaphoreCount = 0,
                                .pSignalSemaphores = NULL};

    err = vkQueueSubmit(cr_vk_inner->queue, 1, &submit_info, nullFence);
    assert(!err);

    err = vkQueueWaitIdle(cr_vk_inner->queue);
    assert(!err);

    vkFreeCommandBuffers(cr_vk_inner->device, cr_vk_inner->cmd_pool, 1, cmd_bufs);
    cr_vk_inner->cmd = VK_NULL_HANDLE;
}

static void demo_draw_build_cmd(struct cr_vk_inner *cr_vk_inner, VkCommandBuffer cmd_buf) {
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
    const VkCommandBufferBeginInfo cmd_buf_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = NULL,
        .flags = 0,
        .pInheritanceInfo = &cmd_buf_hinfo,
    };
    const VkClearValue clear_values[2] = {
            [0] = {.color.float32 = {0.2f, 0.2f, 0.2f, 0.2f}},
            [1] = {.depthStencil = {1.0f, 0}},
    };
    const VkRenderPassBeginInfo rp_begin = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .pNext = NULL,
        .renderPass = cr_vk_inner->render_pass,
        .framebuffer = cr_vk_inner->framebuffers[cr_vk_inner->current_buffer],
        .renderArea.offset.x = 0,
        .renderArea.offset.y = 0,
        .renderArea.extent.width = cr_vk_inner->width,
        .renderArea.extent.height = cr_vk_inner->height,
        .clearValueCount = 2,
        .pClearValues = clear_values,
    };
    VkResult U_ASSERT_ONLY err;

    err = vkBeginCommandBuffer(cmd_buf, &cmd_buf_info);
    assert(!err);

    vkCmdBeginRenderPass(cmd_buf, &rp_begin, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, cr_vk_inner->pipeline);
    vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            cr_vk_inner->pipeline_layout, 0, 1, &cr_vk_inner->desc_set, 0,
                            NULL);

    VkViewport viewport;
    memset(&viewport, 0, sizeof(viewport));
    viewport.height = (float)cr_vk_inner->height;
    viewport.width = (float)cr_vk_inner->width;
    viewport.minDepth = (float)0.0f;
    viewport.maxDepth = (float)1.0f;
    vkCmdSetViewport(cmd_buf, 0, 1, &viewport);

    VkRect2D scissor;
    memset(&scissor, 0, sizeof(scissor));
    scissor.extent.width = cr_vk_inner->width;
    scissor.extent.height = cr_vk_inner->height;
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    vkCmdSetScissor(cmd_buf, 0, 1, &scissor);

    vkCmdDraw(cmd_buf, 12 * 3, 1, 0, 0);
    vkCmdEndRenderPass(cmd_buf);

    VkImageMemoryBarrier prePresentBarrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .pNext = NULL,
        .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_MEMORY_READ_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}};

    prePresentBarrier.image = cr_vk_inner->buffers[cr_vk_inner->current_buffer].image;
    VkImageMemoryBarrier *pmemory_barrier = &prePresentBarrier;
    vkCmdPipelineBarrier(cmd_buf, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                         VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, NULL, 0,
                         NULL, 1, pmemory_barrier);

    err = vkEndCommandBuffer(cmd_buf);
    assert(!err);
}

void demo_update_data_buffer(struct cr_vk_inner *cr_vk_inner) {
    mat4x4 MVP, Model, VP;
    int matrixSize = sizeof(MVP);
    uint8_t *pData;
    VkResult U_ASSERT_ONLY err;

    mat4x4_mul(VP, cr_vk_inner->projection_matrix, cr_vk_inner->view_matrix);

    // Rotate 22.5 degrees around the Y axis
    mat4x4_dup(Model, cr_vk_inner->model_matrix);
    mat4x4_rotate(cr_vk_inner->model_matrix, Model, 0.0f, 1.0f, 0.0f,
                  (float)degreesToRadians(cr_vk_inner->spin_angle));
    mat4x4_mul(MVP, VP, cr_vk_inner->model_matrix);

    err = vkMapMemory(cr_vk_inner->device, cr_vk_inner->uniform_data.mem, 0,
                      cr_vk_inner->uniform_data.mem_alloc.allocationSize, 0,
                      (void **)&pData);
    assert(!err);

    memcpy(pData, (const void *)&MVP[0][0], matrixSize);
    //取消内存映射
    vkUnmapMemory(cr_vk_inner->device, cr_vk_inner->uniform_data.mem);
}

static void demo_draw(struct cr_vk_inner *cr_vk_inner) {
    VkResult U_ASSERT_ONLY err;
    VkSemaphore presentCompleteSemaphore;
    VkSemaphoreCreateInfo presentCompleteSemaphoreCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
    };
    VkFence nullFence = VK_NULL_HANDLE;

    err = vkCreateSemaphore(cr_vk_inner->device, &presentCompleteSemaphoreCreateInfo,
                            NULL, &presentCompleteSemaphore);
    assert(!err);

    // Get the index of the next available swapchain image:
    err = cr_vk_inner->fpAcquireNextImageKHR(cr_vk_inner->device, cr_vk_inner->swapchain, UINT64_MAX,
                                      presentCompleteSemaphore,
                                      (VkFence)0, // TODO: Show use of fence
                                      &cr_vk_inner->current_buffer);
    if (err == VK_ERROR_OUT_OF_DATE_KHR) {
        // cr_vk_inner->swapchain is out of date (e.g. the window was resized) and
        // must be recreated:
        demo_resize(cr_vk_inner);
        demo_draw(cr_vk_inner);
        vkDestroySemaphore(cr_vk_inner->device, presentCompleteSemaphore, NULL);
        return;
    } else if (err == VK_SUBOPTIMAL_KHR) {
        // cr_vk_inner->swapchain is not as optimal as it could be, but the platform's
        // presentation engine will still present the image correctly.
    } else {
        assert(!err);
    }

    // Assume the command buffer has been run on current_buffer before so
    // we need to set the image layout back to COLOR_ATTACHMENT_OPTIMAL
    demo_set_image_layout(cr_vk_inner, cr_vk_inner->buffers[cr_vk_inner->current_buffer].image,
                          VK_IMAGE_ASPECT_COLOR_BIT,
                          VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                          VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    demo_flush_init_cmd(cr_vk_inner);

    // Wait for the present complete semaphore to be signaled to ensure
    // that the image won't be rendered to until the presentation
    // engine has fully released ownership to the application, and it is
    // okay to render to the image.

    // FIXME/TODO: DEAL WITH VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    VkPipelineStageFlags pipe_stage_flags =
        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    VkSubmitInfo submit_info = {.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                                .pNext = NULL,
                                .waitSemaphoreCount = 1,
                                .pWaitSemaphores = &presentCompleteSemaphore,
                                .pWaitDstStageMask = &pipe_stage_flags,
                                .commandBufferCount = 1,
                                .pCommandBuffers =
                                    &cr_vk_inner->buffers[cr_vk_inner->current_buffer].cmd,
                                .signalSemaphoreCount = 0,
                                .pSignalSemaphores = NULL};

    err = vkQueueSubmit(cr_vk_inner->queue, 1, &submit_info, nullFence);
    assert(!err);

    VkPresentInfoKHR present = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .pNext = NULL,
        .swapchainCount = 1,
        .pSwapchains = &cr_vk_inner->swapchain,
        .pImageIndices = &cr_vk_inner->current_buffer,
    };

    // TBD/TODO: SHOULD THE "present" PARAMETER BE "const" IN THE HEADER?
    err = cr_vk_inner->fpQueuePresentKHR(cr_vk_inner->queue, &present);
    if (err == VK_ERROR_OUT_OF_DATE_KHR) {
        // cr_vk_inner->swapchain is out of date (e.g. the window was resized) and
        // must be recreated:
        demo_resize(cr_vk_inner);
    } else if (err == VK_SUBOPTIMAL_KHR) {
        // cr_vk_inner->swapchain is not as optimal as it could be, but the platform's
        // presentation engine will still present the image correctly.
    } else {
        assert(!err);
    }

    err = vkQueueWaitIdle(cr_vk_inner->queue);
    assert(err == VK_SUCCESS);

    vkDestroySemaphore(cr_vk_inner->device, presentCompleteSemaphore, NULL);
}

/* Load a ppm file into memory */
bool loadTexture(
    const char *filename,
    uint8_t *rgba_data,
    VkSubresourceLayout *layout,
    int32_t *width,
    int32_t *height
)
{
    FILE *fPtr = fopen(filename, "rb");
    char header[256], *cPtr, *tmp;

    if (!fPtr)
        return false;

    cPtr = fgets(header, 256, fPtr); // P6
    if (cPtr == NULL || strncmp(header, "P6\n", 3)) {
        fclose(fPtr);
        return false;
    }

    do {
        cPtr = fgets(header, 256, fPtr);
        if (cPtr == NULL) {
            fclose(fPtr);
            return false;
        }
    } while (!strncmp(header, "#", 1));

    sscanf(header, "%u %u", height, width);
    if (rgba_data == NULL) {
        fclose(fPtr);
        return true;
    }
    tmp = fgets(header, 256, fPtr); // Format
    (void)tmp;
    if (cPtr == NULL || strncmp(header, "255\n", 3)) {
        fclose(fPtr);
        return false;
    }

    for (int y = 0; y < *height; y++) {
        uint8_t *rowPtr = rgba_data;
        for (int x = 0; x < *width; x++) {
            size_t s = fread(rowPtr, 3, 1, fPtr);
            (void)s;
            rowPtr[3] = 255; /* Alpha of 1 */
            rowPtr += 4;
        }
        rgba_data += layout->rowPitch;
    }
    fclose(fPtr);
    return true;
}

static void demo_destroy_texture_image(struct cr_vk_inner *cr_vk_inner,
                                       struct texture_object *tex_objs) {
    /* clean up staging resources */
    vkFreeMemory(cr_vk_inner->device, tex_objs->mem, NULL);
    vkDestroyImage(cr_vk_inner->device, tex_objs->image, NULL);
}

static void demo_prepare_textures(struct cr_vk_inner *cr_vk_inner) {
    const VkFormat tex_format = VK_FORMAT_R8G8B8A8_UNORM;
    VkFormatProperties props;
    uint32_t i;

    vkGetPhysicalDeviceFormatProperties(cr_vk_inner->gpu, tex_format, &props);

    for (i = 0; i < DEMO_TEXTURE_COUNT; i++) {
        VkResult U_ASSERT_ONLY err;

        if ((props.linearTilingFeatures &
             VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) &&
            !cr_vk_inner->use_staging_buffer) {
            /* Device can texture using linear textures */
            demo_prepare_texture_image(cr_vk_inner, tex_files[i], &cr_vk_inner->textures[i],
                                       VK_IMAGE_TILING_LINEAR,
                                       VK_IMAGE_USAGE_SAMPLED_BIT,
                                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
        } else if (props.optimalTilingFeatures &
                   VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) {
            /* Must use staging buffer to copy linear texture to optimized */
            struct texture_object staging_texture;

            memset(&staging_texture, 0, sizeof(staging_texture));
            demo_prepare_texture_image(cr_vk_inner, tex_files[i], &staging_texture,
                                       VK_IMAGE_TILING_LINEAR,
                                       VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

            demo_prepare_texture_image(
                cr_vk_inner, tex_files[i], &cr_vk_inner->textures[i], VK_IMAGE_TILING_OPTIMAL,
                (VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT),
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

            demo_set_image_layout(cr_vk_inner, staging_texture.image,
                                  VK_IMAGE_ASPECT_COLOR_BIT,
                                  staging_texture.imageLayout,
                                  VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

            demo_set_image_layout(cr_vk_inner, cr_vk_inner->textures[i].image,
                                  VK_IMAGE_ASPECT_COLOR_BIT,
                                  cr_vk_inner->textures[i].imageLayout,
                                  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

            VkImageCopy copy_region = {
                .srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1},
                .srcOffset = {0, 0, 0},
                .dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1},
                .dstOffset = {0, 0, 0},
                .extent = {staging_texture.tex_width,
                           staging_texture.tex_height, 1},
            };
            vkCmdCopyImage(
                cr_vk_inner->cmd, staging_texture.image,
                VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, cr_vk_inner->textures[i].image,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy_region);

            demo_set_image_layout(cr_vk_inner, cr_vk_inner->textures[i].image,
                                  VK_IMAGE_ASPECT_COLOR_BIT,
                                  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                  cr_vk_inner->textures[i].imageLayout);

            demo_flush_init_cmd(cr_vk_inner);

            demo_destroy_texture_image(cr_vk_inner, &staging_texture);
        } else {
            /* Can't support VK_FORMAT_R8G8B8A8_UNORM !? */
            assert(!"No support for R8G8B8A8_UNORM as texture image format");
        }

        const VkSamplerCreateInfo sampler = {
            .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .pNext = NULL,
            .magFilter = VK_FILTER_NEAREST,
            .minFilter = VK_FILTER_NEAREST,
            .mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
            .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
            .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
            .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
            .mipLodBias = 0.0f,
            .anisotropyEnable = VK_FALSE,
            .maxAnisotropy = 1,
            .compareOp = VK_COMPARE_OP_NEVER,
            .minLod = 0.0f,
            .maxLod = 0.0f,
            .borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE,
            .unnormalizedCoordinates = VK_FALSE,
        };

        VkImageViewCreateInfo view = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext = NULL,
            .image = VK_NULL_HANDLE,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = tex_format,
            .components =
                {
                 VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G,
                 VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A,
                },
            .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
            .flags = 0,
        };

        /* create sampler */
        err = vkCreateSampler(cr_vk_inner->device, &sampler, NULL,
                              &cr_vk_inner->textures[i].sampler);
        assert(!err);

        /* create image view */
        view.image = cr_vk_inner->textures[i].image;
        err = vkCreateImageView(cr_vk_inner->device, &view, NULL,
                                &cr_vk_inner->textures[i].view);
        assert(!err);
    }
}

void demo_prepare_cube_data_buffer(struct cr_vk_inner *cr_vk_inner) {
    VkBufferCreateInfo buf_info;
    VkMemoryRequirements mem_reqs;
    uint8_t *pData;
    int i;
    mat4x4 MVP, VP;
    VkResult U_ASSERT_ONLY err;
    bool U_ASSERT_ONLY pass;
    struct vktexcube_vs_uniform data;

    mat4x4_mul(VP, cr_vk_inner->projection_matrix, cr_vk_inner->view_matrix);
    mat4x4_mul(MVP, VP, cr_vk_inner->model_matrix);
    memcpy(data.mvp, MVP, sizeof(MVP));
    //    dumpMatrix("MVP", MVP);

    for (i = 0; i < 12 * 3; i++) {
        data.position[i][0] = g_vertex_buffer_data[i * 3];
        data.position[i][1] = g_vertex_buffer_data[i * 3 + 1];
        data.position[i][2] = g_vertex_buffer_data[i * 3 + 2];
        data.position[i][3] = 1.0f;
        data.attr[i][0] = g_uv_buffer_data[2 * i];
        data.attr[i][1] = g_uv_buffer_data[2 * i + 1];
        data.attr[i][2] = 0;
        data.attr[i][3] = 0;
    }

    memset(&buf_info, 0, sizeof(buf_info));
    buf_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buf_info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    buf_info.size = sizeof(data);
    err =
        vkCreateBuffer(cr_vk_inner->device, &buf_info, NULL, &cr_vk_inner->uniform_data.buf);
    assert(!err);

    vkGetBufferMemoryRequirements(cr_vk_inner->device, cr_vk_inner->uniform_data.buf,
                                  &mem_reqs);

    cr_vk_inner->uniform_data.mem_alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    cr_vk_inner->uniform_data.mem_alloc.pNext = NULL;
    cr_vk_inner->uniform_data.mem_alloc.allocationSize = mem_reqs.size;
    cr_vk_inner->uniform_data.mem_alloc.memoryTypeIndex = 0;

    pass = memory_type_from_properties(
        cr_vk_inner, mem_reqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
        &cr_vk_inner->uniform_data.mem_alloc.memoryTypeIndex);
    assert(pass);

    err = vkAllocateMemory(cr_vk_inner->device, &cr_vk_inner->uniform_data.mem_alloc, NULL,
                           &(cr_vk_inner->uniform_data.mem));
    assert(!err);

    err = vkMapMemory(cr_vk_inner->device, cr_vk_inner->uniform_data.mem, 0,
                      cr_vk_inner->uniform_data.mem_alloc.allocationSize, 0,
                      (void **)&pData);
    assert(!err);

    memcpy(pData, &data, sizeof data);

    vkUnmapMemory(cr_vk_inner->device, cr_vk_inner->uniform_data.mem);

    err = vkBindBufferMemory(cr_vk_inner->device, cr_vk_inner->uniform_data.buf,
                             cr_vk_inner->uniform_data.mem, 0);
    assert(!err);

    cr_vk_inner->uniform_data.buffer_info.buffer = cr_vk_inner->uniform_data.buf;
    cr_vk_inner->uniform_data.buffer_info.offset = 0;
    cr_vk_inner->uniform_data.buffer_info.range = sizeof(data);
}

static void demo_prepare_descriptor_layout(struct cr_vk_inner *cr_vk_inner) {
    const VkDescriptorSetLayoutBinding layout_bindings[2] = {
            [0] =
                {
                 .binding = 0,
                 .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                 .descriptorCount = 1,
                 .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
                 .pImmutableSamplers = NULL,
                },
            [1] =
                {
                 .binding = 1,
                 .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                 .descriptorCount = DEMO_TEXTURE_COUNT,
                 .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                 .pImmutableSamplers = NULL,
                },
    };
    const VkDescriptorSetLayoutCreateInfo descriptor_layout = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = NULL,
        .bindingCount = 2,
        .pBindings = layout_bindings,
    };
    VkResult U_ASSERT_ONLY err;

    err = vkCreateDescriptorSetLayout(cr_vk_inner->device, &descriptor_layout, NULL,
                                      &cr_vk_inner->desc_layout);
    assert(!err);

    const VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext = NULL,
        .setLayoutCount = 1,
        .pSetLayouts = &cr_vk_inner->desc_layout,
    };

    err = vkCreatePipelineLayout(cr_vk_inner->device, &pPipelineLayoutCreateInfo, NULL,
                                 &cr_vk_inner->pipeline_layout);
    assert(!err);
}

static void demo_prepare_render_pass(struct cr_vk_inner *cr_vk_inner) {
    const VkAttachmentDescription attachments[2] = {
            [0] =
                {
                 .format = cr_vk_inner->format,
                 .samples = VK_SAMPLE_COUNT_1_BIT,
                 .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                 .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                 .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                 .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                 .initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                 .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                },
            [1] =
                {
                 .format = cr_vk_inner->depth.format,
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
    VkResult U_ASSERT_ONLY err;

    err = vkCreateRenderPass(cr_vk_inner->device, &rp_info, NULL, &cr_vk_inner->render_pass);
    assert(!err);
}

static VkShaderModule
demo_prepare_shader_module(struct cr_vk_inner *cr_vk_inner, const void *code, size_t size) {
    VkShaderModule module;
    VkShaderModuleCreateInfo moduleCreateInfo;
    VkResult U_ASSERT_ONLY err;

    moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    moduleCreateInfo.pNext = NULL;

    moduleCreateInfo.codeSize = size;
    moduleCreateInfo.pCode = code;
    moduleCreateInfo.flags = 0;
    err = vkCreateShaderModule(cr_vk_inner->device, &moduleCreateInfo, NULL, &module);
    assert(!err);

    return module;
}

char *demo_read_spv(const char *filename, size_t *psize) {
    long int size;
    size_t U_ASSERT_ONLY retval;
    void *shader_code;

    FILE *fp = fopen(filename, "rb");
    if (!fp)
        return NULL;

    fseek(fp, 0L, SEEK_END);
    size = ftell(fp);

    fseek(fp, 0L, SEEK_SET);

    shader_code = malloc(size);
    retval = fread(shader_code, size, 1, fp);
    assert(retval == 1);

    *psize = size;

    fclose(fp);
    return shader_code;
}

static VkShaderModule demo_prepare_vs(struct cr_vk_inner *cr_vk_inner) {
    void *vertShaderCode;
    size_t size;

    vertShaderCode = demo_read_spv("cube-vert.spv", &size);

    cr_vk_inner->vert_shader_module =
        demo_prepare_shader_module(cr_vk_inner, vertShaderCode, size);

    free(vertShaderCode);

    return cr_vk_inner->vert_shader_module;
}

static VkShaderModule demo_prepare_fs(struct cr_vk_inner *cr_vk_inner) {
    void *fragShaderCode;
    size_t size;

    fragShaderCode = demo_read_spv("cube-frag.spv", &size);

    cr_vk_inner->frag_shader_module =
        demo_prepare_shader_module(cr_vk_inner, fragShaderCode, size);

    free(fragShaderCode);

    return cr_vk_inner->frag_shader_module;
}

static void demo_prepare_pipeline(struct cr_vk_inner *cr_vk_inner) {
    VkGraphicsPipelineCreateInfo pipeline;
    VkPipelineCacheCreateInfo pipelineCache;
    VkPipelineVertexInputStateCreateInfo vi;
    VkPipelineInputAssemblyStateCreateInfo ia;
    VkPipelineRasterizationStateCreateInfo rs;
    VkPipelineColorBlendStateCreateInfo cb;
    VkPipelineDepthStencilStateCreateInfo ds;
    VkPipelineViewportStateCreateInfo vp;
    VkPipelineMultisampleStateCreateInfo ms;
    VkDynamicState dynamicStateEnables[VK_DYNAMIC_STATE_RANGE_SIZE];
    VkPipelineDynamicStateCreateInfo dynamicState;
    VkResult U_ASSERT_ONLY err;

    memset(dynamicStateEnables, 0, sizeof dynamicStateEnables);
    memset(&dynamicState, 0, sizeof dynamicState);
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.pDynamicStates = dynamicStateEnables;

    memset(&pipeline, 0, sizeof(pipeline));
    pipeline.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline.layout = cr_vk_inner->pipeline_layout;

    memset(&vi, 0, sizeof(vi));
    vi.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    memset(&ia, 0, sizeof(ia));
    ia.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    memset(&rs, 0, sizeof(rs));
    rs.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rs.polygonMode = VK_POLYGON_MODE_FILL;
    rs.cullMode = VK_CULL_MODE_BACK_BIT;
    rs.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rs.depthClampEnable = VK_FALSE;
    rs.rasterizerDiscardEnable = VK_FALSE;
    rs.depthBiasEnable = VK_FALSE;

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

    // Two stages: vs and fs
    pipeline.stageCount = 2;
    VkPipelineShaderStageCreateInfo shaderStages[2];
    memset(&shaderStages, 0, 2 * sizeof(VkPipelineShaderStageCreateInfo));

    shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shaderStages[0].module = demo_prepare_vs(cr_vk_inner);
    shaderStages[0].pName = "main";

    shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaderStages[1].module = demo_prepare_fs(cr_vk_inner);
    shaderStages[1].pName = "main";

    memset(&pipelineCache, 0, sizeof(pipelineCache));
    pipelineCache.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;

    err = vkCreatePipelineCache(cr_vk_inner->device, &pipelineCache, NULL,
                                &cr_vk_inner->pipelineCache);
    assert(!err);

    pipeline.pVertexInputState = &vi;
    pipeline.pInputAssemblyState = &ia;
    pipeline.pRasterizationState = &rs;
    pipeline.pColorBlendState = &cb;
    pipeline.pMultisampleState = &ms;
    pipeline.pViewportState = &vp;
    pipeline.pDepthStencilState = &ds;
    pipeline.pStages = shaderStages;
    pipeline.renderPass = cr_vk_inner->render_pass;
    pipeline.pDynamicState = &dynamicState;

    pipeline.renderPass = cr_vk_inner->render_pass;

    err = vkCreateGraphicsPipelines(cr_vk_inner->device, cr_vk_inner->pipelineCache, 1,
                                    &pipeline, NULL, &cr_vk_inner->pipeline);
    assert(!err);

    vkDestroyShaderModule(cr_vk_inner->device, cr_vk_inner->frag_shader_module, NULL);
    vkDestroyShaderModule(cr_vk_inner->device, cr_vk_inner->vert_shader_module, NULL);
}

static void demo_prepare_descriptor_pool(struct cr_vk_inner *cr_vk_inner) {
    const VkDescriptorPoolSize type_counts[2] = {
            [0] =
                {
                 .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                 .descriptorCount = 1,
                },
            [1] =
                {
                 .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                 .descriptorCount = DEMO_TEXTURE_COUNT,
                },
    };
    const VkDescriptorPoolCreateInfo descriptor_pool = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext = NULL,
        .maxSets = 1,
        .poolSizeCount = 2,
        .pPoolSizes = type_counts,
    };
    VkResult U_ASSERT_ONLY err;

    err = vkCreateDescriptorPool(cr_vk_inner->device, &descriptor_pool, NULL,
                                 &cr_vk_inner->desc_pool);
    assert(!err);
}

static void demo_prepare_descriptor_set(struct cr_vk_inner *cr_vk_inner) {
    VkDescriptorImageInfo tex_descs[DEMO_TEXTURE_COUNT];
    VkWriteDescriptorSet writes[2];
    VkResult U_ASSERT_ONLY err;
    uint32_t i;

    VkDescriptorSetAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = NULL,
        .descriptorPool = cr_vk_inner->desc_pool,
        .descriptorSetCount = 1,
        .pSetLayouts = &cr_vk_inner->desc_layout};
    err = vkAllocateDescriptorSets(cr_vk_inner->device, &alloc_info, &cr_vk_inner->desc_set);
    assert(!err);

    memset(&tex_descs, 0, sizeof(tex_descs));
    for (i = 0; i < DEMO_TEXTURE_COUNT; i++) {
        tex_descs[i].sampler = cr_vk_inner->textures[i].sampler;
        tex_descs[i].imageView = cr_vk_inner->textures[i].view;
        tex_descs[i].imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    }

    memset(&writes, 0, sizeof(writes));

    writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[0].dstSet = cr_vk_inner->desc_set;
    writes[0].descriptorCount = 1;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writes[0].pBufferInfo = &cr_vk_inner->uniform_data.buffer_info;

    writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[1].dstSet = cr_vk_inner->desc_set;
    writes[1].dstBinding = 1;
    writes[1].descriptorCount = DEMO_TEXTURE_COUNT;
    writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[1].pImageInfo = tex_descs;

    vkUpdateDescriptorSets(cr_vk_inner->device, 2, writes, 0, NULL);
}

static void demo_prepare_framebuffers(struct cr_vk_inner *cr_vk_inner) {
    VkImageView attachments[2];
    attachments[1] = cr_vk_inner->depth.view;

    const VkFramebufferCreateInfo fb_info = {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .pNext = NULL,
        .renderPass = cr_vk_inner->render_pass,
        .attachmentCount = 2,
        .pAttachments = attachments,
        .width = cr_vk_inner->width,
        .height = cr_vk_inner->height,
        .layers = 1,
    };
    VkResult U_ASSERT_ONLY err;
    uint32_t i;

    cr_vk_inner->framebuffers = (VkFramebuffer *)malloc(cr_vk_inner->swapchainImageCount *
                                                 sizeof(VkFramebuffer));
    assert(cr_vk_inner->framebuffers);

    for (i = 0; i < cr_vk_inner->swapchainImageCount; i++) {
        attachments[0] = cr_vk_inner->buffers[i].view;
        err = vkCreateFramebuffer(cr_vk_inner->device, &fb_info, NULL,
                                  &cr_vk_inner->framebuffers[i]);
        assert(!err);
    }
}

static void demo_prepare(struct cr_vk_inner *cr_vk_inner) {
    VkResult U_ASSERT_ONLY err;

    const VkCommandPoolCreateInfo cmd_pool_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext = NULL,
        .queueFamilyIndex = cr_vk_inner->graphics_queue_node_index,
        .flags = 0,
    };
    err = vkCreateCommandPool(cr_vk_inner->device, &cmd_pool_info, NULL,
                              &cr_vk_inner->cmd_pool);
    assert(!err);

    const VkCommandBufferAllocateInfo cmd = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = NULL,
        .commandPool = cr_vk_inner->cmd_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };

    demo_prepare_buffers(cr_vk_inner);
    demo_prepare_depth(cr_vk_inner);
    demo_prepare_textures(cr_vk_inner);
    demo_prepare_cube_data_buffer(cr_vk_inner);

    demo_prepare_descriptor_layout(cr_vk_inner);
    demo_prepare_render_pass(cr_vk_inner);
    demo_prepare_pipeline(cr_vk_inner);

    for (uint32_t i = 0; i < cr_vk_inner->swapchainImageCount; i++) {
        err =
            vkAllocateCommandBuffers(cr_vk_inner->device, &cmd, &cr_vk_inner->buffers[i].cmd);
        assert(!err);
    }

    demo_prepare_descriptor_pool(cr_vk_inner);
    demo_prepare_descriptor_set(cr_vk_inner);

    demo_prepare_framebuffers(cr_vk_inner);

    for (uint32_t i = 0; i < cr_vk_inner->swapchainImageCount; i++) {
        cr_vk_inner->current_buffer = i;
        demo_draw_build_cmd(cr_vk_inner, cr_vk_inner->buffers[i].cmd);
    }

    /*
     * Prepare functions above may generate pipeline commands
     * that need to be flushed before beginning the render loop.
     */
    demo_flush_init_cmd(cr_vk_inner);

    cr_vk_inner->current_buffer = 0;
    cr_vk_inner->prepared = true;
}

static void demo_cleanup(struct cr_vk_inner *cr_vk_inner) {
    uint32_t i;

    cr_vk_inner->prepared = false;

    for (i = 0; i < cr_vk_inner->swapchainImageCount; i++) {
        vkDestroyFramebuffer(cr_vk_inner->device, cr_vk_inner->framebuffers[i], NULL);
    }
    free(cr_vk_inner->framebuffers);
    vkDestroyDescriptorPool(cr_vk_inner->device, cr_vk_inner->desc_pool, NULL);

    vkDestroyPipeline(cr_vk_inner->device, cr_vk_inner->pipeline, NULL);
    vkDestroyPipelineCache(cr_vk_inner->device, cr_vk_inner->pipelineCache, NULL);
    vkDestroyRenderPass(cr_vk_inner->device, cr_vk_inner->render_pass, NULL);
    vkDestroyPipelineLayout(cr_vk_inner->device, cr_vk_inner->pipeline_layout, NULL);
    vkDestroyDescriptorSetLayout(cr_vk_inner->device, cr_vk_inner->desc_layout, NULL);

    for (i = 0; i < DEMO_TEXTURE_COUNT; i++) {
        vkDestroyImageView(cr_vk_inner->device, cr_vk_inner->textures[i].view, NULL);
        vkDestroyImage(cr_vk_inner->device, cr_vk_inner->textures[i].image, NULL);
        vkFreeMemory(cr_vk_inner->device, cr_vk_inner->textures[i].mem, NULL);
        vkDestroySampler(cr_vk_inner->device, cr_vk_inner->textures[i].sampler, NULL);
    }
    cr_vk_inner->fpDestroySwapchainKHR(cr_vk_inner->device, cr_vk_inner->swapchain, NULL);

    vkDestroyImageView(cr_vk_inner->device, cr_vk_inner->depth.view, NULL);
    vkDestroyImage(cr_vk_inner->device, cr_vk_inner->depth.image, NULL);
    vkFreeMemory(cr_vk_inner->device, cr_vk_inner->depth.mem, NULL);

    vkDestroyBuffer(cr_vk_inner->device, cr_vk_inner->uniform_data.buf, NULL);
    vkFreeMemory(cr_vk_inner->device, cr_vk_inner->uniform_data.mem, NULL);

    for (i = 0; i < cr_vk_inner->swapchainImageCount; i++) {
        vkDestroyImageView(cr_vk_inner->device, cr_vk_inner->buffers[i].view, NULL);
        vkFreeCommandBuffers(cr_vk_inner->device, cr_vk_inner->cmd_pool, 1,
                             &cr_vk_inner->buffers[i].cmd);
    }
    free(cr_vk_inner->buffers);

    free(cr_vk_inner->queue_props);

    vkDestroyCommandPool(cr_vk_inner->device, cr_vk_inner->cmd_pool, NULL);
    vkDestroyDevice(cr_vk_inner->device, NULL);
    if (cr_vk_inner->validate) {
        cr_vk_inner->DestroyDebugReportCallback(cr_vk_inner->inst, cr_vk_inner->msg_callback, NULL);
    }
    vkDestroySurfaceKHR(cr_vk_inner->inst, cr_vk_inner->surface, NULL);
    vkDestroyInstance(cr_vk_inner->inst, NULL);
}

static void demo_resize(struct cr_vk_inner *cr_vk_inner) {
    uint32_t i;

    // Don't react to resize until after first initialization.
    if (!cr_vk_inner->prepared) {
        return;
    }
    // In order to properly resize the window, we must re-create the swapchain
    // AND redo the command buffers, etc.
    //
    // First, perform part of the demo_cleanup() function:
    cr_vk_inner->prepared = false;

    for (i = 0; i < cr_vk_inner->swapchainImageCount; i++) {
        vkDestroyFramebuffer(cr_vk_inner->device, cr_vk_inner->framebuffers[i], NULL);
    }
    free(cr_vk_inner->framebuffers);
    vkDestroyDescriptorPool(cr_vk_inner->device, cr_vk_inner->desc_pool, NULL);

    vkDestroyPipeline(cr_vk_inner->device, cr_vk_inner->pipeline, NULL);
    vkDestroyPipelineCache(cr_vk_inner->device, cr_vk_inner->pipelineCache, NULL);
    vkDestroyRenderPass(cr_vk_inner->device, cr_vk_inner->render_pass, NULL);
    vkDestroyPipelineLayout(cr_vk_inner->device, cr_vk_inner->pipeline_layout, NULL);
    vkDestroyDescriptorSetLayout(cr_vk_inner->device, cr_vk_inner->desc_layout, NULL);

    for (i = 0; i < DEMO_TEXTURE_COUNT; i++) {
        vkDestroyImageView(cr_vk_inner->device, cr_vk_inner->textures[i].view, NULL);
        vkDestroyImage(cr_vk_inner->device, cr_vk_inner->textures[i].image, NULL);
        vkFreeMemory(cr_vk_inner->device, cr_vk_inner->textures[i].mem, NULL);
        vkDestroySampler(cr_vk_inner->device, cr_vk_inner->textures[i].sampler, NULL);
    }

    vkDestroyImageView(cr_vk_inner->device, cr_vk_inner->depth.view, NULL);
    vkDestroyImage(cr_vk_inner->device, cr_vk_inner->depth.image, NULL);
    vkFreeMemory(cr_vk_inner->device, cr_vk_inner->depth.mem, NULL);

    vkDestroyBuffer(cr_vk_inner->device, cr_vk_inner->uniform_data.buf, NULL);
    vkFreeMemory(cr_vk_inner->device, cr_vk_inner->uniform_data.mem, NULL);

    for (i = 0; i < cr_vk_inner->swapchainImageCount; i++) {
        vkDestroyImageView(cr_vk_inner->device, cr_vk_inner->buffers[i].view, NULL);
        vkFreeCommandBuffers(cr_vk_inner->device, cr_vk_inner->cmd_pool, 1,
                             &cr_vk_inner->buffers[i].cmd);
    }
    vkDestroyCommandPool(cr_vk_inner->device, cr_vk_inner->cmd_pool, NULL);
    free(cr_vk_inner->buffers);

    // Second, re-perform the demo_prepare() function, which will re-create the
    // swapchain:
    demo_prepare(cr_vk_inner);
}

// On MS-Windows, make this a global, so it's available to WndProc()
struct cr_vk_inner cr_vk_inner;

static void demo_run(struct cr_vk_inner *cr_vk_inner) {
    if (!cr_vk_inner->prepared)
        return;
    // Wait for work to finish before updating MVP.
    vkDeviceWaitIdle(cr_vk_inner->device);
    demo_update_data_buffer(cr_vk_inner);

    demo_draw(cr_vk_inner);

    // Wait for work to finish before updating MVP.
    vkDeviceWaitIdle(cr_vk_inner->device);

    cr_vk_inner->curFrame++;

    if (cr_vk_inner->frameCount != INT_MAX && cr_vk_inner->curFrame == cr_vk_inner->frameCount) {
        cr_vk_inner->quit = true;
        demo_cleanup(cr_vk_inner);
        ExitProcess(0);
    }
}

// MS-Windows event handling function:
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CLOSE:
        PostQuitMessage(0);
        break;
    case WM_PAINT:
        demo_run(&cr_vk_inner);
        break;
    case WM_SIZE:
        cr_vk_inner.width = lParam & 0xffff;
        cr_vk_inner.height = lParam & 0xffff0000 >> 16;
        demo_resize(&cr_vk_inner);
        break;
    default:
        break;
    }
    return (DefWindowProc(hWnd, uMsg, wParam, lParam));
}

static void demo_create_window(struct cr_vk_inner *cr_vk_inner) {
    WNDCLASSEX win_class;

    // Initialize the window class structure:
    win_class.cbSize = sizeof(WNDCLASSEX);
    win_class.style = CS_HREDRAW | CS_VREDRAW;
    win_class.lpfnWndProc = WndProc;
    win_class.cbClsExtra = 0;
    win_class.cbWndExtra = 0;
    win_class.hInstance = cr_vk_inner->connection; // hInstance
    win_class.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    win_class.hCursor = LoadCursor(NULL, IDC_ARROW);
    win_class.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    win_class.lpszMenuName = NULL;
    win_class.lpszClassName = cr_vk_inner->name;
    win_class.hIconSm = LoadIcon(NULL, IDI_WINLOGO);
    // Register window class:
    if (!RegisterClassEx(&win_class)) {
        // It didn't work, so try to give a useful error:
        printf("Unexpected error trying to start the application!\n");
        fflush(stdout);
        exit(1);
    }
    // Create window with the registered class:
    RECT wr = {0, 0, cr_vk_inner->width, cr_vk_inner->height};
    AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);
    cr_vk_inner->window = CreateWindowEx(0,
                                  cr_vk_inner->name,           // class name
                                  cr_vk_inner->name,           // app name
                                  WS_OVERLAPPEDWINDOW | // window style
                                      WS_VISIBLE | WS_SYSMENU,
                                  100, 100,           // x/y coords
                                  wr.right - wr.left, // width
                                  wr.bottom - wr.top, // height
                                  NULL,               // handle to parent
                                  NULL,               // handle to menu
                                  cr_vk_inner->connection,   // hInstance
                                  NULL);              // no extra parameters
    if (!cr_vk_inner->window) {
        // It didn't work, so try to give a useful error:
        printf("Cannot create a window in which to draw!\n");
        fflush(stdout);
        exit(1);
    }
}

static void demo_init(struct cr_vk_inner *cr_vk_inner, int argc, char **argv) {
    vec3 eye = {0.0f, 3.0f, 5.0f};
    vec3 origin = {0, 0, 0};
    vec3 up = {0.0f, 1.0f, 0.0};

    memset(cr_vk_inner, 0, sizeof(*cr_vk_inner));
    cr_vk_inner->frameCount = INT32_MAX;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--use_staging") == 0) {
            cr_vk_inner->use_staging_buffer = true;
            continue;
        }
        if (strcmp(argv[i], "--break") == 0) {
            cr_vk_inner->use_break = true;
            continue;
        }
        if (strcmp(argv[i], "--validate") == 0) {
            cr_vk_inner->validate = true;
            continue;
        }
        if (strcmp(argv[i], "--c") == 0 && cr_vk_inner->frameCount == INT32_MAX &&
            i < argc - 1 && sscanf(argv[i + 1], "%d", &cr_vk_inner->frameCount) == 1 &&
            cr_vk_inner->frameCount >= 0) {
            i++;
            continue;
        }

        fprintf(stderr, "Usage:\n  %s [--use_staging] [--validate] [--break] "
                        "[--c <framecount>]\n",
                APP_SHORT_NAME);
        fflush(stderr);
        exit(1);
    }

    demo_init_vk(cr_vk_inner);

    cr_vk_inner->width = 500;
    cr_vk_inner->height = 500;

    cr_vk_inner->spin_angle = 0.01f;
    cr_vk_inner->spin_increment = 0.01f;
    cr_vk_inner->pause = false;

    mat4x4_perspective(cr_vk_inner->projection_matrix, (float)degreesToRadians(45.0f),
                       1.0f, 0.1f, 100.0f);
    mat4x4_look_at(cr_vk_inner->view_matrix, eye, origin, up);
    mat4x4_identity(cr_vk_inner->model_matrix);
}

// Include header required for parsing the command line options.
#include <shellapi.h>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine,
                   int nCmdShow) {
    MSG msg;   // message
    bool done; // flag saying when app is complete
    int argc;
    char **argv;

    // Use the CommandLine functions to get the command line arguments.
    // Unfortunately, Microsoft outputs
    // this information as wide characters for Unicode, and we simply want the
    // Ascii version to be compatible
    // with the non-Windows side.  So, we have to convert the information to
    // Ascii character strings.
    LPWSTR *commandLineArgs = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (NULL == commandLineArgs) {
        argc = 0;
    }

    if (argc > 0) {
        argv = (char **)malloc(sizeof(char *) * argc);
        if (argv == NULL) {
            argc = 0;
        } else {
            for (int iii = 0; iii < argc; iii++) {
                size_t wideCharLen = wcslen(commandLineArgs[iii]);
                size_t numConverted = 0;

                argv[iii] = (char *)malloc(sizeof(char) * (wideCharLen + 1));
                if (argv[iii] != NULL) {
                    wcstombs_s(&numConverted, argv[iii], wideCharLen + 1,
                               commandLineArgs[iii], wideCharLen + 1);
                }
            }
        }
    } else {
        argv = NULL;
    }

    demo_init(&cr_vk_inner, argc, argv);

    // Free up the items we had to allocate for the command line arguments.
    if (argc > 0 && argv != NULL) {
        for (int iii = 0; iii < argc; iii++) {
            if (argv[iii] != NULL) {
                free(argv[iii]);
            }
        }
        free(argv);
    }

    cr_vk_inner.connection = hInstance;
    strncpy(cr_vk_inner.name, "cube", APP_NAME_STR_LEN);
    demo_create_window(&cr_vk_inner);
    demo_init_vk_swapchain(&cr_vk_inner);

    demo_prepare(&cr_vk_inner);

    done = false; // initialize loop condition variable

    // main message loop
    while (!done) {
        PeekMessage(&msg, NULL, 0, 0, PM_REMOVE);
        if (msg.message == WM_QUIT) // check for a quit message
        {
            done = true; // if found, quit app
        } else {
            /* Translate and dispatch to event queue*/
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        RedrawWindow(cr_vk_inner.window, NULL, NULL, RDW_INTERNALPAINT);
    }

    demo_cleanup(&cr_vk_inner);

    return (int)msg.wParam;
}

int main(int argc, char** argv)
{
    return WinMain(GetModuleHandle(NULL), NULL, NULL, 0);
}