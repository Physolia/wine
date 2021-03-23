/*
 * Copyright 2002-2004 Jason Edmeades
 * Copyright 2002-2004 Raphael Junqueira
 * Copyright 2004 Christian Costa
 * Copyright 2005 Oliver Stieber
 * Copyright 2006, 2008 Henri Verbeet
 * Copyright 2007-2011, 2013 Stefan Dösinger for CodeWeavers
 * Copyright 2009-2020 Henri Verbeet for CodeWeavers
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"
#include "wine/port.h"

#include "wined3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d);

VkCompareOp vk_compare_op_from_wined3d(enum wined3d_cmp_func op)
{
    switch (op)
    {
        case WINED3D_CMP_NEVER:
            return VK_COMPARE_OP_NEVER;
        case WINED3D_CMP_LESS:
            return VK_COMPARE_OP_LESS;
        case WINED3D_CMP_EQUAL:
            return VK_COMPARE_OP_EQUAL;
        case WINED3D_CMP_LESSEQUAL:
            return VK_COMPARE_OP_LESS_OR_EQUAL;
        case WINED3D_CMP_GREATER:
            return VK_COMPARE_OP_GREATER;
        case WINED3D_CMP_NOTEQUAL:
            return VK_COMPARE_OP_NOT_EQUAL;
        case WINED3D_CMP_GREATEREQUAL:
            return VK_COMPARE_OP_GREATER_OR_EQUAL;
        case WINED3D_CMP_ALWAYS:
            return VK_COMPARE_OP_ALWAYS;
        default:
            if (!op)
                WARN("Unhandled compare operation %#x.\n", op);
            else
                FIXME("Unhandled compare operation %#x.\n", op);
            return VK_COMPARE_OP_NEVER;
    }
}

VkShaderStageFlagBits vk_shader_stage_from_wined3d(enum wined3d_shader_type shader_type)
{
    switch (shader_type)
    {
        case WINED3D_SHADER_TYPE_VERTEX:
            return VK_SHADER_STAGE_VERTEX_BIT;
        case WINED3D_SHADER_TYPE_HULL:
            return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
        case WINED3D_SHADER_TYPE_DOMAIN:
            return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
        case WINED3D_SHADER_TYPE_GEOMETRY:
            return VK_SHADER_STAGE_GEOMETRY_BIT;
        case WINED3D_SHADER_TYPE_PIXEL:
            return VK_SHADER_STAGE_FRAGMENT_BIT;
        case WINED3D_SHADER_TYPE_COMPUTE:
            return VK_SHADER_STAGE_COMPUTE_BIT;
        default:
            ERR("Unhandled shader type %s.\n", debug_shader_type(shader_type));
            return 0;
    }
}

static VkBlendFactor vk_blend_factor_from_wined3d(enum wined3d_blend blend,
        const struct wined3d_format *dst_format, bool alpha)
{
    switch (blend)
    {
        case WINED3D_BLEND_ZERO:
            return VK_BLEND_FACTOR_ZERO;
        case WINED3D_BLEND_ONE:
            return VK_BLEND_FACTOR_ONE;
        case WINED3D_BLEND_SRCCOLOR:
            return VK_BLEND_FACTOR_SRC_COLOR;
        case WINED3D_BLEND_INVSRCCOLOR:
            return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
        case WINED3D_BLEND_SRCALPHA:
            return VK_BLEND_FACTOR_SRC_ALPHA;
        case WINED3D_BLEND_INVSRCALPHA:
            return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        case WINED3D_BLEND_DESTALPHA:
            if (dst_format->alpha_size)
                return VK_BLEND_FACTOR_DST_ALPHA;
            return VK_BLEND_FACTOR_ONE;
        case WINED3D_BLEND_INVDESTALPHA:
            if (dst_format->alpha_size)
                return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
            return VK_BLEND_FACTOR_ZERO;
        case WINED3D_BLEND_DESTCOLOR:
            return VK_BLEND_FACTOR_DST_COLOR;
        case WINED3D_BLEND_INVDESTCOLOR:
            return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
        case WINED3D_BLEND_SRCALPHASAT:
            return VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;
        case WINED3D_BLEND_BLENDFACTOR:
            if (alpha)
                return VK_BLEND_FACTOR_CONSTANT_ALPHA;
            return VK_BLEND_FACTOR_CONSTANT_COLOR;
        case WINED3D_BLEND_INVBLENDFACTOR:
            if (alpha)
                return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA;
            return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;
        case WINED3D_BLEND_SRC1COLOR:
            return VK_BLEND_FACTOR_SRC1_COLOR;
        case WINED3D_BLEND_INVSRC1COLOR:
            return VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR;
        case WINED3D_BLEND_SRC1ALPHA:
            return VK_BLEND_FACTOR_SRC1_ALPHA;
        case WINED3D_BLEND_INVSRC1ALPHA:
            return VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA;
        default:
            FIXME("Unhandled blend %#x.\n", blend);
            return VK_BLEND_FACTOR_ZERO;
    }
}

static VkBlendOp vk_blend_op_from_wined3d(enum wined3d_blend_op op)
{
    switch (op)
    {
        case WINED3D_BLEND_OP_ADD:
            return VK_BLEND_OP_ADD;
        case WINED3D_BLEND_OP_SUBTRACT:
            return VK_BLEND_OP_SUBTRACT;
        case WINED3D_BLEND_OP_REVSUBTRACT:
            return VK_BLEND_OP_REVERSE_SUBTRACT;
        case WINED3D_BLEND_OP_MIN:
            return VK_BLEND_OP_MIN;
        case WINED3D_BLEND_OP_MAX:
            return VK_BLEND_OP_MAX;
        default:
            FIXME("Unhandled blend op %#x.\n", op);
            return VK_BLEND_OP_ADD;
    }
}

static VkColorComponentFlags vk_colour_write_mask_from_wined3d(uint32_t wined3d_mask)
{
    VkColorComponentFlags vk_mask = 0;

    if (wined3d_mask & WINED3DCOLORWRITEENABLE_RED)
        vk_mask |= VK_COLOR_COMPONENT_R_BIT;
    if (wined3d_mask & WINED3DCOLORWRITEENABLE_GREEN)
        vk_mask |= VK_COLOR_COMPONENT_G_BIT;
    if (wined3d_mask & WINED3DCOLORWRITEENABLE_BLUE)
        vk_mask |= VK_COLOR_COMPONENT_B_BIT;
    if (wined3d_mask & WINED3DCOLORWRITEENABLE_ALPHA)
        vk_mask |= VK_COLOR_COMPONENT_A_BIT;

    return vk_mask;
}

static VkCullModeFlags vk_cull_mode_from_wined3d(enum wined3d_cull mode)
{
    switch (mode)
    {
        case WINED3D_CULL_NONE:
            return VK_CULL_MODE_NONE;
        case WINED3D_CULL_FRONT:
            return VK_CULL_MODE_FRONT_BIT;
        case WINED3D_CULL_BACK:
            return VK_CULL_MODE_BACK_BIT;
        default:
            FIXME("Unhandled cull mode %#x.\n", mode);
            return VK_CULL_MODE_NONE;
    }
}

static VkPrimitiveTopology vk_topology_from_wined3d(enum wined3d_primitive_type t)
{
    switch (t)
    {
        case WINED3D_PT_POINTLIST:
            return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
        case WINED3D_PT_LINELIST:
            return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
        case WINED3D_PT_LINESTRIP:
            return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
        case WINED3D_PT_TRIANGLELIST:
            return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        case WINED3D_PT_TRIANGLESTRIP:
            return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
        case WINED3D_PT_TRIANGLEFAN:
            return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
        case WINED3D_PT_LINELIST_ADJ:
            return VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY;
        case WINED3D_PT_LINESTRIP_ADJ:
            return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY;
        case WINED3D_PT_TRIANGLELIST_ADJ:
            return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY;
        case WINED3D_PT_TRIANGLESTRIP_ADJ:
            return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY;
        case WINED3D_PT_PATCH:
            return VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;
        default:
            FIXME("Unhandled primitive type %s.\n", debug_d3dprimitivetype(t));
        case WINED3D_PT_UNDEFINED:
            return ~0u;
    }
}

static VkStencilOp vk_stencil_op_from_wined3d(enum wined3d_stencil_op op)
{
    switch (op)
    {
        case WINED3D_STENCIL_OP_KEEP:
            return VK_STENCIL_OP_KEEP;
        case WINED3D_STENCIL_OP_ZERO:
            return VK_STENCIL_OP_ZERO;
        case WINED3D_STENCIL_OP_REPLACE:
            return VK_STENCIL_OP_REPLACE;
        case WINED3D_STENCIL_OP_INCR_SAT:
            return VK_STENCIL_OP_INCREMENT_AND_CLAMP;
        case WINED3D_STENCIL_OP_DECR_SAT:
            return VK_STENCIL_OP_DECREMENT_AND_CLAMP;
        case WINED3D_STENCIL_OP_INVERT:
            return VK_STENCIL_OP_INVERT;
        case WINED3D_STENCIL_OP_INCR:
            return VK_STENCIL_OP_INCREMENT_AND_WRAP;
        case WINED3D_STENCIL_OP_DECR:
            return VK_STENCIL_OP_DECREMENT_AND_WRAP;
        default:
            if (!op)
                WARN("Unhandled stencil operation %#x.\n", op);
            else
                FIXME("Unhandled stencil operation %#x.\n", op);
            return VK_STENCIL_OP_KEEP;
    }
}

void *wined3d_allocator_chunk_vk_map(struct wined3d_allocator_chunk_vk *chunk_vk,
        struct wined3d_context_vk *context_vk)
{
    struct wined3d_device_vk *device_vk = wined3d_device_vk(context_vk->c.device);
    const struct wined3d_vk_info *vk_info = context_vk->vk_info;
    VkResult vr;

    TRACE("chunk %p, memory 0x%s, map_ptr %p.\n", chunk_vk,
            wine_dbgstr_longlong(chunk_vk->vk_memory), chunk_vk->c.map_ptr);

    if (!chunk_vk->c.map_ptr && (vr = VK_CALL(vkMapMemory(device_vk->vk_device,
            chunk_vk->vk_memory, 0, VK_WHOLE_SIZE, 0, &chunk_vk->c.map_ptr))) < 0)
    {
        ERR("Failed to map chunk memory, vr %s.\n", wined3d_debug_vkresult(vr));
        return NULL;
    }

    ++chunk_vk->c.map_count;

    return chunk_vk->c.map_ptr;
}

void wined3d_allocator_chunk_vk_unmap(struct wined3d_allocator_chunk_vk *chunk_vk,
        struct wined3d_context_vk *context_vk)
{
    struct wined3d_device_vk *device_vk = wined3d_device_vk(context_vk->c.device);
    const struct wined3d_vk_info *vk_info = context_vk->vk_info;

    TRACE("chunk_vk %p, context_vk %p.\n", chunk_vk, context_vk);

    if (--chunk_vk->c.map_count)
        return;

    VK_CALL(vkUnmapMemory(device_vk->vk_device, chunk_vk->vk_memory));
    chunk_vk->c.map_ptr = NULL;
}

VkDeviceMemory wined3d_context_vk_allocate_vram_chunk_memory(struct wined3d_context_vk *context_vk,
        unsigned int pool, size_t size)
{
    struct wined3d_device_vk *device_vk = wined3d_device_vk(context_vk->c.device);
    const struct wined3d_vk_info *vk_info = context_vk->vk_info;
    VkMemoryAllocateInfo allocate_info;
    VkDeviceMemory vk_memory;
    VkResult vr;

    allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocate_info.pNext = NULL;
    allocate_info.allocationSize = size;
    allocate_info.memoryTypeIndex = pool;
    if ((vr = VK_CALL(vkAllocateMemory(device_vk->vk_device, &allocate_info, NULL, &vk_memory))) < 0)
    {
        ERR("Failed to allocate memory, vr %s.\n", wined3d_debug_vkresult(vr));
        return VK_NULL_HANDLE;
    }

    return vk_memory;
}

struct wined3d_allocator_block *wined3d_context_vk_allocate_memory(struct wined3d_context_vk *context_vk,
        unsigned int memory_type, VkDeviceSize size, VkDeviceMemory *vk_memory)
{
    struct wined3d_device_vk *device_vk = wined3d_device_vk(context_vk->c.device);
    struct wined3d_allocator *allocator = &device_vk->allocator;
    struct wined3d_allocator_block *block;

    if (size > WINED3D_ALLOCATOR_CHUNK_SIZE / 2)
    {
        *vk_memory = wined3d_context_vk_allocate_vram_chunk_memory(context_vk, memory_type, size);
        return NULL;
    }

    if (!(block = wined3d_allocator_allocate(allocator, &context_vk->c, memory_type, size)))
    {
        *vk_memory = VK_NULL_HANDLE;
        return NULL;
    }

    *vk_memory = wined3d_allocator_chunk_vk(block->chunk)->vk_memory;

    return block;
}

static bool wined3d_context_vk_create_slab_bo(struct wined3d_context_vk *context_vk,
        VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memory_type, struct wined3d_bo_vk *bo)
{
    const struct wined3d_adapter_vk *adapter_vk = wined3d_adapter_vk(context_vk->c.device->adapter);
    const VkPhysicalDeviceLimits *limits = &adapter_vk->device_limits;
    struct wined3d_bo_slab_vk_key key;
    struct wined3d_bo_slab_vk *slab;
    struct wine_rb_entry *entry;
    size_t object_size, idx;
    size_t alignment;

    if (size > WINED3D_ALLOCATOR_MIN_BLOCK_SIZE / 2)
        return false;

    alignment = WINED3D_SLAB_BO_MIN_OBJECT_ALIGN;
    if ((usage & (VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT))
            && limits->minTexelBufferOffsetAlignment > alignment)
        alignment = limits->minTexelBufferOffsetAlignment;
    if ((usage & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT) && limits->minUniformBufferOffsetAlignment)
        alignment = limits->minUniformBufferOffsetAlignment;
    if ((usage & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT) && limits->minStorageBufferOffsetAlignment)
        alignment = limits->minStorageBufferOffsetAlignment;

    object_size = (size + (alignment - 1)) & ~(alignment - 1);
    if (object_size < WINED3D_ALLOCATOR_MIN_BLOCK_SIZE / 32)
        object_size = WINED3D_ALLOCATOR_MIN_BLOCK_SIZE / 32;
    key.memory_type = memory_type;
    key.usage = usage;
    key.size = 32 * object_size;

    if ((entry = wine_rb_get(&context_vk->bo_slab_available, &key)))
    {
        slab = WINE_RB_ENTRY_VALUE(entry, struct wined3d_bo_slab_vk, entry);
        TRACE("Using existing bo slab %p.\n", slab);
    }
    else
    {
        if (!(slab = heap_alloc_zero(sizeof(*slab))))
        {
            ERR("Failed to allocate bo slab.\n");
            return false;
        }

        slab->requested_memory_type = memory_type;
        if (!wined3d_context_vk_create_bo(context_vk, key.size, usage, memory_type, &slab->bo))
        {
            ERR("Failed to create slab bo.\n");
            heap_free(slab);
            return false;
        }
        slab->map = ~0u;

        if (wine_rb_put(&context_vk->bo_slab_available, &key, &slab->entry) < 0)
        {
            ERR("Failed to add slab to available tree.\n");
            wined3d_context_vk_destroy_bo(context_vk, &slab->bo);
            heap_free(slab);
            return false;
        }

        TRACE("Created new bo slab %p.\n", slab);
    }

    idx = wined3d_bit_scan(&slab->map);
    if (!slab->map)
    {
        if (slab->next)
        {
            wine_rb_replace(&context_vk->bo_slab_available, &slab->entry, &slab->next->entry);
            slab->next = NULL;
        }
        else
        {
            wine_rb_remove(&context_vk->bo_slab_available, &slab->entry);
        }
    }

    *bo = slab->bo;
    bo->memory = NULL;
    bo->slab = slab;
    bo->buffer_offset = idx * object_size;
    bo->memory_offset = slab->bo.memory_offset + bo->buffer_offset;
    bo->size = size;
    list_init(&bo->users);
    bo->command_buffer_id = 0;

    TRACE("Using buffer 0x%s, memory 0x%s, offset 0x%s for bo %p.\n",
            wine_dbgstr_longlong(bo->vk_buffer), wine_dbgstr_longlong(bo->vk_memory),
            wine_dbgstr_longlong(bo->buffer_offset), bo);

    return true;
}

BOOL wined3d_context_vk_create_bo(struct wined3d_context_vk *context_vk, VkDeviceSize size,
        VkBufferUsageFlags usage, VkMemoryPropertyFlags memory_type, struct wined3d_bo_vk *bo)
{
    struct wined3d_device_vk *device_vk = wined3d_device_vk(context_vk->c.device);
    const struct wined3d_vk_info *vk_info = context_vk->vk_info;
    VkMemoryRequirements memory_requirements;
    struct wined3d_adapter_vk *adapter_vk;
    VkBufferCreateInfo create_info;
    unsigned int memory_type_idx;
    VkResult vr;

    if (wined3d_context_vk_create_slab_bo(context_vk, size, usage, memory_type, bo))
        return TRUE;

    adapter_vk = wined3d_adapter_vk(device_vk->d.adapter);

    create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    create_info.pNext = NULL;
    create_info.flags = 0;
    create_info.size = size;
    create_info.usage = usage;
    create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    create_info.queueFamilyIndexCount = 0;
    create_info.pQueueFamilyIndices = NULL;

    if ((vr = VK_CALL(vkCreateBuffer(device_vk->vk_device, &create_info, NULL, &bo->vk_buffer))) < 0)
    {
        ERR("Failed to create Vulkan buffer, vr %s.\n", wined3d_debug_vkresult(vr));
        return FALSE;
    }

    VK_CALL(vkGetBufferMemoryRequirements(device_vk->vk_device, bo->vk_buffer, &memory_requirements));

    memory_type_idx = wined3d_adapter_vk_get_memory_type_index(adapter_vk,
            memory_requirements.memoryTypeBits, memory_type);
    if (memory_type_idx == ~0u)
    {
        ERR("Failed to find suitable memory type.\n");
        VK_CALL(vkDestroyBuffer(device_vk->vk_device, bo->vk_buffer, NULL));
        return FALSE;
    }
    bo->memory = wined3d_context_vk_allocate_memory(context_vk,
            memory_type_idx, memory_requirements.size, &bo->vk_memory);
    if (!bo->vk_memory)
    {
        ERR("Failed to allocate buffer memory.\n");
        VK_CALL(vkDestroyBuffer(device_vk->vk_device, bo->vk_buffer, NULL));
        return FALSE;
    }
    bo->memory_offset = bo->memory ? bo->memory->offset : 0;

    if ((vr = VK_CALL(vkBindBufferMemory(device_vk->vk_device, bo->vk_buffer,
            bo->vk_memory, bo->memory_offset))) < 0)
    {
        ERR("Failed to bind buffer memory, vr %s.\n", wined3d_debug_vkresult(vr));
        if (bo->memory)
            wined3d_allocator_block_free(bo->memory);
        else
            VK_CALL(vkFreeMemory(device_vk->vk_device, bo->vk_memory, NULL));
        VK_CALL(vkDestroyBuffer(device_vk->vk_device, bo->vk_buffer, NULL));
        return FALSE;
    }

    bo->map_ptr = NULL;
    bo->buffer_offset = 0;
    bo->size = size;
    bo->usage = usage;
    bo->memory_type = adapter_vk->memory_properties.memoryTypes[memory_type_idx].propertyFlags;
    list_init(&bo->users);
    bo->command_buffer_id = 0;
    bo->slab = NULL;

    TRACE("Created buffer 0x%s, memory 0x%s for bo %p.\n",
            wine_dbgstr_longlong(bo->vk_buffer), wine_dbgstr_longlong(bo->vk_memory), bo);

    return TRUE;
}

BOOL wined3d_context_vk_create_image(struct wined3d_context_vk *context_vk, VkImageType vk_image_type,
        VkImageUsageFlags usage, VkFormat vk_format, unsigned int width, unsigned int height, unsigned int depth,
        unsigned int sample_count, unsigned int mip_levels, unsigned int layer_count, unsigned int flags,
        struct wined3d_image_vk *image)
{
    struct wined3d_adapter_vk *adapter_vk = wined3d_adapter_vk(context_vk->c.device->adapter);
    struct wined3d_device_vk *device_vk = wined3d_device_vk(context_vk->c.device);
    const struct wined3d_vk_info *vk_info = context_vk->vk_info;
    VkMemoryRequirements memory_requirements;
    VkImageCreateInfo create_info;
    unsigned int memory_type_idx;
    VkResult vr;

    create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    create_info.pNext = NULL;
    create_info.flags = flags;
    create_info.imageType = vk_image_type;
    create_info.format = vk_format;
    create_info.extent.width = width;
    create_info.extent.height = height;
    create_info.extent.depth = depth;
    create_info.mipLevels = mip_levels;
    create_info.arrayLayers = layer_count;
    create_info.samples = sample_count;
    create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    create_info.usage = usage;
    create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    create_info.queueFamilyIndexCount = 0;
    create_info.pQueueFamilyIndices = NULL;
    create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    image->command_buffer_id = 0;

    vr = VK_CALL(vkCreateImage(device_vk->vk_device, &create_info, NULL, &image->vk_image));
    if (vr != VK_SUCCESS)
    {
        ERR("Failed to create image, vr %s.\n", wined3d_debug_vkresult(vr));
        image->vk_image = VK_NULL_HANDLE;
        return FALSE;
    }

    VK_CALL(vkGetImageMemoryRequirements(device_vk->vk_device, image->vk_image,
            &memory_requirements));

    memory_type_idx = wined3d_adapter_vk_get_memory_type_index(adapter_vk,
            memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (memory_type_idx == ~0u)
    {
        ERR("Failed to find suitable image memory type.\n");
        VK_CALL(vkDestroyImage(device_vk->vk_device, image->vk_image, NULL));
        image->vk_image = VK_NULL_HANDLE;
        return FALSE;
    }

    image->memory = wined3d_context_vk_allocate_memory(context_vk, memory_type_idx,
            memory_requirements.size, &image->vk_memory);
    if (!image->vk_memory)
    {
        ERR("Failed to allocate image memory.\n");
        VK_CALL(vkDestroyImage(device_vk->vk_device, image->vk_image, NULL));
        image->vk_image = VK_NULL_HANDLE;
        return FALSE;
    }

    vr = VK_CALL(vkBindImageMemory(device_vk->vk_device, image->vk_image, image->vk_memory,
            image->memory ? image->memory->offset : 0));
    if (vr != VK_SUCCESS)
    {
        VK_CALL(vkDestroyImage(device_vk->vk_device, image->vk_image, NULL));
        if (image->memory)
            wined3d_allocator_block_free(image->memory);
        else
            VK_CALL(vkFreeMemory(device_vk->vk_device, image->vk_memory, NULL));
        ERR("Failed to bind image memory, vr %s.\n", wined3d_debug_vkresult(vr));
        image->memory = NULL;
        image->vk_memory = VK_NULL_HANDLE;
        image->vk_image = VK_NULL_HANDLE;
        return FALSE;
    }

    return TRUE;
}

static struct wined3d_retired_object_vk *wined3d_context_vk_get_retired_object_vk(struct wined3d_context_vk *context_vk)
{
    struct wined3d_retired_objects_vk *retired = &context_vk->retired;
    struct wined3d_retired_object_vk *o;

    if (retired->free)
    {
        o = retired->free;
        retired->free = o->u.next;
        return o;
    }

    if (!wined3d_array_reserve((void **)&retired->objects, &retired->size,
            retired->count + 1, sizeof(*retired->objects)))
        return NULL;

    return &retired->objects[retired->count++];
}

void wined3d_context_vk_destroy_vk_framebuffer(struct wined3d_context_vk *context_vk,
        VkFramebuffer vk_framebuffer, uint64_t command_buffer_id)
{
    struct wined3d_device_vk *device_vk = wined3d_device_vk(context_vk->c.device);
    const struct wined3d_vk_info *vk_info = context_vk->vk_info;
    struct wined3d_retired_object_vk *o;

    if (context_vk->completed_command_buffer_id > command_buffer_id)
    {
        VK_CALL(vkDestroyFramebuffer(device_vk->vk_device, vk_framebuffer, NULL));
        TRACE("Destroyed framebuffer 0x%s.\n", wine_dbgstr_longlong(vk_framebuffer));
        return;
    }

    if (!(o = wined3d_context_vk_get_retired_object_vk(context_vk)))
    {
        ERR("Leaking framebuffer 0x%s.\n", wine_dbgstr_longlong(vk_framebuffer));
        return;
    }

    o->type = WINED3D_RETIRED_FRAMEBUFFER_VK;
    o->u.vk_framebuffer = vk_framebuffer;
    o->command_buffer_id = command_buffer_id;
}

static void wined3d_context_vk_destroy_vk_descriptor_pool(struct wined3d_context_vk *context_vk,
        VkDescriptorPool vk_descriptor_pool, uint64_t command_buffer_id)
{
    struct wined3d_device_vk *device_vk = wined3d_device_vk(context_vk->c.device);
    const struct wined3d_vk_info *vk_info = context_vk->vk_info;
    struct wined3d_retired_object_vk *o;

    if (context_vk->completed_command_buffer_id > command_buffer_id)
    {
        VK_CALL(vkDestroyDescriptorPool(device_vk->vk_device, vk_descriptor_pool, NULL));
        TRACE("Destroyed descriptor pool 0x%s.\n", wine_dbgstr_longlong(vk_descriptor_pool));
        return;
    }

    if (!(o = wined3d_context_vk_get_retired_object_vk(context_vk)))
    {
        ERR("Leaking descriptor pool 0x%s.\n", wine_dbgstr_longlong(vk_descriptor_pool));
        return;
    }

    o->type = WINED3D_RETIRED_DESCRIPTOR_POOL_VK;
    o->u.vk_descriptor_pool = vk_descriptor_pool;
    o->command_buffer_id = command_buffer_id;
}

void wined3d_context_vk_destroy_vk_memory(struct wined3d_context_vk *context_vk,
        VkDeviceMemory vk_memory, uint64_t command_buffer_id)
{
    struct wined3d_device_vk *device_vk = wined3d_device_vk(context_vk->c.device);
    const struct wined3d_vk_info *vk_info = context_vk->vk_info;
    struct wined3d_retired_object_vk *o;

    if (context_vk->completed_command_buffer_id > command_buffer_id)
    {
        VK_CALL(vkFreeMemory(device_vk->vk_device, vk_memory, NULL));
        TRACE("Freed memory 0x%s.\n", wine_dbgstr_longlong(vk_memory));
        return;
    }

    if (!(o = wined3d_context_vk_get_retired_object_vk(context_vk)))
    {
        ERR("Leaking memory 0x%s.\n", wine_dbgstr_longlong(vk_memory));
        return;
    }

    o->type = WINED3D_RETIRED_MEMORY_VK;
    o->u.vk_memory = vk_memory;
    o->command_buffer_id = command_buffer_id;
}

void wined3d_context_vk_destroy_allocator_block(struct wined3d_context_vk *context_vk,
        struct wined3d_allocator_block *block, uint64_t command_buffer_id)
{
    struct wined3d_retired_object_vk *o;

    if (context_vk->completed_command_buffer_id > command_buffer_id)
    {
        wined3d_allocator_block_free(block);
        TRACE("Freed block %p.\n", block);
        return;
    }

    if (!(o = wined3d_context_vk_get_retired_object_vk(context_vk)))
    {
        ERR("Leaking block %p.\n", block);
        return;
    }

    o->type = WINED3D_RETIRED_ALLOCATOR_BLOCK_VK;
    o->u.block = block;
    o->command_buffer_id = command_buffer_id;
}

static void wined3d_bo_slab_vk_free_slice(struct wined3d_bo_slab_vk *slab,
        SIZE_T idx, struct wined3d_context_vk *context_vk)
{
    struct wined3d_bo_slab_vk_key key;
    struct wine_rb_entry *entry;

    TRACE("slab %p, idx %lu, context_vk %p.\n", slab, idx, context_vk);

    if (!slab->map)
    {
        key.memory_type = slab->requested_memory_type;
        key.usage = slab->bo.usage;
        key.size = slab->bo.size;

        if ((entry = wine_rb_get(&context_vk->bo_slab_available, &key)))
        {
            slab->next = WINE_RB_ENTRY_VALUE(entry, struct wined3d_bo_slab_vk, entry);
            wine_rb_replace(&context_vk->bo_slab_available, entry, &slab->entry);
        }
        else if (wine_rb_put(&context_vk->bo_slab_available, &key, &slab->entry) < 0)
        {
            ERR("Unable to return slab %p (map 0x%08x) to available tree.\n", slab, slab->map);
        }
    }
    slab->map |= 1u << idx;
}

static void wined3d_context_vk_destroy_bo_slab_slice(struct wined3d_context_vk *context_vk,
        struct wined3d_bo_slab_vk *slab, SIZE_T idx, uint64_t command_buffer_id)
{
    struct wined3d_retired_object_vk *o;

    if (context_vk->completed_command_buffer_id > command_buffer_id)
    {
        wined3d_bo_slab_vk_free_slice(slab, idx, context_vk);
        return;
    }

    if (!(o = wined3d_context_vk_get_retired_object_vk(context_vk)))
    {
        ERR("Leaking slab %p, slice %#lx.\n", slab, idx);
        return;
    }

    o->type = WINED3D_RETIRED_BO_SLAB_SLICE_VK;
    o->u.slice.slab = slab;
    o->u.slice.idx = idx;
    o->command_buffer_id = command_buffer_id;
}

static void wined3d_context_vk_destroy_vk_buffer(struct wined3d_context_vk *context_vk,
        VkBuffer vk_buffer, uint64_t command_buffer_id)
{
    struct wined3d_device_vk *device_vk = wined3d_device_vk(context_vk->c.device);
    const struct wined3d_vk_info *vk_info = context_vk->vk_info;
    struct wined3d_retired_object_vk *o;

    if (context_vk->completed_command_buffer_id > command_buffer_id)
    {
        VK_CALL(vkDestroyBuffer(device_vk->vk_device, vk_buffer, NULL));
        TRACE("Destroyed buffer 0x%s.\n", wine_dbgstr_longlong(vk_buffer));
        return;
    }

    if (!(o = wined3d_context_vk_get_retired_object_vk(context_vk)))
    {
        ERR("Leaking buffer 0x%s.\n", wine_dbgstr_longlong(vk_buffer));
        return;
    }

    o->type = WINED3D_RETIRED_BUFFER_VK;
    o->u.vk_buffer = vk_buffer;
    o->command_buffer_id = command_buffer_id;
}

void wined3d_context_vk_destroy_vk_image(struct wined3d_context_vk *context_vk,
        VkImage vk_image, uint64_t command_buffer_id)
{
    struct wined3d_device_vk *device_vk = wined3d_device_vk(context_vk->c.device);
    const struct wined3d_vk_info *vk_info = context_vk->vk_info;
    struct wined3d_retired_object_vk *o;

    if (context_vk->completed_command_buffer_id > command_buffer_id)
    {
        VK_CALL(vkDestroyImage(device_vk->vk_device, vk_image, NULL));
        TRACE("Destroyed image 0x%s.\n", wine_dbgstr_longlong(vk_image));
        return;
    }

    if (!(o = wined3d_context_vk_get_retired_object_vk(context_vk)))
    {
        ERR("Leaking image 0x%s.\n", wine_dbgstr_longlong(vk_image));
        return;
    }

    o->type = WINED3D_RETIRED_IMAGE_VK;
    o->u.vk_image = vk_image;
    o->command_buffer_id = command_buffer_id;
}

void wined3d_context_vk_destroy_vk_buffer_view(struct wined3d_context_vk *context_vk,
        VkBufferView vk_view, uint64_t command_buffer_id)
{
    struct wined3d_device_vk *device_vk = wined3d_device_vk(context_vk->c.device);
    const struct wined3d_vk_info *vk_info = context_vk->vk_info;
    struct wined3d_retired_object_vk *o;

    if (context_vk->completed_command_buffer_id > command_buffer_id)
    {
        VK_CALL(vkDestroyBufferView(device_vk->vk_device, vk_view, NULL));
        TRACE("Destroyed buffer view 0x%s.\n", wine_dbgstr_longlong(vk_view));
        return;
    }

    if (!(o = wined3d_context_vk_get_retired_object_vk(context_vk)))
    {
        ERR("Leaking buffer view 0x%s.\n", wine_dbgstr_longlong(vk_view));
        return;
    }

    o->type = WINED3D_RETIRED_BUFFER_VIEW_VK;
    o->u.vk_buffer_view = vk_view;
    o->command_buffer_id = command_buffer_id;
}

void wined3d_context_vk_destroy_vk_image_view(struct wined3d_context_vk *context_vk,
        VkImageView vk_view, uint64_t command_buffer_id)
{
    struct wined3d_device_vk *device_vk = wined3d_device_vk(context_vk->c.device);
    const struct wined3d_vk_info *vk_info = context_vk->vk_info;
    struct wined3d_retired_object_vk *o;

    if (context_vk->completed_command_buffer_id > command_buffer_id)
    {
        VK_CALL(vkDestroyImageView(device_vk->vk_device, vk_view, NULL));
        TRACE("Destroyed image view 0x%s.\n", wine_dbgstr_longlong(vk_view));
        return;
    }

    if (!(o = wined3d_context_vk_get_retired_object_vk(context_vk)))
    {
        ERR("Leaking image view 0x%s.\n", wine_dbgstr_longlong(vk_view));
        return;
    }

    o->type = WINED3D_RETIRED_IMAGE_VIEW_VK;
    o->u.vk_image_view = vk_view;
    o->command_buffer_id = command_buffer_id;
}

void wined3d_context_vk_destroy_vk_sampler(struct wined3d_context_vk *context_vk,
        VkSampler vk_sampler, uint64_t command_buffer_id)
{
    struct wined3d_device_vk *device_vk = wined3d_device_vk(context_vk->c.device);
    const struct wined3d_vk_info *vk_info = context_vk->vk_info;
    struct wined3d_retired_object_vk *o;

    if (context_vk->completed_command_buffer_id > command_buffer_id)
    {
        VK_CALL(vkDestroySampler(device_vk->vk_device, vk_sampler, NULL));
        TRACE("Destroyed sampler 0x%s.\n", wine_dbgstr_longlong(vk_sampler));
        return;
    }

    if (!(o = wined3d_context_vk_get_retired_object_vk(context_vk)))
    {
        ERR("Leaking sampler 0x%s.\n", wine_dbgstr_longlong(vk_sampler));
        return;
    }

    o->type = WINED3D_RETIRED_SAMPLER_VK;
    o->u.vk_sampler = vk_sampler;
    o->command_buffer_id = command_buffer_id;
}

void wined3d_context_vk_destroy_image(struct wined3d_context_vk *context_vk, struct wined3d_image_vk *image)
{
    wined3d_context_vk_destroy_vk_image(context_vk, image->vk_image, image->command_buffer_id);
    if (image->memory)
        wined3d_context_vk_destroy_allocator_block(context_vk, image->memory,
                image->command_buffer_id);
    else
        wined3d_context_vk_destroy_vk_memory(context_vk, image->vk_memory, image->command_buffer_id);

    image->vk_image = VK_NULL_HANDLE;
    image->vk_memory = VK_NULL_HANDLE;
    image->memory = NULL;
}

void wined3d_context_vk_destroy_bo(struct wined3d_context_vk *context_vk, const struct wined3d_bo_vk *bo)
{
    struct wined3d_device_vk *device_vk = wined3d_device_vk(context_vk->c.device);
    const struct wined3d_vk_info *vk_info = context_vk->vk_info;
    struct wined3d_bo_slab_vk *slab_vk;
    size_t object_size, idx;

    TRACE("context_vk %p, bo %p.\n", context_vk, bo);

    if ((slab_vk = bo->slab))
    {
        if (bo->map_ptr)
            wined3d_bo_slab_vk_unmap(slab_vk, context_vk);
        object_size = slab_vk->bo.size / 32;
        idx = bo->buffer_offset / object_size;
        wined3d_context_vk_destroy_bo_slab_slice(context_vk, slab_vk, idx, bo->command_buffer_id);
        return;
    }

    wined3d_context_vk_destroy_vk_buffer(context_vk, bo->vk_buffer, bo->command_buffer_id);
    if (bo->memory)
    {
        if (bo->map_ptr)
            wined3d_allocator_chunk_vk_unmap(wined3d_allocator_chunk_vk(bo->memory->chunk), context_vk);
        wined3d_context_vk_destroy_allocator_block(context_vk, bo->memory, bo->command_buffer_id);
        return;
    }

    if (bo->map_ptr)
        VK_CALL(vkUnmapMemory(device_vk->vk_device, bo->vk_memory));
    wined3d_context_vk_destroy_vk_memory(context_vk, bo->vk_memory, bo->command_buffer_id);
}

void wined3d_context_vk_poll_command_buffers(struct wined3d_context_vk *context_vk)
{
    struct wined3d_device_vk *device_vk = wined3d_device_vk(context_vk->c.device);
    const struct wined3d_vk_info *vk_info = context_vk->vk_info;
    struct wined3d_command_buffer_vk *buffer;
    SIZE_T i = 0;

    while (i < context_vk->submitted.buffer_count)
    {
        buffer = &context_vk->submitted.buffers[i];
        if (VK_CALL(vkGetFenceStatus(device_vk->vk_device, buffer->vk_fence)) == VK_NOT_READY)
        {
            ++i;
            continue;
        }

        TRACE("Command buffer %p with id 0x%s has finished.\n",
                buffer->vk_command_buffer, wine_dbgstr_longlong(buffer->id));
        VK_CALL(vkDestroyFence(device_vk->vk_device, buffer->vk_fence, NULL));
        VK_CALL(vkFreeCommandBuffers(device_vk->vk_device,
                context_vk->vk_command_pool, 1, &buffer->vk_command_buffer));

        if (buffer->id > context_vk->completed_command_buffer_id)
            context_vk->completed_command_buffer_id = buffer->id;
        *buffer = context_vk->submitted.buffers[--context_vk->submitted.buffer_count];
    }
}

static void wined3d_context_vk_cleanup_resources(struct wined3d_context_vk *context_vk)
{
    struct wined3d_device_vk *device_vk = wined3d_device_vk(context_vk->c.device);
    struct wined3d_retired_objects_vk *retired = &context_vk->retired;
    const struct wined3d_vk_info *vk_info = context_vk->vk_info;
    struct wined3d_retired_object_vk *o;
    uint64_t command_buffer_id;
    SIZE_T i = 0;

    wined3d_context_vk_poll_command_buffers(context_vk);
    command_buffer_id = context_vk->completed_command_buffer_id;

    retired->free = NULL;
    for (i = retired->count; i; --i)
    {
        o = &retired->objects[i - 1];

        if (o->type != WINED3D_RETIRED_FREE_VK && o->command_buffer_id > command_buffer_id)
            continue;

        switch (o->type)
        {
            case WINED3D_RETIRED_FREE_VK:
                /* Nothing to do. */
                break;

            case WINED3D_RETIRED_FRAMEBUFFER_VK:
                VK_CALL(vkDestroyFramebuffer(device_vk->vk_device, o->u.vk_framebuffer, NULL));
                TRACE("Destroyed framebuffer 0x%s.\n", wine_dbgstr_longlong(o->u.vk_framebuffer));
                break;

            case WINED3D_RETIRED_DESCRIPTOR_POOL_VK:
                VK_CALL(vkDestroyDescriptorPool(device_vk->vk_device, o->u.vk_descriptor_pool, NULL));
                TRACE("Destroyed descriptor pool 0x%s.\n", wine_dbgstr_longlong(o->u.vk_descriptor_pool));
                break;

            case WINED3D_RETIRED_MEMORY_VK:
                VK_CALL(vkFreeMemory(device_vk->vk_device, o->u.vk_memory, NULL));
                TRACE("Freed memory 0x%s.\n", wine_dbgstr_longlong(o->u.vk_memory));
                break;

            case WINED3D_RETIRED_ALLOCATOR_BLOCK_VK:
                TRACE("Destroying block %p.\n", o->u.block);
                wined3d_allocator_block_free(o->u.block);
                break;

            case WINED3D_RETIRED_BO_SLAB_SLICE_VK:
                wined3d_bo_slab_vk_free_slice(o->u.slice.slab, o->u.slice.idx, context_vk);
                break;

            case WINED3D_RETIRED_BUFFER_VK:
                VK_CALL(vkDestroyBuffer(device_vk->vk_device, o->u.vk_buffer, NULL));
                TRACE("Destroyed buffer 0x%s.\n", wine_dbgstr_longlong(o->u.vk_buffer));
                break;

            case WINED3D_RETIRED_IMAGE_VK:
                VK_CALL(vkDestroyImage(device_vk->vk_device, o->u.vk_image, NULL));
                TRACE("Destroyed image 0x%s.\n", wine_dbgstr_longlong(o->u.vk_image));
                break;

            case WINED3D_RETIRED_BUFFER_VIEW_VK:
                VK_CALL(vkDestroyBufferView(device_vk->vk_device, o->u.vk_buffer_view, NULL));
                TRACE("Destroyed buffer view 0x%s.\n", wine_dbgstr_longlong(o->u.vk_buffer_view));
                break;

            case WINED3D_RETIRED_IMAGE_VIEW_VK:
                VK_CALL(vkDestroyImageView(device_vk->vk_device, o->u.vk_image_view, NULL));
                TRACE("Destroyed image view 0x%s.\n", wine_dbgstr_longlong(o->u.vk_image_view));
                break;

            case WINED3D_RETIRED_SAMPLER_VK:
                VK_CALL(vkDestroySampler(device_vk->vk_device, o->u.vk_sampler, NULL));
                TRACE("Destroyed sampler 0x%s.\n", wine_dbgstr_longlong(o->u.vk_sampler));
                break;

            default:
                ERR("Unhandled object type %#x.\n", o->type);
                break;
        }

        if (i == retired->count)
        {
            --retired->count;
            continue;
        }

        o->type = WINED3D_RETIRED_FREE_VK;
        o->u.next = retired->free;
        retired->free = o;
    }
}

static void wined3d_context_vk_destroy_bo_slab(struct wine_rb_entry *entry, void *ctx)
{
    struct wined3d_context_vk *context_vk = ctx;
    struct wined3d_bo_slab_vk *slab, *next;

    slab = WINE_RB_ENTRY_VALUE(entry, struct wined3d_bo_slab_vk, entry);
    while (slab)
    {
        next = slab->next;
        wined3d_context_vk_destroy_bo(context_vk, &slab->bo);
        heap_free(slab);
        slab = next;
    }
}

static void wined3d_context_vk_destroy_graphics_pipeline(struct wine_rb_entry *entry, void *ctx)
{
    struct wined3d_graphics_pipeline_vk *pipeline_vk = WINE_RB_ENTRY_VALUE(entry,
            struct wined3d_graphics_pipeline_vk, entry);
    struct wined3d_context_vk *context_vk = ctx;
    const struct wined3d_vk_info *vk_info;
    struct wined3d_device_vk *device_vk;

    vk_info = context_vk->vk_info;
    device_vk = wined3d_device_vk(context_vk->c.device);

    VK_CALL(vkDestroyPipeline(device_vk->vk_device, pipeline_vk->vk_pipeline, NULL));
    heap_free(pipeline_vk);
}

static void wined3d_context_vk_destroy_pipeline_layout(struct wine_rb_entry *entry, void *ctx)
{
    struct wined3d_pipeline_layout_vk *layout = WINE_RB_ENTRY_VALUE(entry,
            struct wined3d_pipeline_layout_vk, entry);
    struct wined3d_context_vk *context_vk = ctx;
    const struct wined3d_vk_info *vk_info;
    struct wined3d_device_vk *device_vk;

    vk_info = context_vk->vk_info;
    device_vk = wined3d_device_vk(context_vk->c.device);

    VK_CALL(vkDestroyPipelineLayout(device_vk->vk_device, layout->vk_pipeline_layout, NULL));
    VK_CALL(vkDestroyDescriptorSetLayout(device_vk->vk_device, layout->vk_set_layout, NULL));
    heap_free(layout->key.bindings);
    heap_free(layout);
}

static void wined3d_render_pass_key_vk_init(struct wined3d_render_pass_key_vk *key,
        const struct wined3d_fb_state *fb, unsigned int rt_count, bool depth_stencil, uint32_t clear_flags)
{
    struct wined3d_render_pass_attachment_vk *a;
    struct wined3d_rendertarget_view *view;
    unsigned int i;

    memset(key, 0, sizeof(*key));

    for (i = 0; i < rt_count; ++i)
    {
        if (!(view = fb->render_targets[i]) || view->format->id == WINED3DFMT_NULL)
            continue;

        a = &key->rt[i];
        a->vk_format = wined3d_format_vk(view->format)->vk_format;
        a->vk_samples = max(1, wined3d_resource_get_sample_count(view->resource));
        a->vk_layout = wined3d_texture_vk(wined3d_texture_from_resource(view->resource))->layout;
        key->rt_mask |= 1u << i;
    }

    if (depth_stencil && (view = fb->depth_stencil))
    {
        a = &key->ds;
        a->vk_format = wined3d_format_vk(view->format)->vk_format;
        a->vk_samples = max(1, wined3d_resource_get_sample_count(view->resource));
        a->vk_layout = wined3d_texture_vk(wined3d_texture_from_resource(view->resource))->layout;
        key->rt_mask |= 1u << WINED3D_MAX_RENDER_TARGETS;
    }

    key->clear_flags = clear_flags;
}

static void wined3d_render_pass_vk_cleanup(struct wined3d_render_pass_vk *pass,
        struct wined3d_context_vk *context_vk)
{
    struct wined3d_device_vk *device_vk = wined3d_device_vk(context_vk->c.device);
    const struct wined3d_vk_info *vk_info = context_vk->vk_info;

    VK_CALL(vkDestroyRenderPass(device_vk->vk_device, pass->vk_render_pass, NULL));
}

static bool wined3d_render_pass_vk_init(struct wined3d_render_pass_vk *pass,
        struct wined3d_context_vk *context_vk, const struct wined3d_render_pass_key_vk *key)
{
    struct wined3d_device_vk *device_vk = wined3d_device_vk(context_vk->c.device);
    VkAttachmentReference attachment_references[WINED3D_MAX_RENDER_TARGETS];
    VkAttachmentDescription attachments[WINED3D_MAX_RENDER_TARGETS + 1];
    const struct wined3d_vk_info *vk_info = context_vk->vk_info;
    const struct wined3d_render_pass_attachment_vk *a;
    VkAttachmentReference ds_attachment_reference;
    VkAttachmentReference *ds_reference = NULL;
    unsigned int attachment_count, rt_count, i;
    VkAttachmentDescription *attachment;
    VkSubpassDescription sub_pass_desc;
    VkRenderPassCreateInfo pass_desc;
    uint32_t mask;
    VkResult vr;

    rt_count = 0;
    attachment_count = 0;
    mask = key->rt_mask & ((1u << WINED3D_MAX_RENDER_TARGETS) - 1);
    while (mask)
    {
        i = wined3d_bit_scan(&mask);
        a = &key->rt[i];

        attachment = &attachments[attachment_count];
        attachment->flags = 0;
        attachment->format = a->vk_format;
        attachment->samples = a->vk_samples;
        if (key->clear_flags & WINED3DCLEAR_TARGET)
            attachment->loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        else
            attachment->loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        attachment->storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachment->stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachment->stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachment->initialLayout = a->vk_layout;
        attachment->finalLayout = a->vk_layout;

        attachment_references[i].attachment = attachment_count;
        attachment_references[i].layout = a->vk_layout;

        ++attachment_count;
        rt_count = i + 1;
    }

    mask = ~key->rt_mask & ((1u << rt_count) - 1);
    while (mask)
    {
        i = wined3d_bit_scan(&mask);
        attachment_references[i].attachment = VK_ATTACHMENT_UNUSED;
        attachment_references[i].layout = VK_IMAGE_LAYOUT_UNDEFINED;
    }

    if (key->rt_mask & (1u << WINED3D_MAX_RENDER_TARGETS))
    {
        a = &key->ds;

        attachment = &attachments[attachment_count];
        attachment->flags = 0;
        attachment->format = a->vk_format;
        attachment->samples = a->vk_samples;
        if (key->clear_flags & WINED3DCLEAR_ZBUFFER)
            attachment->loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        else
            attachment->loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        attachment->storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        if (key->clear_flags & WINED3DCLEAR_STENCIL)
            attachment->stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        else
            attachment->stencilLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        attachment->stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachment->initialLayout = a->vk_layout;
        attachment->finalLayout = a->vk_layout;

        ds_reference = &ds_attachment_reference;
        ds_reference->attachment = attachment_count;
        ds_reference->layout = a->vk_layout;

        ++attachment_count;
    }

    sub_pass_desc.flags = 0;
    sub_pass_desc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    sub_pass_desc.inputAttachmentCount = 0;
    sub_pass_desc.pInputAttachments = NULL;
    sub_pass_desc.colorAttachmentCount = rt_count;
    sub_pass_desc.pColorAttachments = attachment_references;
    sub_pass_desc.pResolveAttachments = NULL;
    sub_pass_desc.pDepthStencilAttachment = ds_reference;
    sub_pass_desc.preserveAttachmentCount = 0;
    sub_pass_desc.pPreserveAttachments = NULL;

    pass_desc.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    pass_desc.pNext = NULL;
    pass_desc.flags = 0;
    pass_desc.attachmentCount = attachment_count;
    pass_desc.pAttachments = attachments;
    pass_desc.subpassCount = 1;
    pass_desc.pSubpasses = &sub_pass_desc;
    pass_desc.dependencyCount = 0;
    pass_desc.pDependencies = NULL;

    pass->key = *key;
    if ((vr = VK_CALL(vkCreateRenderPass(device_vk->vk_device,
            &pass_desc, NULL, &pass->vk_render_pass))) < 0)
    {
        WARN("Failed to create Vulkan render pass, vr %d.\n", vr);
        return false;
    }

    return true;
}

VkRenderPass wined3d_context_vk_get_render_pass(struct wined3d_context_vk *context_vk,
        const struct wined3d_fb_state *fb, unsigned int rt_count, bool depth_stencil, uint32_t clear_flags)
{
    struct wined3d_render_pass_key_vk key;
    struct wined3d_render_pass_vk *pass;
    struct wine_rb_entry *entry;

    wined3d_render_pass_key_vk_init(&key, fb, rt_count, depth_stencil, clear_flags);
    if ((entry = wine_rb_get(&context_vk->render_passes, &key)))
        return WINE_RB_ENTRY_VALUE(entry, struct wined3d_render_pass_vk, entry)->vk_render_pass;

    if (!(pass = heap_alloc(sizeof(*pass))))
        return VK_NULL_HANDLE;

    if (!wined3d_render_pass_vk_init(pass, context_vk, &key))
    {
        heap_free(pass);
        return VK_NULL_HANDLE;
    }

    if (wine_rb_put(&context_vk->render_passes, &pass->key, &pass->entry) == -1)
    {
        ERR("Failed to insert render pass.\n");
        wined3d_render_pass_vk_cleanup(pass, context_vk);
        heap_free(pass);
        return VK_NULL_HANDLE;
    }

    return pass->vk_render_pass;
}

void wined3d_context_vk_end_current_render_pass(struct wined3d_context_vk *context_vk)
{
    const struct wined3d_vk_info *vk_info = context_vk->vk_info;
    VkCommandBuffer vk_command_buffer;

    if (context_vk->vk_render_pass)
    {
        vk_command_buffer = context_vk->current_command_buffer.vk_command_buffer;
        VK_CALL(vkCmdEndRenderPass(vk_command_buffer));
        context_vk->vk_render_pass = VK_NULL_HANDLE;
        VK_CALL(vkCmdPipelineBarrier(vk_command_buffer, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, NULL, 0, NULL, 0, NULL));
    }

    if (context_vk->vk_framebuffer)
    {
        wined3d_context_vk_destroy_vk_framebuffer(context_vk,
                context_vk->vk_framebuffer, context_vk->current_command_buffer.id);
        context_vk->vk_framebuffer = VK_NULL_HANDLE;
    }
}

static void wined3d_context_vk_destroy_render_pass(struct wine_rb_entry *entry, void *ctx)
{
    struct wined3d_render_pass_vk *pass = WINE_RB_ENTRY_VALUE(entry,
            struct wined3d_render_pass_vk, entry);

    wined3d_render_pass_vk_cleanup(pass, ctx);
    heap_free(pass);
}

static void wined3d_shader_descriptor_writes_vk_cleanup(struct wined3d_shader_descriptor_writes_vk *writes)
{
    heap_free(writes->writes);
}

static void wined3d_context_vk_destroy_query_pools(struct wined3d_context_vk *context_vk, struct list *free_pools)
{
    struct wined3d_query_pool_vk *pool_vk, *entry;

    LIST_FOR_EACH_ENTRY_SAFE(pool_vk, entry, free_pools, struct wined3d_query_pool_vk, entry)
    {
        wined3d_query_pool_vk_cleanup(pool_vk, context_vk);
        heap_free(pool_vk);
    }
}

bool wined3d_context_vk_allocate_query(struct wined3d_context_vk *context_vk,
        enum wined3d_query_type type, struct wined3d_query_pool_idx_vk *pool_idx)
{
    struct wined3d_query_pool_vk *pool_vk, *entry;
    struct list *free_pools;
    size_t idx;

    switch (type)
    {
        case WINED3D_QUERY_TYPE_OCCLUSION:
            free_pools = &context_vk->free_occlusion_query_pools;
            break;

        case WINED3D_QUERY_TYPE_TIMESTAMP:
            free_pools = &context_vk->free_timestamp_query_pools;
            break;

        case WINED3D_QUERY_TYPE_PIPELINE_STATISTICS:
            free_pools = &context_vk->free_pipeline_statistics_query_pools;
            break;

        case WINED3D_QUERY_TYPE_SO_STATISTICS:
        case WINED3D_QUERY_TYPE_SO_STATISTICS_STREAM0:
        case WINED3D_QUERY_TYPE_SO_STATISTICS_STREAM1:
        case WINED3D_QUERY_TYPE_SO_STATISTICS_STREAM2:
        case WINED3D_QUERY_TYPE_SO_STATISTICS_STREAM3:
            free_pools = &context_vk->free_stream_output_statistics_query_pools;
            break;

        default:
            FIXME("Unhandled query type %#x.\n", type);
            return false;
    }

    LIST_FOR_EACH_ENTRY_SAFE(pool_vk, entry, free_pools, struct wined3d_query_pool_vk, entry)
    {
        if (wined3d_query_pool_vk_allocate_query(pool_vk, &idx))
            goto done;
        list_remove(&pool_vk->entry);
    }

    if (!(pool_vk = heap_alloc_zero(sizeof(*pool_vk))))
        return false;
    if (!wined3d_query_pool_vk_init(pool_vk, context_vk, type, free_pools))
    {
        heap_free(pool_vk);
        return false;
    }

    if (!wined3d_query_pool_vk_allocate_query(pool_vk, &idx))
    {
        wined3d_query_pool_vk_cleanup(pool_vk, context_vk);
        heap_free(pool_vk);
        return false;
    }

done:
    pool_idx->pool_vk = pool_vk;
    pool_idx->idx = idx;

    return true;
}

void wined3d_context_vk_cleanup(struct wined3d_context_vk *context_vk)
{
    struct wined3d_command_buffer_vk *buffer = &context_vk->current_command_buffer;
    struct wined3d_device_vk *device_vk = wined3d_device_vk(context_vk->c.device);
    const struct wined3d_vk_info *vk_info = context_vk->vk_info;

    if (buffer->vk_command_buffer)
    {
        VK_CALL(vkFreeCommandBuffers(device_vk->vk_device,
                context_vk->vk_command_pool, 1, &buffer->vk_command_buffer));
        buffer->vk_command_buffer = VK_NULL_HANDLE;
    }

    wined3d_context_vk_wait_command_buffer(context_vk, buffer->id - 1);
    context_vk->completed_command_buffer_id = buffer->id;

    heap_free(context_vk->compute.bindings.bindings);
    heap_free(context_vk->graphics.bindings.bindings);
    if (context_vk->vk_descriptor_pool)
        VK_CALL(vkDestroyDescriptorPool(device_vk->vk_device, context_vk->vk_descriptor_pool, NULL));
    if (context_vk->vk_framebuffer)
        VK_CALL(vkDestroyFramebuffer(device_vk->vk_device, context_vk->vk_framebuffer, NULL));
    VK_CALL(vkDestroyCommandPool(device_vk->vk_device, context_vk->vk_command_pool, NULL));
    if (context_vk->vk_so_counter_bo.vk_buffer)
        wined3d_context_vk_destroy_bo(context_vk, &context_vk->vk_so_counter_bo);
    wined3d_context_vk_cleanup_resources(context_vk);
    wined3d_context_vk_destroy_query_pools(context_vk, &context_vk->free_occlusion_query_pools);
    wined3d_context_vk_destroy_query_pools(context_vk, &context_vk->free_timestamp_query_pools);
    wined3d_context_vk_destroy_query_pools(context_vk, &context_vk->free_pipeline_statistics_query_pools);
    wined3d_context_vk_destroy_query_pools(context_vk, &context_vk->free_stream_output_statistics_query_pools);
    wine_rb_destroy(&context_vk->bo_slab_available, wined3d_context_vk_destroy_bo_slab, context_vk);
    heap_free(context_vk->pending_queries.queries);
    heap_free(context_vk->submitted.buffers);
    heap_free(context_vk->retired.objects);

    wined3d_shader_descriptor_writes_vk_cleanup(&context_vk->descriptor_writes);
    wine_rb_destroy(&context_vk->graphics_pipelines, wined3d_context_vk_destroy_graphics_pipeline, context_vk);
    wine_rb_destroy(&context_vk->pipeline_layouts, wined3d_context_vk_destroy_pipeline_layout, context_vk);
    wine_rb_destroy(&context_vk->render_passes, wined3d_context_vk_destroy_render_pass, context_vk);

    wined3d_context_cleanup(&context_vk->c);
}

void wined3d_context_vk_remove_pending_queries(struct wined3d_context_vk *context_vk,
        struct wined3d_query_vk *query_vk)
{
    struct wined3d_pending_queries_vk *pending = &context_vk->pending_queries;
    struct wined3d_pending_query_vk *p;
    size_t i;

    pending->free_idx = ~(size_t)0;
    for (i = pending->count; i; --i)
    {
        p = &pending->queries[i - 1];

        if (p->query_vk)
        {
            if (p->query_vk != query_vk && !wined3d_query_vk_accumulate_data(p->query_vk, context_vk, &p->pool_idx))
                continue;
            wined3d_query_pool_vk_free_query(p->pool_idx.pool_vk, p->pool_idx.idx);
            --p->query_vk->pending_count;
        }

        if (i == pending->count)
        {
            --pending->count;
            continue;
        }

        p->query_vk = NULL;
        p->pool_idx.pool_vk = NULL;
        p->pool_idx.idx = pending->free_idx;
        pending->free_idx = i - 1;
    }
}

void wined3d_context_vk_accumulate_pending_queries(struct wined3d_context_vk *context_vk)
{
    wined3d_context_vk_remove_pending_queries(context_vk, NULL);
}

void wined3d_context_vk_add_pending_query(struct wined3d_context_vk *context_vk, struct wined3d_query_vk *query_vk)
{
    struct wined3d_pending_queries_vk *pending = &context_vk->pending_queries;
    struct wined3d_pending_query_vk *p;

    if (pending->free_idx != ~(size_t)0)
    {
        p = &pending->queries[pending->free_idx];
        pending->free_idx = p->pool_idx.idx;
    }
    else
    {
        if (!wined3d_array_reserve((void **)&pending->queries, &pending->size,
                pending->count + 1, sizeof(*pending->queries)))
        {
            ERR("Failed to allocate entry.\n");
            return;
        }
        p = &pending->queries[pending->count++];
    }

    p->query_vk = query_vk;
    p->pool_idx = query_vk->pool_idx;
    ++query_vk->pending_count;
}

VkCommandBuffer wined3d_context_vk_get_command_buffer(struct wined3d_context_vk *context_vk)
{
    struct wined3d_device_vk *device_vk = wined3d_device_vk(context_vk->c.device);
    const struct wined3d_vk_info *vk_info = context_vk->vk_info;
    VkCommandBufferAllocateInfo command_buffer_info;
    struct wined3d_command_buffer_vk *buffer;
    VkCommandBufferBeginInfo begin_info;
    struct wined3d_query_vk *query_vk;
    VkResult vr;

    TRACE("context_vk %p.\n", context_vk);

    buffer = &context_vk->current_command_buffer;
    if (buffer->vk_command_buffer)
    {
        TRACE("Returning existing command buffer %p with id 0x%s.\n",
                buffer->vk_command_buffer, wine_dbgstr_longlong(buffer->id));
        return buffer->vk_command_buffer;
    }

    command_buffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_buffer_info.pNext = NULL;
    command_buffer_info.commandPool = context_vk->vk_command_pool;
    command_buffer_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    command_buffer_info.commandBufferCount = 1;
    if ((vr = VK_CALL(vkAllocateCommandBuffers(device_vk->vk_device,
            &command_buffer_info, &buffer->vk_command_buffer))) < 0)
    {
        WARN("Failed to allocate Vulkan command buffer, vr %s.\n", wined3d_debug_vkresult(vr));
        return VK_NULL_HANDLE;
    }

    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.pNext = NULL;
    begin_info.flags = 0;
    begin_info.pInheritanceInfo = NULL;
    if ((vr = VK_CALL(vkBeginCommandBuffer(buffer->vk_command_buffer, &begin_info))) < 0)
    {
        WARN("Failed to begin command buffer, vr %s.\n", wined3d_debug_vkresult(vr));
        VK_CALL(vkFreeCommandBuffers(device_vk->vk_device, context_vk->vk_command_pool,
                1, &buffer->vk_command_buffer));
        return buffer->vk_command_buffer = VK_NULL_HANDLE;
    }

    wined3d_context_vk_accumulate_pending_queries(context_vk);
    LIST_FOR_EACH_ENTRY(query_vk, &context_vk->active_queries, struct wined3d_query_vk, entry)
    {
        wined3d_query_vk_resume(query_vk, context_vk);
    }

    TRACE("Created new command buffer %p with id 0x%s.\n",
            buffer->vk_command_buffer, wine_dbgstr_longlong(buffer->id));

    return buffer->vk_command_buffer;
}

void wined3d_context_vk_submit_command_buffer(struct wined3d_context_vk *context_vk,
        unsigned int wait_semaphore_count, const VkSemaphore *wait_semaphores, const VkPipelineStageFlags *wait_stages,
        unsigned int signal_semaphore_count, const VkSemaphore *signal_semaphores)
{
    struct wined3d_device_vk *device_vk = wined3d_device_vk(context_vk->c.device);
    const struct wined3d_vk_info *vk_info = context_vk->vk_info;
    struct wined3d_command_buffer_vk *buffer;
    struct wined3d_query_vk *query_vk;
    VkFenceCreateInfo fence_desc;
    VkSubmitInfo submit_info;
    VkResult vr;

    TRACE("context_vk %p, wait_semaphore_count %u, wait_semaphores %p, wait_stages %p,"
            "signal_semaphore_count %u, signal_semaphores %p.\n",
            context_vk, wait_semaphore_count, wait_semaphores, wait_stages,
            signal_semaphore_count, signal_semaphores);

    buffer = &context_vk->current_command_buffer;
    if (!buffer->vk_command_buffer)
        return;

    TRACE("Submitting command buffer %p with id 0x%s.\n",
            buffer->vk_command_buffer, wine_dbgstr_longlong(buffer->id));

    LIST_FOR_EACH_ENTRY(query_vk, &context_vk->active_queries, struct wined3d_query_vk, entry)
    {
        wined3d_query_vk_suspend(query_vk, context_vk);
    }

    wined3d_context_vk_end_current_render_pass(context_vk);
    context_vk->graphics.vk_pipeline = VK_NULL_HANDLE;
    context_vk->update_compute_pipeline = 1;
    context_vk->update_stream_output = 1;
    context_vk->c.update_shader_resource_bindings = 1;
    context_vk->c.update_compute_shader_resource_bindings = 1;
    context_vk->c.update_unordered_access_view_bindings = 1;
    context_vk->c.update_compute_unordered_access_view_bindings = 1;
    context_invalidate_state(&context_vk->c, STATE_STREAMSRC);
    context_invalidate_state(&context_vk->c, STATE_INDEXBUFFER);
    context_invalidate_state(&context_vk->c, STATE_BLEND_FACTOR);
    context_invalidate_state(&context_vk->c, STATE_STENCIL_REF);

    VK_CALL(vkEndCommandBuffer(buffer->vk_command_buffer));

    fence_desc.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_desc.pNext = NULL;
    fence_desc.flags = 0;
    if ((vr = VK_CALL(vkCreateFence(device_vk->vk_device, &fence_desc, NULL, &buffer->vk_fence))) < 0)
        ERR("Failed to create fence, vr %s.\n", wined3d_debug_vkresult(vr));

    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext = NULL;
    submit_info.waitSemaphoreCount = wait_semaphore_count;
    submit_info.pWaitSemaphores = wait_semaphores;
    submit_info.pWaitDstStageMask = wait_stages;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &buffer->vk_command_buffer;
    submit_info.signalSemaphoreCount = signal_semaphore_count;
    submit_info.pSignalSemaphores = signal_semaphores;

    if ((vr = VK_CALL(vkQueueSubmit(device_vk->vk_queue, 1, &submit_info, buffer->vk_fence))) < 0)
        ERR("Failed to submit command buffer %p, vr %s.\n",
                buffer->vk_command_buffer, wined3d_debug_vkresult(vr));

    if (!wined3d_array_reserve((void **)&context_vk->submitted.buffers, &context_vk->submitted.buffers_size,
            context_vk->submitted.buffer_count + 1, sizeof(*context_vk->submitted.buffers)))
        ERR("Failed to grow submitted command buffer array.\n");

    context_vk->submitted.buffers[context_vk->submitted.buffer_count++] = *buffer;

    buffer->vk_command_buffer = VK_NULL_HANDLE;
    /* We don't expect this to ever happen, but handle it anyway. */
    if (!++buffer->id)
    {
        wined3d_context_vk_wait_command_buffer(context_vk, buffer->id - 1);
        context_vk->completed_command_buffer_id = 0;
        buffer->id = 1;
    }
    wined3d_context_vk_cleanup_resources(context_vk);
}

void wined3d_context_vk_wait_command_buffer(struct wined3d_context_vk *context_vk, uint64_t id)
{
    struct wined3d_device_vk *device_vk = wined3d_device_vk(context_vk->c.device);
    const struct wined3d_vk_info *vk_info = context_vk->vk_info;
    SIZE_T i;

    if (id <= context_vk->completed_command_buffer_id
            || id > context_vk->current_command_buffer.id) /* In case the buffer ID wrapped. */
        return;

    for (i = 0; i < context_vk->submitted.buffer_count; ++i)
    {
        if (context_vk->submitted.buffers[i].id != id)
            continue;

        VK_CALL(vkWaitForFences(device_vk->vk_device, 1,
                &context_vk->submitted.buffers[i].vk_fence, VK_TRUE, UINT64_MAX));
        wined3d_context_vk_cleanup_resources(context_vk);
        return;
    }

    ERR("Failed to find fence for command buffer with id 0x%s.\n", wine_dbgstr_longlong(id));
}

void wined3d_context_vk_image_barrier(struct wined3d_context_vk *context_vk,
        VkCommandBuffer vk_command_buffer, VkPipelineStageFlags src_stage_mask, VkPipelineStageFlags dst_stage_mask,
        VkAccessFlags src_access_mask, VkAccessFlags dst_access_mask, VkImageLayout old_layout,
        VkImageLayout new_layout, VkImage image, const VkImageSubresourceRange *range)
{
    const struct wined3d_vk_info *vk_info = context_vk->vk_info;
    VkImageMemoryBarrier barrier;

    wined3d_context_vk_end_current_render_pass(context_vk);

    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.pNext = NULL;
    barrier.srcAccessMask = src_access_mask;
    barrier.dstAccessMask = dst_access_mask;
    barrier.oldLayout = old_layout;
    barrier.newLayout = new_layout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange = *range;

    VK_CALL(vkCmdPipelineBarrier(vk_command_buffer, src_stage_mask, dst_stage_mask, 0, 0, NULL, 0, NULL, 1, &barrier));
}

static int wined3d_render_pass_vk_compare(const void *key, const struct wine_rb_entry *entry)
{
    const struct wined3d_render_pass_key_vk *k = key;
    const struct wined3d_render_pass_vk *pass = WINE_RB_ENTRY_VALUE(entry,
            const struct wined3d_render_pass_vk, entry);

    return memcmp(k, &pass->key, sizeof(*k));
}

static int wined3d_pipeline_layout_vk_compare(const void *key, const struct wine_rb_entry *entry)
{
    const struct wined3d_pipeline_layout_key_vk *a = key;
    const struct wined3d_pipeline_layout_key_vk *b = &WINE_RB_ENTRY_VALUE(entry,
            const struct wined3d_pipeline_layout_vk, entry)->key;

    if (a->binding_count != b->binding_count)
        return a->binding_count - b->binding_count;
    return memcmp(a->bindings, b->bindings, a->binding_count * sizeof(*a->bindings));
}

static int wined3d_graphics_pipeline_vk_compare(const void *key, const struct wine_rb_entry *entry)
{
    const struct wined3d_graphics_pipeline_key_vk *a = key;
    const struct wined3d_graphics_pipeline_key_vk *b = &WINE_RB_ENTRY_VALUE(entry,
            const struct wined3d_graphics_pipeline_vk, entry)->key;
    unsigned int i;
    int ret;

    if (a->pipeline_desc.stageCount != b->pipeline_desc.stageCount)
        return a->pipeline_desc.stageCount - b->pipeline_desc.stageCount;
    for (i = 0; i < a->pipeline_desc.stageCount; ++i)
    {
        if (a->stages[i].module != b->stages[i].module)
            return a->stages[i].module - b->stages[i].module;
    }

    if (a->divisor_desc.vertexBindingDivisorCount != b->divisor_desc.vertexBindingDivisorCount)
        return a->divisor_desc.vertexBindingDivisorCount - b->divisor_desc.vertexBindingDivisorCount;
    if ((ret = memcmp(a->divisors, b->divisors,
            a->divisor_desc.vertexBindingDivisorCount * sizeof(*a->divisors))))
        return ret;

    if (a->input_desc.vertexAttributeDescriptionCount != b->input_desc.vertexAttributeDescriptionCount)
        return a->input_desc.vertexAttributeDescriptionCount - b->input_desc.vertexAttributeDescriptionCount;
    if ((ret = memcmp(a->attributes, b->attributes,
            a->input_desc.vertexAttributeDescriptionCount * sizeof(*a->attributes))))
        return ret;
    if (a->input_desc.vertexBindingDescriptionCount != b->input_desc.vertexBindingDescriptionCount)
        return a->input_desc.vertexBindingDescriptionCount - b->input_desc.vertexBindingDescriptionCount;
    if ((ret = memcmp(a->bindings, b->bindings,
            a->input_desc.vertexBindingDescriptionCount * sizeof(*a->bindings))))
        return ret;

    if (a->ia_desc.topology != b->ia_desc.topology)
        return a->ia_desc.topology - b->ia_desc.topology;
    if (a->ia_desc.primitiveRestartEnable != b->ia_desc.primitiveRestartEnable)
        return a->ia_desc.primitiveRestartEnable - b->ia_desc.primitiveRestartEnable;

    if (a->ts_desc.patchControlPoints != b->ts_desc.patchControlPoints)
        return a->ts_desc.patchControlPoints - b->ts_desc.patchControlPoints;

    if ((ret = memcmp(&a->viewport, &b->viewport, sizeof(a->viewport))))
        return ret;

    if ((ret = memcmp(&a->scissor, &b->scissor, sizeof(a->scissor))))
        return ret;

    if ((ret = memcmp(&a->rs_desc, &b->rs_desc, sizeof(a->rs_desc))))
        return ret;

    if (a->ms_desc.rasterizationSamples != b->ms_desc.rasterizationSamples)
        return a->ms_desc.rasterizationSamples - b->ms_desc.rasterizationSamples;
    if (a->ms_desc.alphaToCoverageEnable != b->ms_desc.alphaToCoverageEnable)
        return a->ms_desc.alphaToCoverageEnable - b->ms_desc.alphaToCoverageEnable;
    if (a->sample_mask != b->sample_mask)
        return a->sample_mask - b->sample_mask;

    if ((ret = memcmp(&a->ds_desc, &b->ds_desc, sizeof(a->ds_desc))))
        return ret;

    if (a->blend_desc.attachmentCount != b->blend_desc.attachmentCount)
        return a->blend_desc.attachmentCount - b->blend_desc.attachmentCount;
    if ((ret = memcmp(a->blend_attachments, b->blend_attachments,
            a->blend_desc.attachmentCount * sizeof(*a->blend_attachments))))
        return ret;

    if (a->pipeline_desc.layout != b->pipeline_desc.layout)
        return a->pipeline_desc.layout - b->pipeline_desc.layout;

    if (a->pipeline_desc.renderPass != b->pipeline_desc.renderPass)
        return a->pipeline_desc.renderPass - b->pipeline_desc.renderPass;

    return 0;
}

static int wined3d_bo_slab_vk_compare(const void *key, const struct wine_rb_entry *entry)
{
    const struct wined3d_bo_slab_vk *slab = WINE_RB_ENTRY_VALUE(entry, const struct wined3d_bo_slab_vk, entry);
    const struct wined3d_bo_slab_vk_key *k = key;

    if (k->memory_type != slab->requested_memory_type)
        return k->memory_type - slab->requested_memory_type;
    if (k->usage != slab->bo.usage)
        return k->usage - slab->bo.usage;
    return k->size - slab->bo.size;
}

static void wined3d_context_vk_init_graphics_pipeline_key(struct wined3d_context_vk *context_vk)
{
    struct wined3d_graphics_pipeline_key_vk *key;
    VkPipelineShaderStageCreateInfo *stage;
    unsigned int i;

    static const VkDynamicState dynamic_states[] =
    {
        VK_DYNAMIC_STATE_BLEND_CONSTANTS,
        VK_DYNAMIC_STATE_STENCIL_REFERENCE,
    };

    key = &context_vk->graphics.pipeline_key_vk;
    memset(key, 0, sizeof(*key));

    for (i = 0; i < ARRAY_SIZE(context_vk->graphics.vk_modules); ++i)
    {
        stage = &key->stages[i];
        stage->sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stage->pName = "main";
    }

    key->input_desc.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    key->input_desc.pVertexBindingDescriptions = key->bindings;
    key->input_desc.pVertexAttributeDescriptions = key->attributes;

    key->divisor_desc.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_DIVISOR_STATE_CREATE_INFO_EXT;
    key->divisor_desc.pVertexBindingDivisors = key->divisors;

    key->ia_desc.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;

    key->ts_desc.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;

    key->vp_desc.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    key->vp_desc.viewportCount = 1;
    key->vp_desc.pViewports = &key->viewport;
    key->vp_desc.scissorCount = 1;
    key->vp_desc.pScissors = &key->scissor;

    key->rs_desc.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    key->rs_desc.lineWidth = 1.0f;

    key->ms_desc.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    key->ms_desc.pSampleMask = &key->sample_mask;

    key->ds_desc.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    key->ds_desc.maxDepthBounds = 1.0f;

    key->blend_desc.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    key->blend_desc.logicOp = VK_LOGIC_OP_COPY;
    key->blend_desc.pAttachments = key->blend_attachments;
    key->blend_desc.blendConstants[0] = 1.0f;
    key->blend_desc.blendConstants[1] = 1.0f;
    key->blend_desc.blendConstants[2] = 1.0f;
    key->blend_desc.blendConstants[3] = 1.0f;

    key->dynamic_desc.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    key->dynamic_desc.dynamicStateCount = ARRAY_SIZE(dynamic_states);
    key->dynamic_desc.pDynamicStates = dynamic_states;

    key->pipeline_desc.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    key->pipeline_desc.pStages = key->stages;
    key->pipeline_desc.pVertexInputState = &key->input_desc;
    key->pipeline_desc.pInputAssemblyState = &key->ia_desc;
    key->pipeline_desc.pTessellationState = &key->ts_desc;
    key->pipeline_desc.pViewportState = &key->vp_desc;
    key->pipeline_desc.pRasterizationState = &key->rs_desc;
    key->pipeline_desc.pMultisampleState = &key->ms_desc;
    key->pipeline_desc.pDepthStencilState = &key->ds_desc;
    key->pipeline_desc.pColorBlendState = &key->blend_desc;
    key->pipeline_desc.pDynamicState = &key->dynamic_desc;
    key->pipeline_desc.basePipelineIndex = -1;
}

static void wined3d_context_vk_update_rasterisation_state(const struct wined3d_context_vk *context_vk,
        const struct wined3d_state *state, struct wined3d_graphics_pipeline_key_vk *key)
{
    const struct wined3d_d3d_info *d3d_info = context_vk->c.d3d_info;
    VkPipelineRasterizationStateCreateInfo *desc = &key->rs_desc;
    const struct wined3d_rasterizer_state_desc *r;
    float scale_bias;
    union
    {
        uint32_t u32;
        float f32;
    } const_bias;

    if (!state->rasterizer_state)
    {
        desc->depthClampEnable = VK_FALSE;
        desc->rasterizerDiscardEnable = is_rasterization_disabled(state->shader[WINED3D_SHADER_TYPE_GEOMETRY]);
        desc->cullMode = VK_CULL_MODE_BACK_BIT;
        desc->frontFace = VK_FRONT_FACE_CLOCKWISE;
        desc->depthBiasEnable = VK_FALSE;
        desc->depthBiasConstantFactor = 0.0f;
        desc->depthBiasClamp = 0.0f;
        desc->depthBiasSlopeFactor = 0.0f;

        return;
    }

    r = &state->rasterizer_state->desc;
    desc->depthClampEnable = !r->depth_clip;
    desc->rasterizerDiscardEnable = is_rasterization_disabled(state->shader[WINED3D_SHADER_TYPE_GEOMETRY]);
    desc->cullMode = vk_cull_mode_from_wined3d(r->cull_mode);
    desc->frontFace = r->front_ccw ? VK_FRONT_FACE_COUNTER_CLOCKWISE : VK_FRONT_FACE_CLOCKWISE;

    scale_bias = r->scale_bias;
    const_bias.f32 = r->depth_bias;
    if (!scale_bias && !const_bias.f32)
    {
        desc->depthBiasEnable = VK_FALSE;
        desc->depthBiasConstantFactor = 0.0f;
        desc->depthBiasClamp = 0.0f;
        desc->depthBiasSlopeFactor = 0.0f;

        return;
    }

    desc->depthBiasEnable = VK_TRUE;
    if (d3d_info->wined3d_creation_flags & WINED3D_LEGACY_DEPTH_BIAS)
    {
        const struct wined3d_rendertarget_view *dsv;

        if ((dsv = state->fb.depth_stencil))
        {
            desc->depthBiasConstantFactor = -(float)const_bias.u32 / dsv->format->depth_bias_scale;
            desc->depthBiasSlopeFactor = -(float)const_bias.u32;
        }
        else
        {
            desc->depthBiasConstantFactor = 0.0f;
            desc->depthBiasSlopeFactor = 0.0f;
        }
    }
    else
    {
        desc->depthBiasConstantFactor = const_bias.f32;
        desc->depthBiasSlopeFactor = scale_bias;
    }
    desc->depthBiasClamp = r->depth_bias_clamp;
}

static void wined3d_context_vk_update_blend_state(const struct wined3d_context_vk *context_vk,
        const struct wined3d_state *state, struct wined3d_graphics_pipeline_key_vk *key)
{
    VkPipelineColorBlendStateCreateInfo *desc = &key->blend_desc;
    const struct wined3d_blend_state_desc *b;
    unsigned int i;

    desc->attachmentCount = context_vk->rt_count;

    memset(key->blend_attachments, 0, sizeof(key->blend_attachments));
    if (!state->blend_state)
    {
        for (i = 0; i < context_vk->rt_count; ++i)
        {
            key->blend_attachments[i].colorWriteMask = VK_COLOR_COMPONENT_R_BIT
                    | VK_COLOR_COMPONENT_G_BIT
                    | VK_COLOR_COMPONENT_B_BIT
                    | VK_COLOR_COMPONENT_A_BIT;
        }

        return;
    }

    b = &state->blend_state->desc;
    for (i = 0; i < context_vk->rt_count; ++i)
    {
        const struct wined3d_rendertarget_blend_state_desc *rt = &b->rt[b->independent ? i : 0];
        const struct wined3d_rendertarget_view *rtv = state->fb.render_targets[i];
        VkPipelineColorBlendAttachmentState *a = &key->blend_attachments[i];
        enum wined3d_blend src_blend, dst_blend;
        const struct wined3d_format *rt_format;

        a->colorWriteMask = vk_colour_write_mask_from_wined3d(rt->writemask);
        if (!rt->enable)
            continue;

        if (rtv)
            rt_format = rtv->format;
        else
            rt_format = wined3d_get_format(context_vk->c.device->adapter, WINED3DFMT_NULL, 0);
        a->blendEnable = VK_TRUE;

        src_blend = rt->src;
        dst_blend = rt->dst;
        if (src_blend == WINED3D_BLEND_BOTHSRCALPHA)
        {
            src_blend = WINED3D_BLEND_SRCALPHA;
            dst_blend = WINED3D_BLEND_INVSRCALPHA;
        }
        else if (src_blend == WINED3D_BLEND_BOTHINVSRCALPHA)
        {
            src_blend = WINED3D_BLEND_INVSRCALPHA;
            dst_blend = WINED3D_BLEND_SRCALPHA;
        }
        a->srcColorBlendFactor = vk_blend_factor_from_wined3d(src_blend, rt_format, FALSE);
        a->dstColorBlendFactor = vk_blend_factor_from_wined3d(dst_blend, rt_format, FALSE);
        a->colorBlendOp = vk_blend_op_from_wined3d(rt->op);

        src_blend = rt->src_alpha;
        dst_blend = rt->dst_alpha;
        a->srcAlphaBlendFactor = vk_blend_factor_from_wined3d(src_blend, rt_format, TRUE);
        a->dstAlphaBlendFactor = vk_blend_factor_from_wined3d(dst_blend, rt_format, TRUE);
        a->alphaBlendOp = vk_blend_op_from_wined3d(rt->op_alpha);
    }
}

static bool wined3d_context_vk_update_graphics_pipeline_key(struct wined3d_context_vk *context_vk,
        const struct wined3d_state *state, VkPipelineLayout vk_pipeline_layout)
{
    unsigned int i, attribute_count, binding_count, divisor_count, stage_count;
    const struct wined3d_d3d_info *d3d_info = context_vk->c.d3d_info;
    struct wined3d_graphics_pipeline_key_vk *key;
    VkPipelineShaderStageCreateInfo *stage;
    struct wined3d_stream_info stream_info;
    VkPrimitiveTopology vk_topology;
    VkShaderModule module;
    bool update = false;
    uint32_t mask;

    key = &context_vk->graphics.pipeline_key_vk;

    if (context_vk->c.shader_update_mask & ~(1u << WINED3D_SHADER_TYPE_COMPUTE))
    {
        stage_count = 0;
        for (i = 0; i < ARRAY_SIZE(context_vk->graphics.vk_modules); ++i)
        {
            if (!(module = context_vk->graphics.vk_modules[i]))
                continue;

            stage = &key->stages[stage_count++];
            stage->stage = vk_shader_stage_from_wined3d(i);
            stage->module = module;
        }

        key->pipeline_desc.stageCount = stage_count;

        update = true;
    }

    if (wined3d_context_is_graphics_state_dirty(&context_vk->c, STATE_VDECL)
            || wined3d_context_is_graphics_state_dirty(&context_vk->c, STATE_STREAMSRC)
            || wined3d_context_is_graphics_state_dirty(&context_vk->c, STATE_SHADER(WINED3D_SHADER_TYPE_VERTEX)))
    {
        wined3d_stream_info_from_declaration(&stream_info, state, d3d_info);
        divisor_count = 0;
        for (i = 0, mask = 0, attribute_count = 0, binding_count = 0; i < ARRAY_SIZE(stream_info.elements); ++i)
        {
            VkVertexInputBindingDivisorDescriptionEXT *d;
            struct wined3d_stream_info_element *e;
            VkVertexInputAttributeDescription *a;
            VkVertexInputBindingDescription *b;
            uint32_t binding;

            if (!(stream_info.use_map & (1u << i)))
                continue;

            a = &key->attributes[attribute_count++];
            e = &stream_info.elements[i];
            binding = e->stream_idx;

            a->location = i;
            a->binding = binding;
            a->format = wined3d_format_vk(e->format)->vk_format;
            a->offset = (UINT_PTR)e->data.addr - state->streams[binding].offset;

            if (mask & (1u << binding))
                continue;
            mask |= 1u << binding;

            b = &key->bindings[binding_count++];
            b->binding = binding;
            b->stride = e->stride;
            b->inputRate = e->instanced ? VK_VERTEX_INPUT_RATE_INSTANCE : VK_VERTEX_INPUT_RATE_VERTEX;

            if (e->instanced)
            {
                d = &key->divisors[divisor_count++];
                d->binding = binding;
                d->divisor = e->divisor;
            }
        }

        key->input_desc.pNext = NULL;
        key->input_desc.vertexBindingDescriptionCount = binding_count;
        key->input_desc.vertexAttributeDescriptionCount = attribute_count;

        if (divisor_count)
        {
            key->input_desc.pNext = &key->divisor_desc;
            key->divisor_desc.vertexBindingDivisorCount = divisor_count;
        }

        update = true;
    }

    vk_topology = vk_topology_from_wined3d(state->primitive_type);
    if (key->ia_desc.topology != vk_topology)
    {
        key->ia_desc.topology = vk_topology;
        key->ia_desc.primitiveRestartEnable = !(d3d_info->wined3d_creation_flags & WINED3D_NO_PRIMITIVE_RESTART)
                && !wined3d_primitive_type_is_list(state->primitive_type);

        update = true;
    }

    if (key->ts_desc.patchControlPoints != state->patch_vertex_count)
    {
        key->ts_desc.patchControlPoints = state->patch_vertex_count;

        update = true;
    }

    if (wined3d_context_is_graphics_state_dirty(&context_vk->c, STATE_VIEWPORT)
            || wined3d_context_is_graphics_state_dirty(&context_vk->c, STATE_SCISSORRECT)
            || wined3d_context_is_graphics_state_dirty(&context_vk->c, STATE_RASTERIZER))
    {
        key->viewport.x = state->viewports[0].x;
        key->viewport.y = state->viewports[0].y;
        key->viewport.width = state->viewports[0].width;
        key->viewport.height = state->viewports[0].height;
        key->viewport.minDepth = state->viewports[0].min_z;
        key->viewport.maxDepth = state->viewports[0].max_z;

        if (state->rasterizer_state && state->rasterizer_state->desc.scissor)
        {
            const RECT *r = &state->scissor_rects[0];

            key->scissor.offset.x = r->left;
            key->scissor.offset.y = r->top;
            key->scissor.extent.width =  r->right - r->left;
            key->scissor.extent.height = r->bottom - r->top;
        }
        else
        {
            key->scissor.offset.x = key->viewport.x;
            key->scissor.offset.y = key->viewport.y;
            key->scissor.extent.width = key->viewport.width;
            key->scissor.extent.height = key->viewport.height;
        }
        key->viewport.y += key->viewport.height;
        key->viewport.height = -key->viewport.height;

        update = true;
    }

    if (wined3d_context_is_graphics_state_dirty(&context_vk->c, STATE_RASTERIZER)
            || wined3d_context_is_graphics_state_dirty(&context_vk->c, STATE_SHADER(WINED3D_SHADER_TYPE_GEOMETRY)))
    {
        wined3d_context_vk_update_rasterisation_state(context_vk, state, key);

        update = true;
    }

    if (key->ms_desc.rasterizationSamples != context_vk->sample_count
            || isStateDirty(&context_vk->c, STATE_BLEND) || isStateDirty(&context_vk->c, STATE_SAMPLE_MASK))
    {
        key->ms_desc.rasterizationSamples = context_vk->sample_count;
        key->ms_desc.alphaToCoverageEnable = state->blend_state && state->blend_state->desc.alpha_to_coverage;
        key->sample_mask = state->sample_mask;

        update = true;
    }

    if (wined3d_context_is_graphics_state_dirty(&context_vk->c, STATE_DEPTH_STENCIL)
            || wined3d_context_is_graphics_state_dirty(&context_vk->c, STATE_FRAMEBUFFER))
    {
        const struct wined3d_depth_stencil_state *d = state->depth_stencil_state;

        if (d)
        {
            key->ds_desc.depthTestEnable = d->desc.depth;
            key->ds_desc.depthWriteEnable = d->desc.depth_write;
            key->ds_desc.depthCompareOp = vk_compare_op_from_wined3d(d->desc.depth_func);
            key->ds_desc.stencilTestEnable = state->fb.depth_stencil && d->desc.stencil;
            if (key->ds_desc.stencilTestEnable)
            {
                key->ds_desc.front.failOp = vk_stencil_op_from_wined3d(d->desc.front.fail_op);
                key->ds_desc.front.passOp = vk_stencil_op_from_wined3d(d->desc.front.pass_op);
                key->ds_desc.front.depthFailOp = vk_stencil_op_from_wined3d(d->desc.front.depth_fail_op);
                key->ds_desc.front.compareOp = vk_compare_op_from_wined3d(d->desc.front.func);
                key->ds_desc.front.compareMask = d->desc.stencil_read_mask;
                key->ds_desc.front.writeMask = d->desc.stencil_write_mask;

                key->ds_desc.back.failOp = vk_stencil_op_from_wined3d(d->desc.back.fail_op);
                key->ds_desc.back.passOp = vk_stencil_op_from_wined3d(d->desc.back.pass_op);
                key->ds_desc.back.depthFailOp = vk_stencil_op_from_wined3d(d->desc.back.depth_fail_op);
                key->ds_desc.back.compareOp = vk_compare_op_from_wined3d(d->desc.back.func);
                key->ds_desc.back.compareMask = d->desc.stencil_read_mask;
                key->ds_desc.back.writeMask = d->desc.stencil_write_mask;
            }
            else
            {
                memset(&key->ds_desc.front, 0, sizeof(key->ds_desc.front));
                memset(&key->ds_desc.back, 0, sizeof(key->ds_desc.back));
            }
        }
        else
        {
            key->ds_desc.depthTestEnable = VK_TRUE;
            key->ds_desc.depthWriteEnable = VK_TRUE;
            key->ds_desc.depthCompareOp = VK_COMPARE_OP_LESS;
            key->ds_desc.stencilTestEnable = VK_FALSE;
        }

        update = true;
    }

    if (wined3d_context_is_graphics_state_dirty(&context_vk->c, STATE_BLEND)
            || wined3d_context_is_graphics_state_dirty(&context_vk->c, STATE_FRAMEBUFFER))
    {
        wined3d_context_vk_update_blend_state(context_vk, state, key);

        update = true;
    }

    if (key->pipeline_desc.layout != vk_pipeline_layout)
    {
        key->pipeline_desc.layout = vk_pipeline_layout;

        update = true;
    }

    if (key->pipeline_desc.renderPass != context_vk->vk_render_pass)
    {
        key->pipeline_desc.renderPass = context_vk->vk_render_pass;

        update = true;
    }

    return update;
}

static bool wined3d_context_vk_begin_render_pass(struct wined3d_context_vk *context_vk,
        VkCommandBuffer vk_command_buffer, const struct wined3d_state *state, const struct wined3d_vk_info *vk_info)
{
    struct wined3d_device_vk *device_vk = wined3d_device_vk(context_vk->c.device);
    VkImageView vk_views[WINED3D_MAX_RENDER_TARGETS + 1];
    unsigned int fb_width, fb_height, fb_layer_count;
    struct wined3d_rendertarget_view_vk *rtv_vk;
    struct wined3d_rendertarget_view *view;
    const VkPhysicalDeviceLimits *limits;
    VkRenderPassBeginInfo begin_info;
    unsigned int attachment_count, i;
    VkFramebufferCreateInfo fb_desc;
    VkResult vr;

    if (context_vk->vk_render_pass)
        return true;

    limits = &wined3d_adapter_vk(device_vk->d.adapter)->device_limits;
    fb_width = limits->maxFramebufferWidth;
    fb_height = limits->maxFramebufferHeight;
    fb_layer_count = limits->maxFramebufferLayers;
    attachment_count = 0;

    context_vk->rt_count = 0;
    for (i = 0; i < ARRAY_SIZE(state->fb.render_targets); ++i)
    {
        if (!(view = state->fb.render_targets[i]) || view->format->id == WINED3DFMT_NULL)
            continue;

        rtv_vk = wined3d_rendertarget_view_vk(view);
        vk_views[attachment_count] = wined3d_rendertarget_view_vk_get_image_view(rtv_vk, context_vk);
        wined3d_rendertarget_view_vk_barrier(rtv_vk, context_vk, WINED3D_BIND_RENDER_TARGET);
        wined3d_context_vk_reference_rendertarget_view(context_vk, rtv_vk);

        if (view->width < fb_width)
            fb_width = view->width;
        if (view->height < fb_height)
            fb_height = view->height;
        if (view->layer_count < fb_layer_count)
            fb_layer_count = view->layer_count;
        context_vk->rt_count = i + 1;
        ++attachment_count;
    }

    if ((view = state->fb.depth_stencil))
    {
        rtv_vk = wined3d_rendertarget_view_vk(view);
        vk_views[attachment_count] = wined3d_rendertarget_view_vk_get_image_view(rtv_vk, context_vk);
        wined3d_rendertarget_view_vk_barrier(rtv_vk, context_vk, WINED3D_BIND_DEPTH_STENCIL);
        wined3d_context_vk_reference_rendertarget_view(context_vk, rtv_vk);

        if (view->width < fb_width)
            fb_width = view->width;
        if (view->height < fb_height)
            fb_height = view->height;
        if (view->layer_count < fb_layer_count)
            fb_layer_count = view->layer_count;
        ++attachment_count;
    }

    if (!(context_vk->vk_render_pass = wined3d_context_vk_get_render_pass(context_vk, &state->fb,
            ARRAY_SIZE(state->fb.render_targets), !!state->fb.depth_stencil, 0)))
    {
        ERR("Failed to get render pass.\n");
        return false;
    }

    fb_desc.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fb_desc.pNext = NULL;
    fb_desc.flags = 0;
    fb_desc.renderPass = context_vk->vk_render_pass;
    fb_desc.attachmentCount = attachment_count;
    fb_desc.pAttachments = vk_views;
    fb_desc.width = fb_width;
    fb_desc.height = fb_height;
    fb_desc.layers = fb_layer_count;

    if ((vr = VK_CALL(vkCreateFramebuffer(device_vk->vk_device, &fb_desc, NULL, &context_vk->vk_framebuffer))) < 0)
    {
        WARN("Failed to create Vulkan framebuffer, vr %s.\n", wined3d_debug_vkresult(vr));
        return false;
    }

    begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    begin_info.pNext = NULL;
    begin_info.renderPass = context_vk->vk_render_pass;
    begin_info.framebuffer = context_vk->vk_framebuffer;
    begin_info.renderArea.offset.x = 0;
    begin_info.renderArea.offset.y = 0;
    begin_info.renderArea.extent.width = fb_width;
    begin_info.renderArea.extent.height = fb_height;
    begin_info.clearValueCount = 0;
    begin_info.pClearValues = NULL;
    VK_CALL(vkCmdBeginRenderPass(vk_command_buffer, &begin_info, VK_SUBPASS_CONTENTS_INLINE));

    return true;
}

static void wined3d_context_vk_bind_vertex_buffers(struct wined3d_context_vk *context_vk,
        VkCommandBuffer vk_command_buffer, const struct wined3d_state *state, const struct wined3d_vk_info *vk_info)
{
    VkDeviceSize offsets[ARRAY_SIZE(state->streams)] = {0};
    VkBuffer buffers[ARRAY_SIZE(state->streams)];
    const struct wined3d_stream_state *stream;
    const VkDescriptorBufferInfo *buffer_info;
    struct wined3d_buffer_vk *buffer_vk;
    struct wined3d_buffer *buffer;
    unsigned int i, first, count;

    first = 0;
    count = 0;
    for (i = 0; i < ARRAY_SIZE(state->streams); ++i)
    {
        stream = &state->streams[i];

        if ((buffer = stream->buffer))
        {
            buffer_vk = wined3d_buffer_vk(buffer);
            buffer_info = wined3d_buffer_vk_get_buffer_info(buffer_vk);
            wined3d_context_vk_reference_bo(context_vk, &buffer_vk->bo);
            buffers[count] = buffer_info->buffer;
            offsets[count] = buffer_info->offset + stream->offset;
            ++count;
            continue;
        }

        if (count)
            VK_CALL(vkCmdBindVertexBuffers(vk_command_buffer, first, count, buffers, offsets));
        first = i + 1;
        count = 0;
    }

    if (count)
        VK_CALL(vkCmdBindVertexBuffers(vk_command_buffer, first, count, buffers, offsets));
}

static void wined3d_context_vk_bind_stream_output_buffers(struct wined3d_context_vk *context_vk,
        VkCommandBuffer vk_command_buffer, const struct wined3d_state *state, const struct wined3d_vk_info *vk_info)
{
    VkDeviceSize offsets[ARRAY_SIZE(state->stream_output)];
    VkDeviceSize sizes[ARRAY_SIZE(state->stream_output)];
    VkBuffer buffers[ARRAY_SIZE(state->stream_output)];
    const struct wined3d_stream_output *stream;
    const VkDescriptorBufferInfo *buffer_info;
    struct wined3d_buffer_vk *buffer_vk;
    struct wined3d_buffer *buffer;
    unsigned int i, first, count;

    first = 0;
    count = 0;
    for (i = 0; i < ARRAY_SIZE(state->stream_output); ++i)
    {
        stream = &state->stream_output[i];

        if ((buffer = stream->buffer))
        {
            buffer_vk = wined3d_buffer_vk(buffer);
            buffer_info = wined3d_buffer_vk_get_buffer_info(buffer_vk);
            wined3d_context_vk_reference_bo(context_vk, &buffer_vk->bo);
            buffers[count] = buffer_info->buffer;
            if ((offsets[count] = stream->offset) == ~0u)
            {
                FIXME("Appending to stream output buffers not implemented.\n");
                offsets[count] = 0;
            }
            sizes[count] = buffer_info->range - offsets[count];
            offsets[count] += buffer_info->offset;
            ++count;
            continue;
        }

        if (count)
            VK_CALL(vkCmdBindTransformFeedbackBuffersEXT(vk_command_buffer, first, count, buffers, offsets, sizes));
        first = i + 1;
        count = 0;
    }

    if (count)
        VK_CALL(vkCmdBindTransformFeedbackBuffersEXT(vk_command_buffer, first, count, buffers, offsets, sizes));
}

static VkResult wined3d_context_vk_create_vk_descriptor_pool(struct wined3d_device_vk *device_vk,
        const struct wined3d_vk_info *vk_info, VkDescriptorPool *vk_pool)
{
    struct VkDescriptorPoolCreateInfo pool_desc;
    VkResult vr;

    static const VkDescriptorPoolSize pool_sizes[] =
    {
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1024},
        {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1024},
        {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1024},
        {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1024},
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1024},
        {VK_DESCRIPTOR_TYPE_SAMPLER, 1024},
    };

    pool_desc.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_desc.pNext = NULL;
    pool_desc.flags = 0;
    pool_desc.maxSets = 512;
    pool_desc.poolSizeCount = ARRAY_SIZE(pool_sizes);
    pool_desc.pPoolSizes = pool_sizes;

    if ((vr = VK_CALL(vkCreateDescriptorPool(device_vk->vk_device, &pool_desc, NULL, vk_pool))) < 0)
        ERR("Failed to create descriptor pool, vr %s.\n", wined3d_debug_vkresult(vr));

    return vr;
}

static VkResult wined3d_context_vk_create_vk_descriptor_set(struct wined3d_context_vk *context_vk,
        VkDescriptorSetLayout vk_set_layout, VkDescriptorSet *vk_descriptor_set)
{
    struct wined3d_device_vk *device_vk = wined3d_device_vk(context_vk->c.device);
    const struct wined3d_vk_info *vk_info = context_vk->vk_info;
    struct VkDescriptorSetAllocateInfo set_desc;
    VkResult vr;

    if (!context_vk->vk_descriptor_pool && (vr = wined3d_context_vk_create_vk_descriptor_pool(device_vk,
            vk_info, &context_vk->vk_descriptor_pool)))
    {
        WARN("Failed to create descriptor pool, vr %s.\n", wined3d_debug_vkresult(vr));
        return vr;
    }

    set_desc.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    set_desc.pNext = NULL;
    set_desc.descriptorPool = context_vk->vk_descriptor_pool;
    set_desc.descriptorSetCount = 1;
    set_desc.pSetLayouts = &vk_set_layout;
    if ((vr = VK_CALL(vkAllocateDescriptorSets(device_vk->vk_device, &set_desc, vk_descriptor_set))) >= 0)
        return vr;

    if (vr == VK_ERROR_FRAGMENTED_POOL || vr == VK_ERROR_OUT_OF_POOL_MEMORY)
    {
        wined3d_context_vk_destroy_vk_descriptor_pool(context_vk,
                context_vk->vk_descriptor_pool, context_vk->current_command_buffer.id);
        context_vk->vk_descriptor_pool = VK_NULL_HANDLE;
        if ((vr = wined3d_context_vk_create_vk_descriptor_pool(device_vk, vk_info, &context_vk->vk_descriptor_pool)))
        {
            WARN("Failed to create descriptor pool, vr %s.\n", wined3d_debug_vkresult(vr));
            return vr;
        }

        set_desc.descriptorPool = context_vk->vk_descriptor_pool;
        if ((vr = VK_CALL(vkAllocateDescriptorSets(device_vk->vk_device, &set_desc, vk_descriptor_set))) >= 0)
            return vr;
    }

    WARN("Failed to allocate descriptor set, vr %s.\n", wined3d_debug_vkresult(vr));

    return vr;
}

static bool wined3d_shader_descriptor_writes_vk_add_write(struct wined3d_shader_descriptor_writes_vk *writes,
        VkDescriptorSet vk_descriptor_set, size_t binding_idx, VkDescriptorType type,
        const VkDescriptorBufferInfo *buffer_info, const VkDescriptorImageInfo *image_info,
        const VkBufferView *buffer_view)
{
    SIZE_T write_count = writes->count;
    VkWriteDescriptorSet *write;

    if (!wined3d_array_reserve((void **)&writes->writes, &writes->size,
            write_count + 1, sizeof(*writes->writes)))
        return false;

    write = &writes->writes[write_count];
    write->sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write->pNext = NULL;
    write->dstSet = vk_descriptor_set;
    write->dstBinding = binding_idx;
    write->dstArrayElement = 0;
    write->descriptorCount = 1;
    write->descriptorType = type;
    write->pImageInfo = image_info;
    write->pBufferInfo = buffer_info;
    write->pTexelBufferView = buffer_view;

    ++writes->count;

    return true;
}

static bool wined3d_shader_resource_bindings_add_null_srv_binding(struct wined3d_shader_descriptor_writes_vk *writes,
        VkDescriptorSet vk_descriptor_set, size_t binding_idx, enum wined3d_shader_resource_type type,
        enum wined3d_data_type data_type, struct wined3d_context_vk *context_vk)
{
    const struct wined3d_null_views_vk *v = &wined3d_device_vk(context_vk->c.device)->null_views_vk;

    switch (type)
    {
        case WINED3D_SHADER_RESOURCE_BUFFER:
            if (data_type == WINED3D_DATA_FLOAT)
                return wined3d_shader_descriptor_writes_vk_add_write(writes, vk_descriptor_set, binding_idx,
                        VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, NULL, NULL, &v->vk_view_buffer_float);
            return wined3d_shader_descriptor_writes_vk_add_write(writes, vk_descriptor_set, binding_idx,
                    VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, NULL, NULL, &v->vk_view_buffer_uint);

        case WINED3D_SHADER_RESOURCE_TEXTURE_1D:
            return wined3d_shader_descriptor_writes_vk_add_write(writes, vk_descriptor_set,
                    binding_idx, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, NULL, &v->vk_info_1d, NULL);

        case WINED3D_SHADER_RESOURCE_TEXTURE_2D:
            return wined3d_shader_descriptor_writes_vk_add_write(writes, vk_descriptor_set,
                    binding_idx, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, NULL, &v->vk_info_2d, NULL);

        case WINED3D_SHADER_RESOURCE_TEXTURE_2DMS:
            return wined3d_shader_descriptor_writes_vk_add_write(writes, vk_descriptor_set,
                    binding_idx, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, NULL, &v->vk_info_2dms, NULL);

        case WINED3D_SHADER_RESOURCE_TEXTURE_3D:
            return wined3d_shader_descriptor_writes_vk_add_write(writes, vk_descriptor_set,
                    binding_idx, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, NULL, &v->vk_info_3d, NULL);

        case WINED3D_SHADER_RESOURCE_TEXTURE_CUBE:
            return wined3d_shader_descriptor_writes_vk_add_write(writes, vk_descriptor_set,
                    binding_idx, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, NULL, &v->vk_info_cube, NULL);

        case WINED3D_SHADER_RESOURCE_TEXTURE_2DARRAY:
            return wined3d_shader_descriptor_writes_vk_add_write(writes, vk_descriptor_set,
                    binding_idx, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, NULL, &v->vk_info_2d_array, NULL);

        case WINED3D_SHADER_RESOURCE_TEXTURE_2DMSARRAY:
            return wined3d_shader_descriptor_writes_vk_add_write(writes, vk_descriptor_set,
                    binding_idx, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, NULL, &v->vk_info_2dms_array, NULL);

        default:
            FIXME("Unhandled resource type %#x.\n", type);
            return false;
    }
}

static bool wined3d_context_vk_update_descriptors(struct wined3d_context_vk *context_vk,
        VkCommandBuffer vk_command_buffer, const struct wined3d_state *state, enum wined3d_pipeline pipeline)
{
    struct wined3d_shader_descriptor_writes_vk *writes = &context_vk->descriptor_writes;
    struct wined3d_device_vk *device_vk = wined3d_device_vk(context_vk->c.device);
    const struct wined3d_vk_info *vk_info = context_vk->vk_info;
    const struct wined3d_shader_resource_binding *binding;
    struct wined3d_shader_resource_bindings *bindings;
    struct wined3d_unordered_access_view_vk *uav_vk;
    struct wined3d_shader_resource_view_vk *srv_vk;
    struct wined3d_unordered_access_view *uav;
    const VkDescriptorBufferInfo *buffer_info;
    struct wined3d_shader_resource_view *srv;
    const VkDescriptorImageInfo *image_info;
    struct wined3d_buffer_vk *buffer_vk;
    VkDescriptorSetLayout vk_set_layout;
    VkPipelineLayout vk_pipeline_layout;
    struct wined3d_resource *resource;
    VkPipelineBindPoint vk_bind_point;
    VkDescriptorSet vk_descriptor_set;
    struct wined3d_view_vk *view_vk;
    struct wined3d_sampler *sampler;
    struct wined3d_buffer *buffer;
    VkBufferView *buffer_view;
    VkDescriptorType type;
    VkResult vr;
    size_t i;

    switch (pipeline)
    {
        case WINED3D_PIPELINE_GRAPHICS:
            bindings = &context_vk->graphics.bindings;
            vk_bind_point = VK_PIPELINE_BIND_POINT_GRAPHICS;
            vk_set_layout = context_vk->graphics.vk_set_layout;
            vk_pipeline_layout = context_vk->graphics.vk_pipeline_layout;
            break;

        case WINED3D_PIPELINE_COMPUTE:
            bindings = &context_vk->compute.bindings;
            vk_bind_point = VK_PIPELINE_BIND_POINT_COMPUTE;
            vk_set_layout = context_vk->compute.vk_set_layout;
            vk_pipeline_layout = context_vk->compute.vk_pipeline_layout;
            break;

        default:
            ERR("Invalid pipeline %#x.\n", pipeline);
            return false;
    }

    if ((vr = wined3d_context_vk_create_vk_descriptor_set(context_vk, vk_set_layout, &vk_descriptor_set)))
    {
        WARN("Failed to create descriptor set, vr %s.\n", wined3d_debug_vkresult(vr));
        return false;
    }

    writes->count = 0;
    for (i = 0; i < bindings->count; ++i)
    {
        binding = &bindings->bindings[i];

        switch (binding->shader_descriptor_type)
        {
            case WINED3D_SHADER_DESCRIPTOR_TYPE_CBV:
                if (!(buffer = state->cb[binding->shader_type][binding->resource_idx]))
                {
                    if (!wined3d_shader_descriptor_writes_vk_add_write(writes, vk_descriptor_set,
                            binding->binding_idx, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                            &device_vk->null_resources_vk.buffer_info, NULL, NULL))
                        return false;
                    break;
                }
                buffer_vk = wined3d_buffer_vk(buffer);
                buffer_info = wined3d_buffer_vk_get_buffer_info(buffer_vk);
                if (!wined3d_shader_descriptor_writes_vk_add_write(writes, vk_descriptor_set,
                        binding->binding_idx, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, buffer_info, NULL, NULL))
                    return false;
                wined3d_context_vk_reference_bo(context_vk, &buffer_vk->bo);
                break;

            case WINED3D_SHADER_DESCRIPTOR_TYPE_SRV:
                if (!(srv = state->shader_resource_view[binding->shader_type][binding->resource_idx]))
                {
                    if (!wined3d_shader_resource_bindings_add_null_srv_binding(writes, vk_descriptor_set,
                            binding->binding_idx, binding->resource_type, binding->resource_data_type, context_vk))
                        return false;
                    break;
                }
                resource = srv->resource;

                srv_vk = wined3d_shader_resource_view_vk(srv);
                view_vk = &srv_vk->view_vk;
                if (resource->type == WINED3D_RTYPE_BUFFER)
                {
                    image_info = NULL;
                    buffer_view = &view_vk->u.vk_buffer_view;
                    type = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
                }
                else
                {
                    struct wined3d_texture_vk *texture_vk = wined3d_texture_vk(texture_from_resource(resource));

                    if (view_vk->u.vk_image_info.imageView)
                        image_info = &view_vk->u.vk_image_info;
                    else
                        image_info = wined3d_texture_vk_get_default_image_info(texture_vk, context_vk);
                    buffer_view = NULL;
                    type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
                }

                if (!wined3d_shader_descriptor_writes_vk_add_write(writes, vk_descriptor_set,
                        binding->binding_idx, type, NULL, image_info, buffer_view))
                    return false;
                wined3d_context_vk_reference_shader_resource_view(context_vk, srv_vk);
                break;

            case WINED3D_SHADER_DESCRIPTOR_TYPE_UAV:
                if (!(uav = state->unordered_access_view[pipeline][binding->resource_idx]))
                {
                    FIXME("NULL unordered access views not implemented.\n");
                    return false;
                }
                resource = uav->resource;

                uav_vk = wined3d_unordered_access_view_vk(uav);
                view_vk = &uav_vk->view_vk;
                if (resource->type == WINED3D_RTYPE_BUFFER)
                {
                    image_info = NULL;
                    buffer_view = &view_vk->u.vk_buffer_view;
                    type = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
                }
                else
                {
                    struct wined3d_texture_vk *texture_vk = wined3d_texture_vk(texture_from_resource(resource));

                    if (view_vk->u.vk_image_info.imageView)
                        image_info = &view_vk->u.vk_image_info;
                    else
                        image_info = wined3d_texture_vk_get_default_image_info(texture_vk, context_vk);
                    buffer_view = NULL;
                    type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                }

                if (!wined3d_shader_descriptor_writes_vk_add_write(writes, vk_descriptor_set,
                        binding->binding_idx, type, NULL, image_info, buffer_view))
                    return false;
                wined3d_context_vk_reference_unordered_access_view(context_vk, uav_vk);
                break;

            case WINED3D_SHADER_DESCRIPTOR_TYPE_UAV_COUNTER:
                if (!(uav = state->unordered_access_view[pipeline][binding->resource_idx]))
                {
                    FIXME("NULL unordered access view counters not implemented.\n");
                    return false;
                }

                uav_vk = wined3d_unordered_access_view_vk(uav);
                if (!uav_vk->vk_counter_view || !wined3d_shader_descriptor_writes_vk_add_write(writes,
                        vk_descriptor_set, binding->binding_idx, VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,
                        NULL, NULL, &uav_vk->vk_counter_view))
                    return false;
                break;

            case WINED3D_SHADER_DESCRIPTOR_TYPE_SAMPLER:
                if (!(sampler = state->sampler[binding->shader_type][binding->resource_idx]))
                    sampler = context_vk->c.device->null_sampler;
                if (!wined3d_shader_descriptor_writes_vk_add_write(writes, vk_descriptor_set, binding->binding_idx,
                        VK_DESCRIPTOR_TYPE_SAMPLER, NULL, &wined3d_sampler_vk(sampler)->vk_image_info, NULL))
                    return false;
                wined3d_context_vk_reference_sampler(context_vk, wined3d_sampler_vk(sampler));
                break;

            default:
                ERR("Invalid descriptor type %#x.\n", binding->shader_descriptor_type);
                return false;
        }
    }

    VK_CALL(vkUpdateDescriptorSets(device_vk->vk_device, writes->count, writes->writes, 0, NULL));
    VK_CALL(vkCmdBindDescriptorSets(vk_command_buffer, vk_bind_point,
            vk_pipeline_layout, 0, 1, &vk_descriptor_set, 0, NULL));

    return true;
}

static VkResult wined3d_context_vk_create_vk_descriptor_set_layout(struct wined3d_device_vk *device_vk,
        const struct wined3d_vk_info *vk_info, const struct wined3d_pipeline_layout_key_vk *key,
        VkDescriptorSetLayout *vk_set_layout)
{
    VkDescriptorSetLayoutCreateInfo layout_desc;
    VkResult vr;

    layout_desc.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout_desc.pNext = NULL;
    layout_desc.flags = 0;
    layout_desc.bindingCount = key->binding_count;
    layout_desc.pBindings = key->bindings;

    if ((vr = VK_CALL(vkCreateDescriptorSetLayout(device_vk->vk_device, &layout_desc, NULL, vk_set_layout))) < 0)
        WARN("Failed to create Vulkan descriptor set layout, vr %s.\n", wined3d_debug_vkresult(vr));

    return vr;
}

struct wined3d_pipeline_layout_vk *wined3d_context_vk_get_pipeline_layout(
        struct wined3d_context_vk *context_vk, VkDescriptorSetLayoutBinding *bindings, SIZE_T binding_count)
{
    struct wined3d_device_vk *device_vk = wined3d_device_vk(context_vk->c.device);
    const struct wined3d_vk_info *vk_info = context_vk->vk_info;
    struct wined3d_pipeline_layout_key_vk key;
    struct wined3d_pipeline_layout_vk *layout;
    VkPipelineLayoutCreateInfo layout_desc;
    struct wine_rb_entry *entry;
    VkResult vr;

    key.bindings = bindings;
    key.binding_count = binding_count;
    if ((entry = wine_rb_get(&context_vk->pipeline_layouts, &key)))
        return WINE_RB_ENTRY_VALUE(entry, struct wined3d_pipeline_layout_vk, entry);

    if (!(layout = heap_alloc(sizeof(*layout))))
        return NULL;

    if (!(layout->key.bindings = heap_alloc(sizeof(*layout->key.bindings) * key.binding_count)))
    {
        heap_free(layout);
        return NULL;
    }
    memcpy(layout->key.bindings, key.bindings, sizeof(*layout->key.bindings) * key.binding_count);
    layout->key.binding_count = key.binding_count;

    if ((vr = wined3d_context_vk_create_vk_descriptor_set_layout(device_vk, vk_info, &key, &layout->vk_set_layout)))
    {
        WARN("Failed to create descriptor set layout, vr %s.\n", wined3d_debug_vkresult(vr));
        goto fail;
    }

    layout_desc.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layout_desc.pNext = NULL;
    layout_desc.flags = 0;
    layout_desc.setLayoutCount = 1;
    layout_desc.pSetLayouts = &layout->vk_set_layout;
    layout_desc.pushConstantRangeCount = 0;
    layout_desc.pPushConstantRanges = NULL;

    if ((vr = VK_CALL(vkCreatePipelineLayout(device_vk->vk_device,
            &layout_desc, NULL, &layout->vk_pipeline_layout))) < 0)
    {
        WARN("Failed to create Vulkan pipeline layout, vr %s.\n", wined3d_debug_vkresult(vr));
        VK_CALL(vkDestroyDescriptorSetLayout(device_vk->vk_device, layout->vk_set_layout, NULL));
        goto fail;
    }

    if (wine_rb_put(&context_vk->pipeline_layouts, &layout->key, &layout->entry) == -1)
    {
        ERR("Failed to insert pipeline layout.\n");
        VK_CALL(vkDestroyPipelineLayout(device_vk->vk_device, layout->vk_pipeline_layout, NULL));
        VK_CALL(vkDestroyDescriptorSetLayout(device_vk->vk_device, layout->vk_set_layout, NULL));
        goto fail;
    }

    return layout;

fail:
    heap_free(layout->key.bindings);
    heap_free(layout);
    return NULL;
}

static VkPipeline wined3d_context_vk_get_graphics_pipeline(struct wined3d_context_vk *context_vk)
{
    struct wined3d_device_vk *device_vk = wined3d_device_vk(context_vk->c.device);
    const struct wined3d_vk_info *vk_info = context_vk->vk_info;
    struct wined3d_graphics_pipeline_vk *pipeline_vk;
    struct wined3d_graphics_pipeline_key_vk *key;
    struct wine_rb_entry *entry;
    VkResult vr;

    key = &context_vk->graphics.pipeline_key_vk;
    if ((entry = wine_rb_get(&context_vk->graphics_pipelines, key)))
        return WINE_RB_ENTRY_VALUE(entry, struct wined3d_graphics_pipeline_vk, entry)->vk_pipeline;

    if (!(pipeline_vk = heap_alloc(sizeof(*pipeline_vk))))
        return VK_NULL_HANDLE;
    pipeline_vk->key = *key;

    if ((vr = VK_CALL(vkCreateGraphicsPipelines(device_vk->vk_device,
            VK_NULL_HANDLE, 1, &key->pipeline_desc, NULL, &pipeline_vk->vk_pipeline))) < 0)
    {
        WARN("Failed to create graphics pipeline, vr %s.\n", wined3d_debug_vkresult(vr));
        heap_free(pipeline_vk);
        return VK_NULL_HANDLE;
    }

    if (wine_rb_put(&context_vk->graphics_pipelines, &pipeline_vk->key, &pipeline_vk->entry) == -1)
        ERR("Failed to insert pipeline.\n");

    return pipeline_vk->vk_pipeline;
}

static void wined3d_context_vk_load_shader_resources(struct wined3d_context_vk *context_vk,
        const struct wined3d_state *state, enum wined3d_pipeline pipeline)
{
    struct wined3d_shader_descriptor_writes_vk *writes = &context_vk->descriptor_writes;
    const struct wined3d_shader_resource_bindings *bindings;
    const struct wined3d_shader_resource_binding *binding;
    struct wined3d_unordered_access_view_vk *uav_vk;
    struct wined3d_shader_resource_view_vk *srv_vk;
    struct wined3d_unordered_access_view *uav;
    struct wined3d_shader_resource_view *srv;
    struct wined3d_buffer_vk *buffer_vk;
    struct wined3d_sampler *sampler;
    struct wined3d_buffer *buffer;
    size_t i;

    switch (pipeline)
    {
        case WINED3D_PIPELINE_GRAPHICS:
            bindings = &context_vk->graphics.bindings;
            break;

        case WINED3D_PIPELINE_COMPUTE:
            bindings = &context_vk->compute.bindings;
            break;

        default:
            ERR("Invalid pipeline %#x.\n", pipeline);
            return;
    }

    writes->count = 0;
    for (i = 0; i < bindings->count; ++i)
    {
        binding = &bindings->bindings[i];

        switch (binding->shader_descriptor_type)
        {
            case WINED3D_SHADER_DESCRIPTOR_TYPE_CBV:
                if (!(buffer = state->cb[binding->shader_type][binding->resource_idx]))
                    break;

                buffer_vk = wined3d_buffer_vk(buffer);
                wined3d_buffer_load(buffer, &context_vk->c, state);
                if (!buffer_vk->bo_user.valid)
                {
                    if (pipeline == WINED3D_PIPELINE_GRAPHICS)
                        context_invalidate_state(&context_vk->c, STATE_GRAPHICS_CONSTANT_BUFFER(binding->shader_type));
                    else
                        context_invalidate_compute_state(&context_vk->c, STATE_COMPUTE_CONSTANT_BUFFER);
                }
                wined3d_buffer_vk_barrier(buffer_vk, context_vk, WINED3D_BIND_CONSTANT_BUFFER);
                break;

            case WINED3D_SHADER_DESCRIPTOR_TYPE_SRV:
                if (!(srv = state->shader_resource_view[binding->shader_type][binding->resource_idx]))
                    break;

                srv_vk = wined3d_shader_resource_view_vk(srv);
                if (srv->resource->type == WINED3D_RTYPE_BUFFER)
                {
                    if (!srv_vk->view_vk.bo_user.valid)
                    {
                        wined3d_shader_resource_view_vk_update(srv_vk, context_vk);
                        if (pipeline == WINED3D_PIPELINE_GRAPHICS)
                            context_invalidate_state(&context_vk->c, STATE_GRAPHICS_SHADER_RESOURCE_BINDING);
                        else
                            context_invalidate_compute_state(&context_vk->c, STATE_COMPUTE_SHADER_RESOURCE_BINDING);
                    }
                    wined3d_buffer_load(buffer_from_resource(srv->resource), &context_vk->c, state);
                }
                else
                {
                    wined3d_texture_load(texture_from_resource(srv->resource), &context_vk->c, FALSE);
                }
                wined3d_shader_resource_view_vk_barrier(srv_vk, context_vk, WINED3D_BIND_SHADER_RESOURCE);
                break;

            case WINED3D_SHADER_DESCRIPTOR_TYPE_UAV:
                if (!(uav = state->unordered_access_view[pipeline][binding->resource_idx]))
                    break;

                uav_vk = wined3d_unordered_access_view_vk(uav);
                if (uav->resource->type == WINED3D_RTYPE_BUFFER)
                {
                    if (!uav_vk->view_vk.bo_user.valid)
                    {
                        wined3d_unordered_access_view_vk_update(uav_vk, context_vk);
                        if (pipeline == WINED3D_PIPELINE_GRAPHICS)
                            context_invalidate_state(&context_vk->c, STATE_GRAPHICS_UNORDERED_ACCESS_VIEW_BINDING);
                        else
                            context_invalidate_compute_state(&context_vk->c,
                                    STATE_COMPUTE_UNORDERED_ACCESS_VIEW_BINDING);
                    }
                    wined3d_buffer_load(buffer_from_resource(uav->resource), &context_vk->c, state);
                    wined3d_unordered_access_view_invalidate_location(uav, ~WINED3D_LOCATION_BUFFER);
                }
                else
                {
                    wined3d_texture_load(texture_from_resource(uav->resource), &context_vk->c, FALSE);
                    wined3d_unordered_access_view_invalidate_location(uav, ~WINED3D_LOCATION_TEXTURE_RGB);
                }
                wined3d_unordered_access_view_vk_barrier(uav_vk, context_vk, WINED3D_BIND_UNORDERED_ACCESS);
                break;

            case WINED3D_SHADER_DESCRIPTOR_TYPE_UAV_COUNTER:
                break;

            case WINED3D_SHADER_DESCRIPTOR_TYPE_SAMPLER:
                if (!(sampler = state->sampler[binding->shader_type][binding->resource_idx]))
                    sampler = context_vk->c.device->null_sampler;
                break;

            default:
                ERR("Invalid descriptor type %#x.\n", binding->shader_descriptor_type);
                break;
        }
    }
}

VkCommandBuffer wined3d_context_vk_apply_draw_state(struct wined3d_context_vk *context_vk,
        const struct wined3d_state *state, struct wined3d_buffer_vk *indirect_vk, bool indexed)
{
    struct wined3d_device_vk *device_vk = wined3d_device_vk(context_vk->c.device);
    const struct wined3d_vk_info *vk_info = context_vk->vk_info;
    struct wined3d_rendertarget_view *dsv;
    struct wined3d_buffer_vk *buffer_vk;
    VkSampleCountFlagBits sample_count;
    VkCommandBuffer vk_command_buffer;
    struct wined3d_buffer *buffer;
    unsigned int i;

    if (wined3d_context_is_graphics_state_dirty(&context_vk->c, STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL))
            || wined3d_context_is_graphics_state_dirty(&context_vk->c, STATE_FRAMEBUFFER))
        context_vk->c.shader_update_mask |= (1u << WINED3D_SHADER_TYPE_PIXEL);
    if (wined3d_context_is_graphics_state_dirty(&context_vk->c, STATE_SHADER(WINED3D_SHADER_TYPE_VERTEX)))
        context_vk->c.shader_update_mask |= (1u << WINED3D_SHADER_TYPE_VERTEX);
    if (wined3d_context_is_graphics_state_dirty(&context_vk->c, STATE_SHADER(WINED3D_SHADER_TYPE_GEOMETRY)))
        context_vk->c.shader_update_mask |= (1u << WINED3D_SHADER_TYPE_GEOMETRY);
    if (wined3d_context_is_graphics_state_dirty(&context_vk->c, STATE_SHADER(WINED3D_SHADER_TYPE_HULL)))
        context_vk->c.shader_update_mask |= (1u << WINED3D_SHADER_TYPE_HULL) | (1u << WINED3D_SHADER_TYPE_DOMAIN);
    if (wined3d_context_is_graphics_state_dirty(&context_vk->c, STATE_SHADER(WINED3D_SHADER_TYPE_DOMAIN)))
        context_vk->c.shader_update_mask |= (1u << WINED3D_SHADER_TYPE_DOMAIN);

    context_vk->sample_count = 0;
    for (i = 0; i < ARRAY_SIZE(state->fb.render_targets); ++i)
    {
        struct wined3d_rendertarget_view *rtv;

        if (!(rtv = state->fb.render_targets[i]) || rtv->format->id == WINED3DFMT_NULL)
            continue;

        if (wined3d_blend_state_get_writemask(state->blend_state, i))
        {
            wined3d_rendertarget_view_load_location(rtv, &context_vk->c, rtv->resource->draw_binding);
            wined3d_rendertarget_view_invalidate_location(rtv, ~rtv->resource->draw_binding);
        }
        else
        {
            wined3d_rendertarget_view_prepare_location(rtv, &context_vk->c, rtv->resource->draw_binding);
        }

        sample_count = max(1, wined3d_resource_get_sample_count(rtv->resource));
        if (!context_vk->sample_count)
            context_vk->sample_count = sample_count;
        else if (context_vk->sample_count != sample_count)
            FIXME("Inconsistent sample counts (%u != %u).\n", context_vk->sample_count, sample_count);
    }

    if ((dsv = state->fb.depth_stencil))
    {
        if (wined3d_state_uses_depth_buffer(state))
            wined3d_rendertarget_view_load_location(dsv, &context_vk->c, dsv->resource->draw_binding);
        else
            wined3d_rendertarget_view_prepare_location(dsv, &context_vk->c, dsv->resource->draw_binding);
        if (!state->depth_stencil_state || state->depth_stencil_state->desc.depth_write)
            wined3d_rendertarget_view_invalidate_location(dsv, ~dsv->resource->draw_binding);

        sample_count = max(1, wined3d_resource_get_sample_count(dsv->resource));
        if (!context_vk->sample_count)
            context_vk->sample_count = sample_count;
        else if (context_vk->sample_count != sample_count)
            FIXME("Inconsistent sample counts (%u != %u).\n", context_vk->sample_count, sample_count);
    }

    if (!context_vk->sample_count)
        context_vk->sample_count = VK_SAMPLE_COUNT_1_BIT;
    if (context_vk->c.shader_update_mask & ~(1u << WINED3D_SHADER_TYPE_COMPUTE))
    {
        device_vk->d.shader_backend->shader_select(device_vk->d.shader_priv, &context_vk->c, state);
        if (!context_vk->graphics.vk_pipeline_layout)
        {
            ERR("No pipeline layout set.\n");
            return VK_NULL_HANDLE;
        }
        context_vk->c.update_shader_resource_bindings = 1;
        context_vk->c.update_unordered_access_view_bindings = 1;
    }

    wined3d_context_vk_load_shader_resources(context_vk, state, WINED3D_PIPELINE_GRAPHICS);

    for (i = 0; i < ARRAY_SIZE(state->streams); ++i)
    {
        if (!(buffer = state->streams[i].buffer))
            continue;

        buffer_vk = wined3d_buffer_vk(buffer);
        wined3d_buffer_load(&buffer_vk->b, &context_vk->c, state);
        wined3d_buffer_vk_barrier(buffer_vk, context_vk, WINED3D_BIND_VERTEX_BUFFER);
        if (!buffer_vk->bo_user.valid)
            context_invalidate_state(&context_vk->c, STATE_STREAMSRC);
    }

    if (use_transform_feedback(state) && vk_info->supported[WINED3D_VK_EXT_TRANSFORM_FEEDBACK])
    {
        for (i = 0; i < ARRAY_SIZE(state->stream_output); ++i)
        {
            if (!(buffer = state->stream_output[i].buffer))
                continue;

            buffer_vk = wined3d_buffer_vk(buffer);
            wined3d_buffer_load(&buffer_vk->b, &context_vk->c, state);
            wined3d_buffer_vk_barrier(buffer_vk, context_vk, WINED3D_BIND_STREAM_OUTPUT);
            wined3d_buffer_invalidate_location(&buffer_vk->b, ~WINED3D_LOCATION_BUFFER);
            if (!buffer_vk->bo_user.valid)
                context_vk->update_stream_output = 1;
        }
        context_vk->c.transform_feedback_active = 1;
    }

    if (indexed || (wined3d_context_is_graphics_state_dirty(&context_vk->c, STATE_INDEXBUFFER) && state->index_buffer))
    {
        buffer_vk = wined3d_buffer_vk(state->index_buffer);
        wined3d_buffer_load(&buffer_vk->b, &context_vk->c, state);
        wined3d_buffer_vk_barrier(buffer_vk, context_vk, WINED3D_BIND_INDEX_BUFFER);
        if (!buffer_vk->bo_user.valid)
            context_invalidate_state(&context_vk->c, STATE_INDEXBUFFER);
    }

    if (indirect_vk)
    {
        wined3d_buffer_load(&indirect_vk->b, &context_vk->c, state);
        wined3d_buffer_vk_barrier(indirect_vk, context_vk, WINED3D_BIND_INDIRECT_BUFFER);
    }

    if (!(vk_command_buffer = wined3d_context_vk_get_command_buffer(context_vk)))
    {
        ERR("Failed to get command buffer.\n");
        return VK_NULL_HANDLE;
    }

    if (wined3d_context_is_graphics_state_dirty(&context_vk->c, STATE_FRAMEBUFFER))
        wined3d_context_vk_end_current_render_pass(context_vk);
    if (!wined3d_context_vk_begin_render_pass(context_vk, vk_command_buffer, state, vk_info))
    {
        ERR("Failed to begin render pass.\n");
        return VK_NULL_HANDLE;
    }

    if (wined3d_context_vk_update_graphics_pipeline_key(context_vk, state, context_vk->graphics.vk_pipeline_layout)
            || !context_vk->graphics.vk_pipeline)
    {
        if (!(context_vk->graphics.vk_pipeline = wined3d_context_vk_get_graphics_pipeline(context_vk)))
        {
            ERR("Failed to get graphics pipeline.\n");
            return VK_NULL_HANDLE;
        }

        VK_CALL(vkCmdBindPipeline(vk_command_buffer,
                VK_PIPELINE_BIND_POINT_GRAPHICS, context_vk->graphics.vk_pipeline));
    }

    if (wined3d_context_is_graphics_state_dirty(&context_vk->c, STATE_STENCIL_REF) && dsv)
    {
        VK_CALL(vkCmdSetStencilReference(vk_command_buffer, VK_STENCIL_FACE_FRONT_AND_BACK,
            state->stencil_ref & ((1 << dsv->format->stencil_size) - 1)));
    }

    if (wined3d_context_is_graphics_state_dirty(&context_vk->c, STATE_STREAMSRC))
        wined3d_context_vk_bind_vertex_buffers(context_vk, vk_command_buffer, state, vk_info);

    if (wined3d_context_is_graphics_state_dirty(&context_vk->c, STATE_STREAM_OUTPUT))
    {
        context_vk->update_stream_output = 1;
        context_vk->c.transform_feedback_paused = 0;
    }
    if (context_vk->c.transform_feedback_active && context_vk->update_stream_output)
    {
        wined3d_context_vk_bind_stream_output_buffers(context_vk, vk_command_buffer, state, vk_info);
        context_vk->update_stream_output = 0;
    }

    if (wined3d_context_is_graphics_state_dirty(&context_vk->c, STATE_INDEXBUFFER) && state->index_buffer)
    {
        const VkDescriptorBufferInfo *buffer_info;
        VkIndexType idx_type;

        if (state->index_format == WINED3DFMT_R16_UINT)
            idx_type = VK_INDEX_TYPE_UINT16;
        else
            idx_type = VK_INDEX_TYPE_UINT32;
        buffer_vk = wined3d_buffer_vk(state->index_buffer);
        buffer_info = wined3d_buffer_vk_get_buffer_info(buffer_vk);
        wined3d_context_vk_reference_bo(context_vk, &buffer_vk->bo);
        VK_CALL(vkCmdBindIndexBuffer(vk_command_buffer, buffer_info->buffer,
                buffer_info->offset + state->index_offset, idx_type));
    }

    if (wined3d_context_is_graphics_state_dirty(&context_vk->c, STATE_CONSTANT_BUFFER(WINED3D_SHADER_TYPE_PIXEL))
            || wined3d_context_is_graphics_state_dirty(&context_vk->c,
            STATE_CONSTANT_BUFFER(WINED3D_SHADER_TYPE_VERTEX))
            || wined3d_context_is_graphics_state_dirty(&context_vk->c,
            STATE_CONSTANT_BUFFER(WINED3D_SHADER_TYPE_GEOMETRY))
            || wined3d_context_is_graphics_state_dirty(&context_vk->c,
            STATE_CONSTANT_BUFFER(WINED3D_SHADER_TYPE_HULL))
            || wined3d_context_is_graphics_state_dirty(&context_vk->c,
            STATE_CONSTANT_BUFFER(WINED3D_SHADER_TYPE_DOMAIN))
            || wined3d_context_is_graphics_state_dirty(&context_vk->c, STATE_GRAPHICS_SHADER_RESOURCE_BINDING))
        context_vk->c.update_shader_resource_bindings = 1;
    if (wined3d_context_is_graphics_state_dirty(&context_vk->c, STATE_GRAPHICS_UNORDERED_ACCESS_VIEW_BINDING))
        context_vk->c.update_unordered_access_view_bindings = 1;

    if (context_vk->c.update_shader_resource_bindings || context_vk->c.update_unordered_access_view_bindings)
    {
        if (!wined3d_context_vk_update_descriptors(context_vk, vk_command_buffer, state, WINED3D_PIPELINE_GRAPHICS))
        {
            ERR("Failed to update shader descriptors.\n");
            return VK_NULL_HANDLE;
        }

        context_vk->c.update_shader_resource_bindings = 0;
        context_vk->c.update_unordered_access_view_bindings = 0;
    }

    if (wined3d_context_is_graphics_state_dirty(&context_vk->c, STATE_BLEND_FACTOR))
        VK_CALL(vkCmdSetBlendConstants(vk_command_buffer, &state->blend_factor.r));

    memset(context_vk->c.dirty_graphics_states, 0, sizeof(context_vk->c.dirty_graphics_states));
    context_vk->c.shader_update_mask &= 1u << WINED3D_SHADER_TYPE_COMPUTE;

    return vk_command_buffer;
}

VkCommandBuffer wined3d_context_vk_apply_compute_state(struct wined3d_context_vk *context_vk,
        const struct wined3d_state *state, struct wined3d_buffer_vk *indirect_vk)
{
    struct wined3d_device_vk *device_vk = wined3d_device_vk(context_vk->c.device);
    const struct wined3d_vk_info *vk_info = context_vk->vk_info;
    VkCommandBuffer vk_command_buffer;

    wined3d_context_vk_end_current_render_pass(context_vk);

    if (wined3d_context_is_compute_state_dirty(&context_vk->c, STATE_COMPUTE_SHADER))
        context_vk->c.shader_update_mask |= 1u << WINED3D_SHADER_TYPE_COMPUTE;

    if (context_vk->c.shader_update_mask & (1u << WINED3D_SHADER_TYPE_COMPUTE))
    {
        device_vk->d.shader_backend->shader_select_compute(device_vk->d.shader_priv, &context_vk->c, state);
        if (!context_vk->compute.vk_pipeline)
        {
            ERR("No compute pipeline set.\n");
            return VK_NULL_HANDLE;
        }
        context_vk->c.update_compute_shader_resource_bindings = 1;
        context_vk->c.update_compute_unordered_access_view_bindings = 1;
        context_vk->update_compute_pipeline = 1;
    }

    wined3d_context_vk_load_shader_resources(context_vk, state, WINED3D_PIPELINE_COMPUTE);

    if (indirect_vk)
    {
        wined3d_buffer_load_location(&indirect_vk->b, &context_vk->c, WINED3D_LOCATION_BUFFER);
        wined3d_buffer_vk_barrier(indirect_vk, context_vk, WINED3D_BIND_INDIRECT_BUFFER);
    }

    if (!(vk_command_buffer = wined3d_context_vk_get_command_buffer(context_vk)))
    {
        ERR("Failed to get command buffer.\n");
        return VK_NULL_HANDLE;
    }

    if (context_vk->update_compute_pipeline)
    {
        VK_CALL(vkCmdBindPipeline(vk_command_buffer,
                VK_PIPELINE_BIND_POINT_COMPUTE, context_vk->compute.vk_pipeline));
        context_vk->update_compute_pipeline = 0;
    }

    if (wined3d_context_is_compute_state_dirty(&context_vk->c, STATE_COMPUTE_CONSTANT_BUFFER)
            || wined3d_context_is_compute_state_dirty(&context_vk->c, STATE_COMPUTE_SHADER_RESOURCE_BINDING))
        context_vk->c.update_compute_shader_resource_bindings = 1;
    if (wined3d_context_is_compute_state_dirty(&context_vk->c, STATE_COMPUTE_UNORDERED_ACCESS_VIEW_BINDING))
        context_vk->c.update_compute_unordered_access_view_bindings = 1;

    if (context_vk->c.update_compute_shader_resource_bindings
            || context_vk->c.update_compute_unordered_access_view_bindings)
    {
        if (!wined3d_context_vk_update_descriptors(context_vk, vk_command_buffer, state, WINED3D_PIPELINE_COMPUTE))
        {
            ERR("Failed to update shader descriptors.\n");
            return VK_NULL_HANDLE;
        }

        context_vk->c.update_compute_shader_resource_bindings = 0;
        context_vk->c.update_compute_unordered_access_view_bindings = 0;
    }

    memset(context_vk->c.dirty_compute_states, 0, sizeof(context_vk->c.dirty_compute_states));
    context_vk->c.shader_update_mask &= ~(1u << WINED3D_SHADER_TYPE_COMPUTE);

    return vk_command_buffer;
}

HRESULT wined3d_context_vk_init(struct wined3d_context_vk *context_vk, struct wined3d_swapchain *swapchain)
{
    VkCommandPoolCreateInfo command_pool_info;
    const struct wined3d_vk_info *vk_info;
    struct wined3d_adapter_vk *adapter_vk;
    struct wined3d_device_vk *device_vk;
    VkResult vr;

    TRACE("context_vk %p, swapchain %p.\n", context_vk, swapchain);

    memset(context_vk, 0, sizeof(*context_vk));
    wined3d_context_init(&context_vk->c, swapchain);
    device_vk = wined3d_device_vk(swapchain->device);
    adapter_vk = wined3d_adapter_vk(device_vk->d.adapter);
    context_vk->vk_info = vk_info = &adapter_vk->vk_info;

    command_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    command_pool_info.pNext = NULL;
    command_pool_info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    command_pool_info.queueFamilyIndex = device_vk->vk_queue_family_index;
    if ((vr = VK_CALL(vkCreateCommandPool(device_vk->vk_device,
            &command_pool_info, NULL, &context_vk->vk_command_pool))) < 0)
    {
        ERR("Failed to create Vulkan command pool, vr %s.\n", wined3d_debug_vkresult(vr));
        wined3d_context_cleanup(&context_vk->c);
        return E_FAIL;
    }
    context_vk->current_command_buffer.id = 1;

    wined3d_context_vk_init_graphics_pipeline_key(context_vk);

    list_init(&context_vk->active_queries);
    list_init(&context_vk->free_occlusion_query_pools);
    list_init(&context_vk->free_timestamp_query_pools);
    list_init(&context_vk->free_pipeline_statistics_query_pools);
    list_init(&context_vk->free_stream_output_statistics_query_pools);

    wine_rb_init(&context_vk->render_passes, wined3d_render_pass_vk_compare);
    wine_rb_init(&context_vk->pipeline_layouts, wined3d_pipeline_layout_vk_compare);
    wine_rb_init(&context_vk->graphics_pipelines, wined3d_graphics_pipeline_vk_compare);
    wine_rb_init(&context_vk->bo_slab_available, wined3d_bo_slab_vk_compare);

    return WINED3D_OK;
}
