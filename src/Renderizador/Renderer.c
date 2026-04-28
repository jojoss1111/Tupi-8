// Renderer.c — TupiEngine (SDL2 + Vulkan)

#include "Renderer.h"
#include "../Inputs/Inputs.h"
#include "../Sprites/Sprites.h"
#include "../Camera/Camera.h"

#include <SDL2/SDL_vulkan.h>
#include <vulkan/vulkan.h>

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// --- Estado interno da janela ---

static SDL_Window* _janela = NULL;

static int   _largura  = 800;  // drawable size atual (px)
static int   _altura   = 600;
static int   _logico_w = 800;  // tamanho lógico do mundo
static int   _logico_h = 600;

static float _fundo_r = 0.1f, _fundo_g = 0.1f, _fundo_b = 0.15f;
static float _cor_r   = 1.0f, _cor_g   = 1.0f, _cor_b   = 1.0f, _cor_a = 1.0f;

static double _tempo_inicio   = 0.0;
static double _tempo_anterior = 0.0;
static double _delta          = 0.0;

static int   _flag_sem_borda = 0;
static float _escala         = 1.0f;
static int   _janela_aberta  = 0;

// --- Letterbox ---

static int _letterbox_ativo = 0;
static int _lb_x = 0, _lb_y = 0, _lb_w = 0, _lb_h = 0;
static int _framebuffer_resized = 0;

static void _calcular_letterbox(int fw, int fh) {
    float razao_alvo = (float)_logico_w / (float)_logico_h;
    float razao_tela = (float)fw        / (float)fh;

    if (razao_tela >= razao_alvo) {
        _lb_h = fh;
        _lb_w = (int)roundf((float)fh * razao_alvo);
        _lb_x = (fw - _lb_w) / 2;
        _lb_y = 0;
    } else {
        _lb_w = fw;
        _lb_h = (int)roundf((float)fw / razao_alvo);
        _lb_x = 0;
        _lb_y = (fh - _lb_h) / 2;
    }
}

// --- Limitador de FPS ---

static int    _fps_limite  = 0;
static float  _fps_atual   = 0.0f;
static Uint64 _tick_frame  = 0;

void tupi_fps_limite(int fps_max) {
    _fps_limite = (fps_max > 0) ? fps_max : 0;
}

float tupi_fps_atual(void) {
    return _fps_atual;
}

static void _fps_frame_inicio(void) {
    _tick_frame = SDL_GetPerformanceCounter();
}

static void _fps_frame_fim(void) {
    Uint64 freq  = SDL_GetPerformanceFrequency();
    Uint64 agora = SDL_GetPerformanceCounter();

    double frame_seg = (double)(agora - _tick_frame) / (double)freq;
    float fps_medido = (frame_seg > 0.0) ? (float)(1.0 / frame_seg) : 9999.0f;
    _fps_atual = _fps_atual * 0.9f + fps_medido * 0.1f;

    if (_fps_limite > 0) {
        double alvo_seg = 1.0 / (double)_fps_limite;
        double restante = alvo_seg - frame_seg;

        if (restante > 0.001) {
            double dormir_ms = restante * 1000.0 - 1.5;
            if (dormir_ms > 0.0)
                SDL_Delay((Uint32)dormir_ms);

            while (1) {
                double elapsed = (double)(SDL_GetPerformanceCounter() - _tick_frame) / (double)freq;
                if (elapsed >= alvo_seg) break;
            }
        }
    }
}

// --- Vulkan ---

#define TUPI_MAX_FRAMES_IN_FLIGHT 2
#define TUPI_INITIAL_VERTEX_CAPACITY 8192u
#define TUPI_INITIAL_PACKET_CAPACITY 1024u
#define TUPI_MAX_TEXTURES 4096u

typedef struct {
    float x, y;
    float r, g, b, a;
    float u, v;
} TupiGPUVertice;

typedef struct {
    uint32_t textura_id;
    uint32_t primeiro_vertice;
    uint32_t total_vertices;
} TupiDrawPacketGPU;

typedef struct {
    uint32_t graphics_family;
    uint32_t present_family;
    int      has_graphics;
    int      has_present;
} TupiQueueFamilies;

typedef struct {
    VkSurfaceCapabilitiesKHR capabilities;
    VkSurfaceFormatKHR*      formats;
    uint32_t                 format_count;
    VkPresentModeKHR*        present_modes;
    uint32_t                 present_mode_count;
} TupiSwapchainSupport;

typedef struct {
    VkImage           image;
    VkDeviceMemory    memory;
    VkImageView       view;
    VkDescriptorSet   descriptor_set;
    uint32_t          largura;
    uint32_t          altura;
    int               ativo;
} TupiTextureVulkan;

typedef struct {
    VkSemaphore image_available;
    VkSemaphore render_finished;
    VkFence     in_flight;
    VkCommandBuffer command_buffer;
} TupiFrameVulkan;

typedef struct {
    VkInstance instance;
    VkPhysicalDevice physical_device;
    VkDevice device;
    VkSurfaceKHR surface;
    VkQueue graphics_queue;
    VkQueue present_queue;
    TupiQueueFamilies queues;

    VkSwapchainKHR swapchain;
    VkFormat       swapchain_format;
    VkExtent2D     swapchain_extent;
    VkImage*       swapchain_images;
    VkImageView*   swapchain_image_views;
    VkFramebuffer* swapchain_framebuffers;
    uint32_t       swapchain_image_count;

    VkRenderPass           render_pass;
    VkDescriptorSetLayout  descriptor_set_layout;
    VkPipelineLayout       pipeline_layout;
    VkPipeline             pipeline;
    VkSampler              sampler;
    VkDescriptorPool       descriptor_pool;

    VkCommandPool command_pool;
    TupiFrameVulkan frames[TUPI_MAX_FRAMES_IN_FLIGHT];
    VkFence* images_in_flight;
    uint32_t frame_atual;

    VkBuffer       vertex_buffer;
    VkDeviceMemory vertex_buffer_memory;
    void*          vertex_buffer_mapeado;
    uint32_t       vertex_capacity;

    TupiGPUVertice*    frame_vertices;
    uint32_t           frame_vertex_count;
    uint32_t           frame_vertex_capacity;
    TupiDrawPacketGPU* frame_packets;
    uint32_t           frame_packet_count;
    uint32_t           frame_packet_capacity;

    TupiTextureVulkan* texturas;
    uint32_t           texturas_cap;
    uint32_t           textura_branca_id;

    int pronto;
} TupiVulkanRenderer;

static TupiVulkanRenderer _vk = {0};

typedef struct {
    float proj[16];
} TupiPushConstantes;

typedef struct {
    unsigned char* pixels;
    int largura, altura, tamanho;
} TupiImagemRust;

extern TupiImagemRust* tupi_imagem_carregar_seguro(const char* caminho);
extern void            tupi_imagem_destruir(TupiImagemRust* img);

static const char* TUPI_SHADER_VERT_PATH = "src/Renderizador/shaders/tupi2d.vert.spv";
static const char* TUPI_SHADER_FRAG_PATH = "src/Renderizador/shaders/tupi2d.frag.spv";

static inline Uint8 _f2u(float v) {
    return (Uint8)(v < 0.0f ? 0 : v > 1.0f ? 255 : v * 255.0f);
}

static uint32_t _clamp_u32(uint32_t v, uint32_t min_v) {
    return v < min_v ? min_v : v;
}

static void _vk_reset_frame_cpu(void) {
    _vk.frame_vertex_count = 0;
    _vk.frame_packet_count = 0;
}

static void _vk_destroy_buffer(VkBuffer* buffer, VkDeviceMemory* memory) {
    if (_vk.device == VK_NULL_HANDLE) return;
    if (*buffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(_vk.device, *buffer, NULL);
        *buffer = VK_NULL_HANDLE;
    }
    if (*memory != VK_NULL_HANDLE) {
        vkFreeMemory(_vk.device, *memory, NULL);
        *memory = VK_NULL_HANDLE;
    }
}

static void _vk_get_drawable_size(int* w, int* h) {
    int dw = 0, dh = 0;
    if (_janela) {
        SDL_Vulkan_GetDrawableSize(_janela, &dw, &dh);
    }
    if (dw <= 0 || dh <= 0) {
        SDL_GetWindowSize(_janela, &dw, &dh);
    }
    if (w) *w = dw;
    if (h) *h = dh;
}

static void _configurar_projecao(int largura, int altura) {
    if (_letterbox_ativo) {
        _calcular_letterbox(largura, altura);
    } else {
        _lb_x = 0;
        _lb_y = 0;
        _lb_w = largura;
        _lb_h = altura;
    }

    TupiCamera* cam = tupi_camera_ativa();
    if (cam) {
        tupi_camera_frame(cam, _logico_w, _logico_h);
    } else {
        TupiMatriz proj = tupi_projecao_ortografica(_logico_w, _logico_h);
        tupi_sprite_set_viewport(_logico_w, _logico_h);
        tupi_sprite_set_projecao(proj.m);
    }
}

static int _vk_find_memory_type(uint32_t type_filter, VkMemoryPropertyFlags properties, uint32_t* out_index) {
    VkPhysicalDeviceMemoryProperties mem_props;
    vkGetPhysicalDeviceMemoryProperties(_vk.physical_device, &mem_props);

    for (uint32_t i = 0; i < mem_props.memoryTypeCount; i++) {
        if ((type_filter & (1u << i)) &&
            (mem_props.memoryTypes[i].propertyFlags & properties) == properties) {
            *out_index = i;
            return 1;
        }
    }
    return 0;
}

static int _vk_create_buffer(
    VkDeviceSize size,
    VkBufferUsageFlags usage,
    VkMemoryPropertyFlags properties,
    VkBuffer* out_buffer,
    VkDeviceMemory* out_memory)
{
    VkBufferCreateInfo buffer_info = {0};
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size = size;
    buffer_info.usage = usage;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(_vk.device, &buffer_info, NULL, out_buffer) != VK_SUCCESS) {
        return 0;
    }

    VkMemoryRequirements mem_req;
    vkGetBufferMemoryRequirements(_vk.device, *out_buffer, &mem_req);

    uint32_t memory_type = 0;
    if (!_vk_find_memory_type(mem_req.memoryTypeBits, properties, &memory_type)) {
        vkDestroyBuffer(_vk.device, *out_buffer, NULL);
        *out_buffer = VK_NULL_HANDLE;
        return 0;
    }

    VkMemoryAllocateInfo alloc_info = {0};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = mem_req.size;
    alloc_info.memoryTypeIndex = memory_type;

    if (vkAllocateMemory(_vk.device, &alloc_info, NULL, out_memory) != VK_SUCCESS) {
        vkDestroyBuffer(_vk.device, *out_buffer, NULL);
        *out_buffer = VK_NULL_HANDLE;
        return 0;
    }

    vkBindBufferMemory(_vk.device, *out_buffer, *out_memory, 0);
    return 1;
}

static VkCommandBuffer _vk_begin_one_time_commands(void) {
    VkCommandBufferAllocateInfo alloc_info = {0};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandPool = _vk.command_pool;
    alloc_info.commandBufferCount = 1;

    VkCommandBuffer cmd = VK_NULL_HANDLE;
    if (vkAllocateCommandBuffers(_vk.device, &alloc_info, &cmd) != VK_SUCCESS) {
        return VK_NULL_HANDLE;
    }

    VkCommandBufferBeginInfo begin_info = {0};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &begin_info);
    return cmd;
}

static int _vk_end_one_time_commands(VkCommandBuffer cmd) {
    if (cmd == VK_NULL_HANDLE) return 0;

    if (vkEndCommandBuffer(cmd) != VK_SUCCESS) {
        vkFreeCommandBuffers(_vk.device, _vk.command_pool, 1, &cmd);
        return 0;
    }

    VkSubmitInfo submit_info = {0};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &cmd;

    if (vkQueueSubmit(_vk.graphics_queue, 1, &submit_info, VK_NULL_HANDLE) != VK_SUCCESS) {
        vkFreeCommandBuffers(_vk.device, _vk.command_pool, 1, &cmd);
        return 0;
    }

    vkQueueWaitIdle(_vk.graphics_queue);
    vkFreeCommandBuffers(_vk.device, _vk.command_pool, 1, &cmd);
    return 1;
}

static void _vk_transition_image_layout(
    VkCommandBuffer cmd,
    VkImage image,
    VkImageLayout old_layout,
    VkImageLayout new_layout)
{
    VkImageMemoryBarrier barrier = {0};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = old_layout;
    barrier.newLayout = new_layout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags src_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    VkPipelineStageFlags dst_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;

    if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED &&
        new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        src_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dst_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
               new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        src_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        dst_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }

    vkCmdPipelineBarrier(
        cmd,
        src_stage, dst_stage,
        0,
        0, NULL,
        0, NULL,
        1, &barrier
    );
}

static int _vk_create_image(
    uint32_t width,
    uint32_t height,
    VkFormat format,
    VkImageTiling tiling,
    VkImageUsageFlags usage,
    VkMemoryPropertyFlags properties,
    VkImage* out_image,
    VkDeviceMemory* out_memory)
{
    VkImageCreateInfo image_info = {0};
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.extent.width = width;
    image_info.extent.height = height;
    image_info.extent.depth = 1;
    image_info.mipLevels = 1;
    image_info.arrayLayers = 1;
    image_info.format = format;
    image_info.tiling = tiling;
    image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_info.usage = usage;
    image_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(_vk.device, &image_info, NULL, out_image) != VK_SUCCESS) {
        return 0;
    }

    VkMemoryRequirements mem_req;
    vkGetImageMemoryRequirements(_vk.device, *out_image, &mem_req);

    uint32_t memory_type = 0;
    if (!_vk_find_memory_type(mem_req.memoryTypeBits, properties, &memory_type)) {
        vkDestroyImage(_vk.device, *out_image, NULL);
        *out_image = VK_NULL_HANDLE;
        return 0;
    }

    VkMemoryAllocateInfo alloc_info = {0};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = mem_req.size;
    alloc_info.memoryTypeIndex = memory_type;

    if (vkAllocateMemory(_vk.device, &alloc_info, NULL, out_memory) != VK_SUCCESS) {
        vkDestroyImage(_vk.device, *out_image, NULL);
        *out_image = VK_NULL_HANDLE;
        return 0;
    }

    vkBindImageMemory(_vk.device, *out_image, *out_memory, 0);
    return 1;
}

static VkImageView _vk_create_image_view(VkImage image, VkFormat format) {
    VkImageViewCreateInfo view_info = {0};
    view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_info.image = image;
    view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view_info.format = format;
    view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    view_info.subresourceRange.baseMipLevel = 0;
    view_info.subresourceRange.levelCount = 1;
    view_info.subresourceRange.baseArrayLayer = 0;
    view_info.subresourceRange.layerCount = 1;

    VkImageView view = VK_NULL_HANDLE;
    if (vkCreateImageView(_vk.device, &view_info, NULL, &view) != VK_SUCCESS) {
        return VK_NULL_HANDLE;
    }
    return view;
}

static int _vk_copy_buffer_to_image(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
    VkCommandBuffer cmd = _vk_begin_one_time_commands();
    if (cmd == VK_NULL_HANDLE) return 0;

    _vk_transition_image_layout(cmd, image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    VkBufferImageCopy region = {0};
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageExtent.width = width;
    region.imageExtent.height = height;
    region.imageExtent.depth = 1;

    vkCmdCopyBufferToImage(cmd, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    _vk_transition_image_layout(cmd, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    return _vk_end_one_time_commands(cmd);
}

static void _vk_destroy_texture_slot(uint32_t idx) {
    if (!_vk.texturas || idx >= _vk.texturas_cap) return;
    TupiTextureVulkan* tex = &_vk.texturas[idx];
    if (!tex->ativo) return;

    if (tex->descriptor_set != VK_NULL_HANDLE) {
        vkFreeDescriptorSets(_vk.device, _vk.descriptor_pool, 1, &tex->descriptor_set);
        tex->descriptor_set = VK_NULL_HANDLE;
    }
    if (tex->view != VK_NULL_HANDLE) {
        vkDestroyImageView(_vk.device, tex->view, NULL);
        tex->view = VK_NULL_HANDLE;
    }
    if (tex->image != VK_NULL_HANDLE) {
        vkDestroyImage(_vk.device, tex->image, NULL);
        tex->image = VK_NULL_HANDLE;
    }
    if (tex->memory != VK_NULL_HANDLE) {
        vkFreeMemory(_vk.device, tex->memory, NULL);
        tex->memory = VK_NULL_HANDLE;
    }

    tex->largura = 0;
    tex->altura = 0;
    tex->ativo = 0;
}

static int _vk_garantir_texturas(uint32_t idx) {
    if (idx < _vk.texturas_cap) return 1;

    uint32_t novo = (_vk.texturas_cap == 0) ? 16u : _vk.texturas_cap;
    while (novo <= idx) novo *= 2u;
    if (novo > TUPI_MAX_TEXTURES) novo = TUPI_MAX_TEXTURES;
    if (idx >= novo) return 0;

    TupiTextureVulkan* tmp = (TupiTextureVulkan*)realloc(_vk.texturas, novo * sizeof(TupiTextureVulkan));
    if (!tmp) return 0;

    for (uint32_t i = _vk.texturas_cap; i < novo; i++) {
        tmp[i] = (TupiTextureVulkan){0};
    }
    _vk.texturas = tmp;
    _vk.texturas_cap = novo;
    return 1;
}

static uint32_t _vk_reservar_slot_textura(void) {
    for (uint32_t i = 1; i < _vk.texturas_cap; i++) {
        if (!_vk.texturas[i].ativo) return i;
    }
    uint32_t idx = (_vk.texturas_cap == 0) ? 1u : _vk.texturas_cap;
    return _vk_garantir_texturas(idx) ? idx : 0u;
}

static int _vk_atualizar_descriptor_textura(uint32_t idx) {
    TupiTextureVulkan* tex = &_vk.texturas[idx];

    VkDescriptorSetAllocateInfo alloc_info = {0};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorPool = _vk.descriptor_pool;
    alloc_info.descriptorSetCount = 1;
    alloc_info.pSetLayouts = &_vk.descriptor_set_layout;

    if (vkAllocateDescriptorSets(_vk.device, &alloc_info, &tex->descriptor_set) != VK_SUCCESS) {
        return 0;
    }

    VkDescriptorImageInfo image_info = {0};
    image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    image_info.imageView = tex->view;
    image_info.sampler = _vk.sampler;

    VkWriteDescriptorSet write = {0};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = tex->descriptor_set;
    write.dstBinding = 0;
    write.dstArrayElement = 0;
    write.descriptorCount = 1;
    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.pImageInfo = &image_info;

    vkUpdateDescriptorSets(_vk.device, 1, &write, 0, NULL);
    return 1;
}

static int _vk_criar_textura_rgba8_interna(
    const unsigned char* pixels,
    uint32_t largura,
    uint32_t altura,
    uint32_t* out_id)
{
    if (!pixels || largura == 0 || altura == 0) return 0;

    VkDeviceSize tamanho = (VkDeviceSize)largura * (VkDeviceSize)altura * 4u;
    VkBuffer staging_buffer = VK_NULL_HANDLE;
    VkDeviceMemory staging_memory = VK_NULL_HANDLE;
    void* mapped = NULL;

    if (!_vk_create_buffer(
            tamanho,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            &staging_buffer,
            &staging_memory)) {
        return 0;
    }

    if (vkMapMemory(_vk.device, staging_memory, 0, tamanho, 0, &mapped) != VK_SUCCESS) {
        _vk_destroy_buffer(&staging_buffer, &staging_memory);
        return 0;
    }
    memcpy(mapped, pixels, (size_t)tamanho);
    vkUnmapMemory(_vk.device, staging_memory);

    uint32_t idx = _vk_reservar_slot_textura();
    if (idx == 0) {
        _vk_destroy_buffer(&staging_buffer, &staging_memory);
        return 0;
    }

    TupiTextureVulkan* tex = &_vk.texturas[idx];

    if (!_vk_create_image(
            largura,
            altura,
            VK_FORMAT_R8G8B8A8_UNORM,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            &tex->image,
            &tex->memory)) {
        _vk_destroy_buffer(&staging_buffer, &staging_memory);
        return 0;
    }

    if (!_vk_copy_buffer_to_image(staging_buffer, tex->image, largura, altura)) {
        _vk_destroy_texture_slot(idx);
        _vk_destroy_buffer(&staging_buffer, &staging_memory);
        return 0;
    }

    tex->view = _vk_create_image_view(tex->image, VK_FORMAT_R8G8B8A8_UNORM);
    if (tex->view == VK_NULL_HANDLE) {
        _vk_destroy_texture_slot(idx);
        _vk_destroy_buffer(&staging_buffer, &staging_memory);
        return 0;
    }

    tex->largura = largura;
    tex->altura = altura;
    tex->ativo = 1;

    if (!_vk_atualizar_descriptor_textura(idx)) {
        _vk_destroy_texture_slot(idx);
        _vk_destroy_buffer(&staging_buffer, &staging_memory);
        return 0;
    }

    _vk_destroy_buffer(&staging_buffer, &staging_memory);
    *out_id = idx;
    return 1;
}

static int _vk_garantir_cpu_vertices(uint32_t extra) {
    if (_vk.frame_vertex_count + extra <= _vk.frame_vertex_capacity) return 1;

    uint32_t novo = (_vk.frame_vertex_capacity == 0) ? TUPI_INITIAL_VERTEX_CAPACITY : _vk.frame_vertex_capacity;
    while (_vk.frame_vertex_count + extra > novo) novo *= 2u;

    TupiGPUVertice* tmp = (TupiGPUVertice*)realloc(_vk.frame_vertices, novo * sizeof(TupiGPUVertice));
    if (!tmp) return 0;
    _vk.frame_vertices = tmp;
    _vk.frame_vertex_capacity = novo;
    return 1;
}

static int _vk_garantir_packets(uint32_t extra) {
    if (_vk.frame_packet_count + extra <= _vk.frame_packet_capacity) return 1;

    uint32_t novo = (_vk.frame_packet_capacity == 0) ? TUPI_INITIAL_PACKET_CAPACITY : _vk.frame_packet_capacity;
    while (_vk.frame_packet_count + extra > novo) novo *= 2u;

    TupiDrawPacketGPU* tmp = (TupiDrawPacketGPU*)realloc(_vk.frame_packets, novo * sizeof(TupiDrawPacketGPU));
    if (!tmp) return 0;
    _vk.frame_packets = tmp;
    _vk.frame_packet_capacity = novo;
    return 1;
}

static int _vk_garantir_vertex_buffer(uint32_t capacidade_vertices) {
    capacidade_vertices = _clamp_u32(capacidade_vertices, TUPI_INITIAL_VERTEX_CAPACITY);
    if (_vk.vertex_capacity >= capacidade_vertices && _vk.vertex_buffer_mapeado) return 1;

    VkBuffer novo_buffer = VK_NULL_HANDLE;
    VkDeviceMemory nova_memoria = VK_NULL_HANDLE;
    VkDeviceSize tamanho = (VkDeviceSize)capacidade_vertices * sizeof(TupiGPUVertice);

    if (!_vk_create_buffer(
            tamanho,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            &novo_buffer,
            &nova_memoria)) {
        return 0;
    }

    void* novo_mapeado = NULL;
    if (vkMapMemory(_vk.device, nova_memoria, 0, tamanho, 0, &novo_mapeado) != VK_SUCCESS) {
        vkDestroyBuffer(_vk.device, novo_buffer, NULL);
        vkFreeMemory(_vk.device, nova_memoria, NULL);
        return 0;
    }

    if (_vk.vertex_buffer_mapeado && _vk.frame_vertex_count > 0) {
        memcpy(novo_mapeado, _vk.frame_vertices, (size_t)_vk.frame_vertex_count * sizeof(TupiGPUVertice));
        vkUnmapMemory(_vk.device, _vk.vertex_buffer_memory);
    }

    _vk_destroy_buffer(&_vk.vertex_buffer, &_vk.vertex_buffer_memory);

    _vk.vertex_buffer = novo_buffer;
    _vk.vertex_buffer_memory = nova_memoria;
    _vk.vertex_buffer_mapeado = novo_mapeado;
    _vk.vertex_capacity = capacidade_vertices;
    return 1;
}

static void _vk_queue_vertices(uint32_t textura_id, const TupiGPUVertice* vertices, uint32_t count) {
    if (!_vk.pronto || count == 0 || !vertices) return;
    if (!_vk_garantir_cpu_vertices(count) || !_vk_garantir_packets(1)) return;

    uint32_t inicio = _vk.frame_vertex_count;
    memcpy(&_vk.frame_vertices[inicio], vertices, (size_t)count * sizeof(TupiGPUVertice));
    _vk.frame_vertex_count += count;

    if (_vk.frame_packet_count > 0) {
        TupiDrawPacketGPU* ultimo = &_vk.frame_packets[_vk.frame_packet_count - 1];
        if (ultimo->textura_id == textura_id &&
            ultimo->primeiro_vertice + ultimo->total_vertices == inicio) {
            ultimo->total_vertices += count;
            return;
        }
    }

    _vk.frame_packets[_vk.frame_packet_count++] = (TupiDrawPacketGPU){
        textura_id,
        inicio,
        count
    };
}

static void _vk_emitir_triangulo(
    uint32_t textura_id,
    float x1, float y1, float u1, float v1,
    float x2, float y2, float u2, float v2,
    float x3, float y3, float u3, float v3,
    float r, float g, float b, float a)
{
    TupiGPUVertice tri[3] = {
        {x1, y1, r, g, b, a, u1, v1},
        {x2, y2, r, g, b, a, u2, v2},
        {x3, y3, r, g, b, a, u3, v3},
    };
    _vk_queue_vertices(textura_id, tri, 3);
}

static void _vk_emitir_quad_texturizado(uint32_t textura_id, const float* quad, const float* cor) {
    if (!quad || !cor) return;

    TupiGPUVertice vertices[6];
    const float r = cor[0], g = cor[1], b = cor[2], a = cor[3];

    vertices[0] = (TupiGPUVertice){quad[0],  quad[1],  r, g, b, a, quad[2],  quad[3]};
    vertices[1] = (TupiGPUVertice){quad[4],  quad[5],  r, g, b, a, quad[6],  quad[7]};
    vertices[2] = (TupiGPUVertice){quad[8],  quad[9],  r, g, b, a, quad[10], quad[11]};
    vertices[3] = (TupiGPUVertice){quad[4],  quad[5],  r, g, b, a, quad[6],  quad[7]};
    vertices[4] = (TupiGPUVertice){quad[12], quad[13], r, g, b, a, quad[14], quad[15]};
    vertices[5] = (TupiGPUVertice){quad[8],  quad[9],  r, g, b, a, quad[10], quad[11]};

    _vk_queue_vertices(textura_id, vertices, 6);
}

static void _vk_emitir_linha(float x1, float y1, float x2, float y2, float espessura, const float cor[4]) {
    float dx = x2 - x1;
    float dy = y2 - y1;
    float len = sqrtf(dx * dx + dy * dy);
    if (len < 0.0001f) return;

    float half = (espessura <= 1.5f) ? 0.5f : espessura * 0.5f;
    float nx = -dy / len * half;
    float ny =  dx / len * half;

    _vk_emitir_triangulo(_vk.textura_branca_id,
        x1 + nx, y1 + ny, 0.0f, 0.0f,
        x1 - nx, y1 - ny, 0.0f, 0.0f,
        x2 - nx, y2 - ny, 0.0f, 0.0f,
        cor[0], cor[1], cor[2], cor[3]);

    _vk_emitir_triangulo(_vk.textura_branca_id,
        x1 + nx, y1 + ny, 0.0f, 0.0f,
        x2 - nx, y2 - ny, 0.0f, 0.0f,
        x2 + nx, y2 + ny, 0.0f, 0.0f,
        cor[0], cor[1], cor[2], cor[3]);
}

static void _vk_emitir_retangulo(float x, float y, float largura, float altura, const float cor[4]) {
    _vk_emitir_triangulo(_vk.textura_branca_id,
        x, y, 0.0f, 0.0f,
        x + largura, y, 0.0f, 0.0f,
        x, y + altura, 0.0f, 0.0f,
        cor[0], cor[1], cor[2], cor[3]);

    _vk_emitir_triangulo(_vk.textura_branca_id,
        x + largura, y, 0.0f, 0.0f,
        x + largura, y + altura, 0.0f, 0.0f,
        x, y + altura, 0.0f, 0.0f,
        cor[0], cor[1], cor[2], cor[3]);
}

static void _vk_emitir_triangulo_solido(
    float x1, float y1,
    float x2, float y2,
    float x3, float y3,
    const float cor[4])
{
    _vk_emitir_triangulo(_vk.textura_branca_id,
        x1, y1, 0.0f, 0.0f,
        x2, y2, 0.0f, 0.0f,
        x3, y3, 0.0f, 0.0f,
        cor[0], cor[1], cor[2], cor[3]);
}

static void _vk_emitir_circulo(float x, float y, float raio, int segmentos, const float cor[4]) {
    if (segmentos < 3) segmentos = 3;
    for (int i = 0; i < segmentos; i++) {
        float a0 = ((float)i / (float)segmentos) * 2.0f * (float)M_PI;
        float a1 = ((float)(i + 1) / (float)segmentos) * 2.0f * (float)M_PI;
        _vk_emitir_triangulo_solido(
            x, y,
            x + cosf(a0) * raio, y + sinf(a0) * raio,
            x + cosf(a1) * raio, y + sinf(a1) * raio,
            cor);
    }
}

static void _vk_emitir_circulo_borda(float x, float y, float raio, int segmentos, float espessura, const float cor[4]) {
    if (segmentos < 3) segmentos = 3;
    for (int i = 0; i < segmentos; i++) {
        float a0 = ((float)i / (float)segmentos) * 2.0f * (float)M_PI;
        float a1 = ((float)(i + 1) / (float)segmentos) * 2.0f * (float)M_PI;
        _vk_emitir_linha(
            x + cosf(a0) * raio, y + sinf(a0) * raio,
            x + cosf(a1) * raio, y + sinf(a1) * raio,
            espessura, cor);
    }
}

static void _flush_batcher(const TupiDrawCall* calls, int n) {
    for (int i = 0; i < n; i++) {
        const TupiDrawCall* dc = &calls[i];
        const float* v = dc->verts;
        const float cor[4] = {dc->cor[0], dc->cor[1], dc->cor[2], dc->cor[3]};

        switch (dc->primitiva) {
            case TUPI_RET:
                _vk_emitir_retangulo(v[0], v[1], v[2] - v[0], v[5] - v[1], cor);
                break;
            case TUPI_TRI:
                _vk_emitir_triangulo_solido(v[0], v[1], v[2], v[3], v[4], v[5], cor);
                break;
            case TUPI_LIN:
                _vk_emitir_linha(v[0], v[1], v[2], v[3], 1.0f, cor);
                break;
            default:
                break;
        }
    }
}

static int _vk_supports_device_extensions(VkPhysicalDevice device) {
    uint32_t count = 0;
    vkEnumerateDeviceExtensionProperties(device, NULL, &count, NULL);
    if (count == 0) return 0;

    VkExtensionProperties* props = (VkExtensionProperties*)malloc(count * sizeof(VkExtensionProperties));
    if (!props) return 0;
    vkEnumerateDeviceExtensionProperties(device, NULL, &count, props);

    int found = 0;
    for (uint32_t i = 0; i < count; i++) {
        if (strcmp(props[i].extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0) {
            found = 1;
            break;
        }
    }
    free(props);
    return found;
}

static TupiQueueFamilies _vk_find_queue_families(VkPhysicalDevice device) {
    TupiQueueFamilies families = {0};
    uint32_t count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &count, NULL);
    if (count == 0) return families;

    VkQueueFamilyProperties* props = (VkQueueFamilyProperties*)malloc(count * sizeof(VkQueueFamilyProperties));
    if (!props) return families;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &count, props);

    for (uint32_t i = 0; i < count; i++) {
        if (props[i].queueCount > 0 && (props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
            families.graphics_family = i;
            families.has_graphics = 1;
        }

        VkBool32 present = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, _vk.surface, &present);
        if (props[i].queueCount > 0 && present) {
            families.present_family = i;
            families.has_present = 1;
        }

        if (families.has_graphics && families.has_present) break;
    }

    free(props);
    return families;
}

static TupiSwapchainSupport _vk_query_swapchain_support(VkPhysicalDevice device) {
    TupiSwapchainSupport support = {0};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, _vk.surface, &support.capabilities);

    vkGetPhysicalDeviceSurfaceFormatsKHR(device, _vk.surface, &support.format_count, NULL);
    if (support.format_count > 0) {
        support.formats = (VkSurfaceFormatKHR*)malloc(support.format_count * sizeof(VkSurfaceFormatKHR));
        if (support.formats) {
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, _vk.surface, &support.format_count, support.formats);
        } else {
            support.format_count = 0;
        }
    }

    vkGetPhysicalDeviceSurfacePresentModesKHR(device, _vk.surface, &support.present_mode_count, NULL);
    if (support.present_mode_count > 0) {
        support.present_modes = (VkPresentModeKHR*)malloc(support.present_mode_count * sizeof(VkPresentModeKHR));
        if (support.present_modes) {
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, _vk.surface, &support.present_mode_count, support.present_modes);
        } else {
            support.present_mode_count = 0;
        }
    }

    return support;
}

static void _vk_free_swapchain_support(TupiSwapchainSupport* support) {
    free(support->formats);
    free(support->present_modes);
    support->formats = NULL;
    support->present_modes = NULL;
    support->format_count = 0;
    support->present_mode_count = 0;
}

static int _vk_score_device(VkPhysicalDevice device) {
    if (!_vk_supports_device_extensions(device)) return -1;

    TupiQueueFamilies families = _vk_find_queue_families(device);
    if (!families.has_graphics || !families.has_present) return -1;

    TupiSwapchainSupport support = _vk_query_swapchain_support(device);
    int adequado = support.format_count > 0 && support.present_mode_count > 0;
    _vk_free_swapchain_support(&support);
    if (!adequado) return -1;

    VkPhysicalDeviceProperties props;
    VkPhysicalDeviceFeatures features;
    vkGetPhysicalDeviceProperties(device, &props);
    vkGetPhysicalDeviceFeatures(device, &features);

    int score = 0;
    if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) score += 1000;
    if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) score += 500;
    score += (int)props.limits.maxImageDimension2D;
    if (!features.samplerAnisotropy) score -= 10;
    return score;
}

static VkSurfaceFormatKHR _vk_choose_surface_format(const VkSurfaceFormatKHR* formats, uint32_t count) {
    for (uint32_t i = 0; i < count; i++) {
        if (formats[i].format == VK_FORMAT_B8G8R8A8_UNORM &&
            formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return formats[i];
        }
    }
    for (uint32_t i = 0; i < count; i++) {
        if (formats[i].format == VK_FORMAT_R8G8B8A8_UNORM &&
            formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return formats[i];
        }
    }
    for (uint32_t i = 0; i < count; i++) {
        if (formats[i].format == VK_FORMAT_B8G8R8A8_SRGB &&
            formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return formats[i];
        }
    }
    return formats[0];
}

static VkPresentModeKHR _vk_choose_present_mode(const VkPresentModeKHR* modes, uint32_t count) {
    for (uint32_t i = 0; i < count; i++) {
        if (modes[i] == VK_PRESENT_MODE_MAILBOX_KHR) return modes[i];
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}

static VkExtent2D _vk_choose_extent(const VkSurfaceCapabilitiesKHR* caps) {
    if (caps->currentExtent.width != UINT32_MAX) {
        return caps->currentExtent;
    }

    int dw = 0, dh = 0;
    _vk_get_drawable_size(&dw, &dh);
    VkExtent2D extent = {
        (uint32_t)dw,
        (uint32_t)dh
    };
    if (extent.width < caps->minImageExtent.width) extent.width = caps->minImageExtent.width;
    if (extent.height < caps->minImageExtent.height) extent.height = caps->minImageExtent.height;
    if (extent.width > caps->maxImageExtent.width) extent.width = caps->maxImageExtent.width;
    if (extent.height > caps->maxImageExtent.height) extent.height = caps->maxImageExtent.height;
    return extent;
}

static int _vk_create_instance(void) {
    unsigned int ext_count = 0;
    if (!SDL_Vulkan_GetInstanceExtensions(_janela, &ext_count, NULL) || ext_count == 0) {
        fprintf(stderr, "[Renderer/Vulkan] SDL_Vulkan_GetInstanceExtensions falhou: %s\n", SDL_GetError());
        return 0;
    }

    const char** extensions = (const char**)malloc(ext_count * sizeof(const char*));
    if (!extensions) return 0;
    if (!SDL_Vulkan_GetInstanceExtensions(_janela, &ext_count, extensions)) {
        fprintf(stderr, "[Renderer/Vulkan] Falha ao obter extensoes Vulkan do SDL: %s\n", SDL_GetError());
        free(extensions);
        return 0;
    }

    VkApplicationInfo app_info = {0};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "TupiEngine";
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "TupiEngine";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo create_info = {0};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &app_info;
    create_info.enabledExtensionCount = ext_count;
    create_info.ppEnabledExtensionNames = extensions;

    VkResult res = vkCreateInstance(&create_info, NULL, &_vk.instance);
    free(extensions);
    if (res != VK_SUCCESS) {
        fprintf(stderr, "[Renderer/Vulkan] vkCreateInstance falhou (%d)\n", (int)res);
        return 0;
    }
    return 1;
}

static int _vk_pick_physical_device(void) {
    uint32_t count = 0;
    vkEnumeratePhysicalDevices(_vk.instance, &count, NULL);
    if (count == 0) {
        fprintf(stderr, "[Renderer/Vulkan] Nenhuma GPU com suporte a Vulkan foi encontrada.\n");
        return 0;
    }

    VkPhysicalDevice* devices = (VkPhysicalDevice*)malloc(count * sizeof(VkPhysicalDevice));
    if (!devices) return 0;
    vkEnumeratePhysicalDevices(_vk.instance, &count, devices);

    int melhor_score = -1;
    VkPhysicalDevice melhor = VK_NULL_HANDLE;
    TupiQueueFamilies melhor_familias = {0};

    for (uint32_t i = 0; i < count; i++) {
        int score = _vk_score_device(devices[i]);
        if (score > melhor_score) {
            melhor_score = score;
            melhor = devices[i];
            melhor_familias = _vk_find_queue_families(devices[i]);
        }
    }

    free(devices);

    if (melhor == VK_NULL_HANDLE) {
        fprintf(stderr, "[Renderer/Vulkan] Nenhuma GPU compativel com swapchain foi encontrada.\n");
        return 0;
    }

    _vk.physical_device = melhor;
    _vk.queues = melhor_familias;
    return 1;
}

static int _vk_create_device(void) {
    float priority = 1.0f;
    uint32_t familias[2] = {_vk.queues.graphics_family, _vk.queues.present_family};
    uint32_t unique_count = (_vk.queues.graphics_family == _vk.queues.present_family) ? 1u : 2u;

    VkDeviceQueueCreateInfo queue_infos[2] = {0};
    for (uint32_t i = 0; i < unique_count; i++) {
        queue_infos[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_infos[i].queueFamilyIndex = familias[i];
        queue_infos[i].queueCount = 1;
        queue_infos[i].pQueuePriorities = &priority;
    }

    const char* extensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    VkPhysicalDeviceFeatures features = {0};

    VkDeviceCreateInfo create_info = {0};
    create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    create_info.queueCreateInfoCount = unique_count;
    create_info.pQueueCreateInfos = queue_infos;
    create_info.enabledExtensionCount = 1;
    create_info.ppEnabledExtensionNames = extensions;
    create_info.pEnabledFeatures = &features;

    VkResult res = vkCreateDevice(_vk.physical_device, &create_info, NULL, &_vk.device);
    if (res != VK_SUCCESS) {
        fprintf(stderr, "[Renderer/Vulkan] vkCreateDevice falhou (%d)\n", (int)res);
        return 0;
    }

    vkGetDeviceQueue(_vk.device, _vk.queues.graphics_family, 0, &_vk.graphics_queue);
    vkGetDeviceQueue(_vk.device, _vk.queues.present_family, 0, &_vk.present_queue);
    return 1;
}

static int _vk_create_command_pool(void) {
    VkCommandPoolCreateInfo pool_info = {0};
    pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pool_info.queueFamilyIndex = _vk.queues.graphics_family;

    VkResult res = vkCreateCommandPool(_vk.device, &pool_info, NULL, &_vk.command_pool);
    if (res != VK_SUCCESS) {
        fprintf(stderr, "[Renderer/Vulkan] vkCreateCommandPool falhou (%d)\n", (int)res);
        return 0;
    }
    return 1;
}

static int _vk_carregar_shader(const char* caminho, VkShaderModule* out_module) {
    TupiAsset asset = tupi_asset_carregar(caminho);
    if (!asset.ptr || asset.tamanho == 0) {
        fprintf(stderr, "[Renderer/Vulkan] Shader SPIR-V indisponivel: '%s'\n", caminho);
        return 0;
    }
    if (asset.tamanho % 4 != 0) {
        fprintf(stderr, "[Renderer/Vulkan] Shader invalido (tamanho nao multiplo de 4): '%s'\n", caminho);
        tupi_asset_liberar(asset);
        return 0;
    }

    VkShaderModuleCreateInfo create_info = {0};
    create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    create_info.codeSize = asset.tamanho;
    create_info.pCode = (const uint32_t*)asset.ptr;

    VkResult res = vkCreateShaderModule(_vk.device, &create_info, NULL, out_module);
    tupi_asset_liberar(asset);
    if (res != VK_SUCCESS) {
        fprintf(stderr, "[Renderer/Vulkan] vkCreateShaderModule falhou para '%s' (%d)\n", caminho, (int)res);
        return 0;
    }
    return 1;
}

static int _vk_create_render_pass(void) {
    VkAttachmentDescription color = {0};
    color.format = _vk.swapchain_format;
    color.samples = VK_SAMPLE_COUNT_1_BIT;
    color.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference color_ref = {0};
    color_ref.attachment = 0;
    color_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {0};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_ref;

    VkSubpassDependency dependency = {0};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo render_pass_info = {0};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_info.attachmentCount = 1;
    render_pass_info.pAttachments = &color;
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &subpass;
    render_pass_info.dependencyCount = 1;
    render_pass_info.pDependencies = &dependency;

    return vkCreateRenderPass(_vk.device, &render_pass_info, NULL, &_vk.render_pass) == VK_SUCCESS;
}

static int _vk_create_descriptor_layout(void) {
    VkDescriptorSetLayoutBinding binding = {0};
    binding.binding = 0;
    binding.descriptorCount = 1;
    binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo layout_info = {0};
    layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout_info.bindingCount = 1;
    layout_info.pBindings = &binding;

    return vkCreateDescriptorSetLayout(_vk.device, &layout_info, NULL, &_vk.descriptor_set_layout) == VK_SUCCESS;
}

static int _vk_create_descriptor_pool(void) {
    VkDescriptorPoolSize pool_size = {0};
    pool_size.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    pool_size.descriptorCount = TUPI_MAX_TEXTURES;

    VkDescriptorPoolCreateInfo pool_info = {0};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets = TUPI_MAX_TEXTURES;
    pool_info.poolSizeCount = 1;
    pool_info.pPoolSizes = &pool_size;

    return vkCreateDescriptorPool(_vk.device, &pool_info, NULL, &_vk.descriptor_pool) == VK_SUCCESS;
}

static int _vk_create_sampler(void) {
    VkSamplerCreateInfo sampler_info = {0};
    sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_info.magFilter = VK_FILTER_NEAREST;
    sampler_info.minFilter = VK_FILTER_NEAREST;
    sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_info.anisotropyEnable = VK_FALSE;
    sampler_info.maxAnisotropy = 1.0f;
    sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    sampler_info.unnormalizedCoordinates = VK_FALSE;
    sampler_info.compareEnable = VK_FALSE;
    sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;
    sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    sampler_info.minLod = 0.0f;
    sampler_info.maxLod = 0.0f;

    return vkCreateSampler(_vk.device, &sampler_info, NULL, &_vk.sampler) == VK_SUCCESS;
}

static int _vk_create_pipeline(void) {
    VkShaderModule vert = VK_NULL_HANDLE;
    VkShaderModule frag = VK_NULL_HANDLE;

    if (!_vk_carregar_shader(TUPI_SHADER_VERT_PATH, &vert) ||
        !_vk_carregar_shader(TUPI_SHADER_FRAG_PATH, &frag)) {
        if (vert) vkDestroyShaderModule(_vk.device, vert, NULL);
        if (frag) vkDestroyShaderModule(_vk.device, frag, NULL);
        return 0;
    }

    VkPipelineShaderStageCreateInfo stages[2] = {0};
    stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = vert;
    stages[0].pName = "main";

    stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = frag;
    stages[1].pName = "main";

    VkVertexInputBindingDescription binding = {0};
    binding.binding = 0;
    binding.stride = sizeof(TupiGPUVertice);
    binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription attrs[3] = {0};
    attrs[0].location = 0;
    attrs[0].binding = 0;
    attrs[0].format = VK_FORMAT_R32G32_SFLOAT;
    attrs[0].offset = offsetof(TupiGPUVertice, x);

    attrs[1].location = 1;
    attrs[1].binding = 0;
    attrs[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attrs[1].offset = offsetof(TupiGPUVertice, r);

    attrs[2].location = 2;
    attrs[2].binding = 0;
    attrs[2].format = VK_FORMAT_R32G32_SFLOAT;
    attrs[2].offset = offsetof(TupiGPUVertice, u);

    VkPipelineVertexInputStateCreateInfo vertex_input = {0};
    vertex_input.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input.vertexBindingDescriptionCount = 1;
    vertex_input.pVertexBindingDescriptions = &binding;
    vertex_input.vertexAttributeDescriptionCount = 3;
    vertex_input.pVertexAttributeDescriptions = attrs;

    VkPipelineInputAssemblyStateCreateInfo assembly = {0};
    assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    assembly.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewport_state = {0};
    viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state.viewportCount = 1;
    viewport_state.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo raster = {0};
    raster.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    raster.depthClampEnable = VK_FALSE;
    raster.rasterizerDiscardEnable = VK_FALSE;
    raster.polygonMode = VK_POLYGON_MODE_FILL;
    raster.lineWidth = 1.0f;
    raster.cullMode = VK_CULL_MODE_NONE;
    raster.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    raster.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisample = {0};
    multisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisample.minSampleShading = 1.0f;

    VkPipelineColorBlendAttachmentState blend_attachment = {0};
    blend_attachment.blendEnable = VK_TRUE;
    blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
    blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;
    blend_attachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT |
        VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT |
        VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo blend = {0};
    blend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    blend.logicOpEnable = VK_FALSE;
    blend.attachmentCount = 1;
    blend.pAttachments = &blend_attachment;

    VkDynamicState dynamic_states[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
    VkPipelineDynamicStateCreateInfo dynamic = {0};
    dynamic.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic.dynamicStateCount = 2;
    dynamic.pDynamicStates = dynamic_states;

    VkPushConstantRange push = {0};
    push.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    push.offset = 0;
    push.size = sizeof(TupiPushConstantes);

    VkPipelineLayoutCreateInfo layout_info = {0};
    layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layout_info.setLayoutCount = 1;
    layout_info.pSetLayouts = &_vk.descriptor_set_layout;
    layout_info.pushConstantRangeCount = 1;
    layout_info.pPushConstantRanges = &push;

    if (vkCreatePipelineLayout(_vk.device, &layout_info, NULL, &_vk.pipeline_layout) != VK_SUCCESS) {
        vkDestroyShaderModule(_vk.device, vert, NULL);
        vkDestroyShaderModule(_vk.device, frag, NULL);
        return 0;
    }

    VkPipelineDepthStencilStateCreateInfo depth = {0};
    depth.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth.depthTestEnable = VK_FALSE;
    depth.depthWriteEnable = VK_FALSE;
    depth.depthCompareOp = VK_COMPARE_OP_ALWAYS;

    VkGraphicsPipelineCreateInfo pipeline_info = {0};
    pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_info.stageCount = 2;
    pipeline_info.pStages = stages;
    pipeline_info.pVertexInputState = &vertex_input;
    pipeline_info.pInputAssemblyState = &assembly;
    pipeline_info.pViewportState = &viewport_state;
    pipeline_info.pRasterizationState = &raster;
    pipeline_info.pMultisampleState = &multisample;
    pipeline_info.pDepthStencilState = &depth;
    pipeline_info.pColorBlendState = &blend;
    pipeline_info.pDynamicState = &dynamic;
    pipeline_info.layout = _vk.pipeline_layout;
    pipeline_info.renderPass = _vk.render_pass;
    pipeline_info.subpass = 0;

    VkResult res = vkCreateGraphicsPipelines(_vk.device, VK_NULL_HANDLE, 1, &pipeline_info, NULL, &_vk.pipeline);
    vkDestroyShaderModule(_vk.device, vert, NULL);
    vkDestroyShaderModule(_vk.device, frag, NULL);
    return res == VK_SUCCESS;
}

static int _vk_create_swapchain(void) {
    TupiSwapchainSupport support = _vk_query_swapchain_support(_vk.physical_device);
    if (support.format_count == 0 || support.present_mode_count == 0) {
        _vk_free_swapchain_support(&support);
        fprintf(stderr, "[Renderer/Vulkan] Swapchain sem formatos ou present modes compatíveis.\n");
        return 0;
    }

    VkSurfaceFormatKHR surface_format = _vk_choose_surface_format(support.formats, support.format_count);
    VkPresentModeKHR present_mode = _vk_choose_present_mode(support.present_modes, support.present_mode_count);
    VkExtent2D extent = _vk_choose_extent(&support.capabilities);

    uint32_t image_count = support.capabilities.minImageCount + 1;
    if (support.capabilities.maxImageCount > 0 && image_count > support.capabilities.maxImageCount) {
        image_count = support.capabilities.maxImageCount;
    }

    uint32_t queue_family_indices[] = {_vk.queues.graphics_family, _vk.queues.present_family};

    VkSwapchainCreateInfoKHR create_info = {0};
    create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    create_info.surface = _vk.surface;
    create_info.minImageCount = image_count;
    create_info.imageFormat = surface_format.format;
    create_info.imageColorSpace = surface_format.colorSpace;
    create_info.imageExtent = extent;
    create_info.imageArrayLayers = 1;
    create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    if (_vk.queues.graphics_family != _vk.queues.present_family) {
        create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        create_info.queueFamilyIndexCount = 2;
        create_info.pQueueFamilyIndices = queue_family_indices;
    } else {
        create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    create_info.preTransform = support.capabilities.currentTransform;
    create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    create_info.presentMode = present_mode;
    create_info.clipped = VK_TRUE;
    create_info.oldSwapchain = VK_NULL_HANDLE;

    VkResult res = vkCreateSwapchainKHR(_vk.device, &create_info, NULL, &_vk.swapchain);
    if (res != VK_SUCCESS) {
        _vk_free_swapchain_support(&support);
        fprintf(stderr, "[Renderer/Vulkan] vkCreateSwapchainKHR falhou (%d)\n", (int)res);
        return 0;
    }

    vkGetSwapchainImagesKHR(_vk.device, _vk.swapchain, &image_count, NULL);
    _vk.swapchain_images = (VkImage*)malloc(image_count * sizeof(VkImage));
    _vk.swapchain_image_views = (VkImageView*)calloc(image_count, sizeof(VkImageView));
    _vk.swapchain_framebuffers = (VkFramebuffer*)calloc(image_count, sizeof(VkFramebuffer));
    if (!_vk.swapchain_images || !_vk.swapchain_image_views || !_vk.swapchain_framebuffers) {
        _vk_free_swapchain_support(&support);
        return 0;
    }

    vkGetSwapchainImagesKHR(_vk.device, _vk.swapchain, &image_count, _vk.swapchain_images);
    _vk.swapchain_image_count = image_count;
    _vk.swapchain_format = surface_format.format;
    _vk.swapchain_extent = extent;
    _vk.images_in_flight = (VkFence*)realloc(_vk.images_in_flight, image_count * sizeof(VkFence));
    if (!_vk.images_in_flight) {
        _vk_free_swapchain_support(&support);
        return 0;
    }
    for (uint32_t i = 0; i < image_count; i++) _vk.images_in_flight[i] = VK_NULL_HANDLE;

    for (uint32_t i = 0; i < image_count; i++) {
        _vk.swapchain_image_views[i] = _vk_create_image_view(_vk.swapchain_images[i], _vk.swapchain_format);
        if (_vk.swapchain_image_views[i] == VK_NULL_HANDLE) {
            _vk_free_swapchain_support(&support);
            return 0;
        }
    }

    _vk_free_swapchain_support(&support);
    return 1;
}

static int _vk_create_framebuffers(void) {
    for (uint32_t i = 0; i < _vk.swapchain_image_count; i++) {
        VkImageView attachments[] = {_vk.swapchain_image_views[i]};

        VkFramebufferCreateInfo framebuffer_info = {0};
        framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebuffer_info.renderPass = _vk.render_pass;
        framebuffer_info.attachmentCount = 1;
        framebuffer_info.pAttachments = attachments;
        framebuffer_info.width = _vk.swapchain_extent.width;
        framebuffer_info.height = _vk.swapchain_extent.height;
        framebuffer_info.layers = 1;

        if (vkCreateFramebuffer(_vk.device, &framebuffer_info, NULL, &_vk.swapchain_framebuffers[i]) != VK_SUCCESS) {
            return 0;
        }
    }
    return 1;
}

static int _vk_create_frame_sync(void) {
    VkSemaphoreCreateInfo sem_info = {0};
    sem_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fence_info = {0};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    VkCommandBufferAllocateInfo alloc_info = {0};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.commandPool = _vk.command_pool;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandBufferCount = TUPI_MAX_FRAMES_IN_FLIGHT;

    VkCommandBuffer buffers[TUPI_MAX_FRAMES_IN_FLIGHT] = {0};
    if (vkAllocateCommandBuffers(_vk.device, &alloc_info, buffers) != VK_SUCCESS) {
        return 0;
    }

    for (uint32_t i = 0; i < TUPI_MAX_FRAMES_IN_FLIGHT; i++) {
        _vk.frames[i].command_buffer = buffers[i];
        if (vkCreateSemaphore(_vk.device, &sem_info, NULL, &_vk.frames[i].image_available) != VK_SUCCESS) return 0;
        if (vkCreateSemaphore(_vk.device, &sem_info, NULL, &_vk.frames[i].render_finished) != VK_SUCCESS) return 0;
        if (vkCreateFence(_vk.device, &fence_info, NULL, &_vk.frames[i].in_flight) != VK_SUCCESS) return 0;
    }
    return 1;
}

static void _vk_cleanup_swapchain(void) {
    if (_vk.device == VK_NULL_HANDLE) return;

    for (uint32_t i = 0; i < _vk.swapchain_image_count; i++) {
        if (_vk.swapchain_framebuffers && _vk.swapchain_framebuffers[i] != VK_NULL_HANDLE) {
            vkDestroyFramebuffer(_vk.device, _vk.swapchain_framebuffers[i], NULL);
        }
        if (_vk.swapchain_image_views && _vk.swapchain_image_views[i] != VK_NULL_HANDLE) {
            vkDestroyImageView(_vk.device, _vk.swapchain_image_views[i], NULL);
        }
    }

    free(_vk.swapchain_framebuffers);
    free(_vk.swapchain_image_views);
    free(_vk.swapchain_images);
    _vk.swapchain_framebuffers = NULL;
    _vk.swapchain_image_views = NULL;
    _vk.swapchain_images = NULL;

    if (_vk.swapchain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(_vk.device, _vk.swapchain, NULL);
        _vk.swapchain = VK_NULL_HANDLE;
    }

    _vk.swapchain_image_count = 0;
}

static int _vk_recreate_swapchain(void) {
    int dw = 0, dh = 0;
    _vk_get_drawable_size(&dw, &dh);
    if (dw <= 0 || dh <= 0) return 0;

    vkDeviceWaitIdle(_vk.device);
    _vk_cleanup_swapchain();

    if (!_vk_create_swapchain()) return 0;

    if (_vk.render_pass != VK_NULL_HANDLE) {
        vkDestroyRenderPass(_vk.device, _vk.render_pass, NULL);
        _vk.render_pass = VK_NULL_HANDLE;
    }
    if (_vk.pipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(_vk.device, _vk.pipeline, NULL);
        _vk.pipeline = VK_NULL_HANDLE;
    }
    if (_vk.pipeline_layout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(_vk.device, _vk.pipeline_layout, NULL);
        _vk.pipeline_layout = VK_NULL_HANDLE;
    }

    if (!_vk_create_render_pass()) return 0;
    if (!_vk_create_pipeline()) return 0;
    if (!_vk_create_framebuffers()) return 0;

    _largura = (int)_vk.swapchain_extent.width;
    _altura = (int)_vk.swapchain_extent.height;
    _configurar_projecao(_largura, _altura);
    _framebuffer_resized = 0;
    return 1;
}

static int _vk_record_command_buffer(VkCommandBuffer cmd, uint32_t image_index) {
    VkCommandBufferBeginInfo begin_info = {0};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    if (vkBeginCommandBuffer(cmd, &begin_info) != VK_SUCCESS) {
        return 0;
    }

    VkClearValue clear = {0};
    clear.color.float32[0] = _letterbox_ativo ? 0.0f : _fundo_r;
    clear.color.float32[1] = _letterbox_ativo ? 0.0f : _fundo_g;
    clear.color.float32[2] = _letterbox_ativo ? 0.0f : _fundo_b;
    clear.color.float32[3] = 1.0f;

    VkRenderPassBeginInfo render_pass_info = {0};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_info.renderPass = _vk.render_pass;
    render_pass_info.framebuffer = _vk.swapchain_framebuffers[image_index];
    render_pass_info.renderArea.offset = (VkOffset2D){0, 0};
    render_pass_info.renderArea.extent = _vk.swapchain_extent;
    render_pass_info.clearValueCount = 1;
    render_pass_info.pClearValues = &clear;

    vkCmdBeginRenderPass(cmd, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _vk.pipeline);

    VkViewport viewport = {0};
    viewport.x = (float)(_letterbox_ativo ? _lb_x : 0);
    viewport.y = (float)(_letterbox_ativo ? _lb_y : 0);
    viewport.width = (float)(_letterbox_ativo ? _lb_w : (int)_vk.swapchain_extent.width);
    viewport.height = (float)(_letterbox_ativo ? _lb_h : (int)_vk.swapchain_extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D scissor = {0};
    scissor.offset.x = _letterbox_ativo ? _lb_x : 0;
    scissor.offset.y = _letterbox_ativo ? _lb_y : 0;
    scissor.extent.width = (uint32_t)(_letterbox_ativo ? _lb_w : (int)_vk.swapchain_extent.width);
    scissor.extent.height = (uint32_t)(_letterbox_ativo ? _lb_h : (int)_vk.swapchain_extent.height);
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    VkBuffer vertex_buffers[] = {_vk.vertex_buffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(cmd, 0, 1, vertex_buffers, offsets);

    TupiMatriz proj = tupi_projecao_ortografica(_logico_w, _logico_h);
    TupiPushConstantes push = {0};
    memcpy(push.proj, proj.m, sizeof(push.proj));
    push.proj[5]  = -push.proj[5];
    push.proj[13] = -push.proj[13];
    vkCmdPushConstants(cmd, _vk.pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(push), &push);

    if (_letterbox_ativo) {
        TupiGPUVertice fundo[6];
        float cor[4] = {_fundo_r, _fundo_g, _fundo_b, 1.0f};
        float quad[16] = {
            0.0f,            0.0f,            0.0f, 0.0f,
            (float)_logico_w, 0.0f,            1.0f, 0.0f,
            0.0f,            (float)_logico_h, 0.0f, 1.0f,
            (float)_logico_w, (float)_logico_h, 1.0f, 1.0f,
        };

        fundo[0] = (TupiGPUVertice){quad[0],  quad[1],  cor[0], cor[1], cor[2], cor[3], quad[2],  quad[3]};
        fundo[1] = (TupiGPUVertice){quad[4],  quad[5],  cor[0], cor[1], cor[2], cor[3], quad[6],  quad[7]};
        fundo[2] = (TupiGPUVertice){quad[8],  quad[9],  cor[0], cor[1], cor[2], cor[3], quad[10], quad[11]};
        fundo[3] = (TupiGPUVertice){quad[4],  quad[5],  cor[0], cor[1], cor[2], cor[3], quad[6],  quad[7]};
        fundo[4] = (TupiGPUVertice){quad[12], quad[13], cor[0], cor[1], cor[2], cor[3], quad[14], quad[15]};
        fundo[5] = (TupiGPUVertice){quad[8],  quad[9],  cor[0], cor[1], cor[2], cor[3], quad[10], quad[11]};

        if (_vk.frame_vertex_count + 6 <= _vk.vertex_capacity && _vk.vertex_buffer_mapeado) {
            uint32_t offset = _vk.frame_vertex_count;
            memcpy((char*)_vk.vertex_buffer_mapeado + (size_t)offset * sizeof(TupiGPUVertice), fundo, sizeof(fundo));

            TupiTextureVulkan* branco = (_vk.texturas_cap > _vk.textura_branca_id) ? &_vk.texturas[_vk.textura_branca_id] : NULL;
            if (branco && branco->ativo) {
                vkCmdBindDescriptorSets(
                    cmd,
                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                    _vk.pipeline_layout,
                    0, 1, &branco->descriptor_set,
                    0, NULL
                );
                vkCmdDraw(cmd, 6, 1, offset, 0);
            }
        }
    }

    for (uint32_t i = 0; i < _vk.frame_packet_count; i++) {
        TupiDrawPacketGPU* packet = &_vk.frame_packets[i];
        if (packet->textura_id >= _vk.texturas_cap) continue;
        TupiTextureVulkan* tex = &_vk.texturas[packet->textura_id];
        if (!tex->ativo) continue;

        vkCmdBindDescriptorSets(
            cmd,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            _vk.pipeline_layout,
            0, 1, &tex->descriptor_set,
            0, NULL
        );
        vkCmdDraw(cmd, packet->total_vertices, 1, packet->primeiro_vertice, 0);
    }

    vkCmdEndRenderPass(cmd);
    return vkEndCommandBuffer(cmd) == VK_SUCCESS;
}

static int _vk_desenhar_frame(void) {
    TupiFrameVulkan* frame = &_vk.frames[_vk.frame_atual];
    vkWaitForFences(_vk.device, 1, &frame->in_flight, VK_TRUE, UINT64_MAX);

    uint32_t image_index = 0;
    VkResult acquire = vkAcquireNextImageKHR(
        _vk.device,
        _vk.swapchain,
        UINT64_MAX,
        frame->image_available,
        VK_NULL_HANDLE,
        &image_index
    );

    if (acquire == VK_ERROR_OUT_OF_DATE_KHR) {
        return _vk_recreate_swapchain();
    }
    if (acquire != VK_SUCCESS && acquire != VK_SUBOPTIMAL_KHR) {
        fprintf(stderr, "[Renderer/Vulkan] vkAcquireNextImageKHR falhou (%d)\n", (int)acquire);
        return 0;
    }

    if (_vk.images_in_flight[image_index] != VK_NULL_HANDLE) {
        vkWaitForFences(_vk.device, 1, &_vk.images_in_flight[image_index], VK_TRUE, UINT64_MAX);
    }
    _vk.images_in_flight[image_index] = frame->in_flight;

    vkResetFences(_vk.device, 1, &frame->in_flight);
    vkResetCommandBuffer(frame->command_buffer, 0);

    if (!_vk_record_command_buffer(frame->command_buffer, image_index)) {
        fprintf(stderr, "[Renderer/Vulkan] Falha ao gravar command buffer.\n");
        return 0;
    }

    VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    VkSubmitInfo submit = {0};
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.waitSemaphoreCount = 1;
    submit.pWaitSemaphores = &frame->image_available;
    submit.pWaitDstStageMask = wait_stages;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &frame->command_buffer;
    submit.signalSemaphoreCount = 1;
    submit.pSignalSemaphores = &frame->render_finished;

    if (vkQueueSubmit(_vk.graphics_queue, 1, &submit, frame->in_flight) != VK_SUCCESS) {
        fprintf(stderr, "[Renderer/Vulkan] vkQueueSubmit falhou.\n");
        return 0;
    }

    VkPresentInfoKHR present = {0};
    present.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present.waitSemaphoreCount = 1;
    present.pWaitSemaphores = &frame->render_finished;
    present.swapchainCount = 1;
    present.pSwapchains = &_vk.swapchain;
    present.pImageIndices = &image_index;

    VkResult pres = vkQueuePresentKHR(_vk.present_queue, &present);
    if (pres == VK_ERROR_OUT_OF_DATE_KHR || pres == VK_SUBOPTIMAL_KHR || _framebuffer_resized) {
        _vk_recreate_swapchain();
    } else if (pres != VK_SUCCESS) {
        fprintf(stderr, "[Renderer/Vulkan] vkQueuePresentKHR falhou (%d)\n", (int)pres);
        return 0;
    }

    _vk.frame_atual = (_vk.frame_atual + 1) % TUPI_MAX_FRAMES_IN_FLIGHT;
    return 1;
}

static int _vk_init(void) {
    if (SDL_Vulkan_LoadLibrary(NULL) != 0) {
        fprintf(stderr, "[Renderer/Vulkan] Vulkan indisponivel neste sistema: %s\n", SDL_GetError());
        return 0;
    }

    if (!_vk_create_instance()) return 0;

    if (!SDL_Vulkan_CreateSurface(_janela, _vk.instance, &_vk.surface)) {
        fprintf(stderr, "[Renderer/Vulkan] SDL_Vulkan_CreateSurface falhou: %s\n", SDL_GetError());
        return 0;
    }

    if (!_vk_pick_physical_device()) return 0;
    if (!_vk_create_device()) return 0;
    if (!_vk_create_command_pool()) return 0;
    if (!_vk_create_descriptor_layout()) return 0;
    if (!_vk_create_descriptor_pool()) return 0;
    if (!_vk_create_sampler()) return 0;
    if (!_vk_create_swapchain()) return 0;
    if (!_vk_create_render_pass()) return 0;
    if (!_vk_create_pipeline()) return 0;
    if (!_vk_create_framebuffers()) return 0;
    if (!_vk_create_frame_sync()) return 0;
    if (!_vk_garantir_vertex_buffer(TUPI_INITIAL_VERTEX_CAPACITY)) return 0;
    if (!_vk_garantir_texturas(1)) return 0;

    const unsigned char branco[4] = {255, 255, 255, 255};
    if (!_vk_criar_textura_rgba8_interna(branco, 1, 1, &_vk.textura_branca_id)) {
        fprintf(stderr, "[Renderer/Vulkan] Falha ao criar textura branca padrao.\n");
        return 0;
    }

    _vk.pronto = 1;
    _largura = (int)_vk.swapchain_extent.width;
    _altura  = (int)_vk.swapchain_extent.height;
    return 1;
}

static void _vk_shutdown(void) {
    if (_vk.device != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(_vk.device);
    }

    for (uint32_t i = 0; i < _vk.texturas_cap; i++) {
        _vk_destroy_texture_slot(i);
    }
    free(_vk.texturas);
    _vk.texturas = NULL;
    _vk.texturas_cap = 0;

    if (_vk.vertex_buffer_mapeado && _vk.vertex_buffer_memory != VK_NULL_HANDLE) {
        vkUnmapMemory(_vk.device, _vk.vertex_buffer_memory);
        _vk.vertex_buffer_mapeado = NULL;
    }
    _vk_destroy_buffer(&_vk.vertex_buffer, &_vk.vertex_buffer_memory);

    for (uint32_t i = 0; i < TUPI_MAX_FRAMES_IN_FLIGHT; i++) {
        if (_vk.frames[i].image_available != VK_NULL_HANDLE) {
            vkDestroySemaphore(_vk.device, _vk.frames[i].image_available, NULL);
        }
        if (_vk.frames[i].render_finished != VK_NULL_HANDLE) {
            vkDestroySemaphore(_vk.device, _vk.frames[i].render_finished, NULL);
        }
        if (_vk.frames[i].in_flight != VK_NULL_HANDLE) {
            vkDestroyFence(_vk.device, _vk.frames[i].in_flight, NULL);
        }
    }

    _vk_cleanup_swapchain();
    free(_vk.images_in_flight);
    _vk.images_in_flight = NULL;

    if (_vk.pipeline != VK_NULL_HANDLE) vkDestroyPipeline(_vk.device, _vk.pipeline, NULL);
    if (_vk.pipeline_layout != VK_NULL_HANDLE) vkDestroyPipelineLayout(_vk.device, _vk.pipeline_layout, NULL);
    if (_vk.render_pass != VK_NULL_HANDLE) vkDestroyRenderPass(_vk.device, _vk.render_pass, NULL);
    if (_vk.sampler != VK_NULL_HANDLE) vkDestroySampler(_vk.device, _vk.sampler, NULL);
    if (_vk.descriptor_pool != VK_NULL_HANDLE) vkDestroyDescriptorPool(_vk.device, _vk.descriptor_pool, NULL);
    if (_vk.descriptor_set_layout != VK_NULL_HANDLE) vkDestroyDescriptorSetLayout(_vk.device, _vk.descriptor_set_layout, NULL);
    if (_vk.command_pool != VK_NULL_HANDLE) vkDestroyCommandPool(_vk.device, _vk.command_pool, NULL);
    if (_vk.device != VK_NULL_HANDLE) vkDestroyDevice(_vk.device, NULL);
    if (_vk.surface != VK_NULL_HANDLE) vkDestroySurfaceKHR(_vk.instance, _vk.surface, NULL);
    if (_vk.instance != VK_NULL_HANDLE) vkDestroyInstance(_vk.instance, NULL);

    free(_vk.frame_vertices);
    free(_vk.frame_packets);
    _vk.frame_vertices = NULL;
    _vk.frame_packets = NULL;

    memset(&_vk, 0, sizeof(_vk));
    SDL_Vulkan_UnloadLibrary();
}

// --- API interna de sprites ---

unsigned int tupi_renderer_textura_criar_rgba8(const unsigned char* pixels, int largura, int altura) {
    if (!_vk.pronto || largura <= 0 || altura <= 0) return 0;
    uint32_t id = 0;
    if (!_vk_criar_textura_rgba8_interna(pixels, (uint32_t)largura, (uint32_t)altura, &id)) {
        fprintf(stderr, "[Renderer/Vulkan] Falha ao subir textura %dx%d para a GPU.\n", largura, altura);
        return 0;
    }
    return id;
}

void tupi_renderer_textura_destruir(unsigned int textura_id) {
    if (!_vk.pronto || textura_id == 0 || textura_id == _vk.textura_branca_id) return;
    if (textura_id >= _vk.texturas_cap) return;
    _vk_destroy_texture_slot(textura_id);
}

void tupi_renderer_desenhar_quad(unsigned int textura_id, const float* quad, const float* cor) {
    if (!_vk.pronto || textura_id == 0) return;
    _vk_emitir_quad_texturizado(textura_id, quad, cor);
}

// Legado: mantido apenas para compatibilidade de link.
SDL_Renderer* tupi_renderer_get(void) {
    return NULL;
}

// --- Ícone da janela ---

static void _carregar_icone(const char* caminho) {
    const char* alvo = (caminho && caminho[0]) ? caminho : ".engine/icon.png";

    TupiImagemRust* img = tupi_imagem_carregar_seguro(alvo);
    if (!img) {
        fprintf(stderr, "[Renderer] Icone nao encontrado: '%s'\n", alvo);
        return;
    }

    SDL_Surface* surf = SDL_CreateRGBSurfaceFrom(
        img->pixels, img->largura, img->altura,
        32, img->largura * 4,
        0x000000FFu, 0x0000FF00u, 0x00FF0000u, 0xFF000000u
    );

    if (surf) {
        SDL_SetWindowIcon(_janela, surf);
        SDL_FreeSurface(surf);
    } else {
        fprintf(stderr, "[Renderer] Falha ao definir icone: %s\n", SDL_GetError());
    }

    tupi_imagem_destruir(img);
}

// --- Criação da janela ---

int tupi_janela_criar(int largura, int altura, const char* titulo,
                      float escala, int sem_borda, const char* icone) {

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        fprintf(stderr, "[Renderer] Falha ao inicializar SDL2: %s\n", SDL_GetError());
        return 0;
    }

    _escala         = (escala > 0.0f) ? escala : 1.0f;
    _flag_sem_borda = sem_borda;

    int fis_w = (int)(largura * _escala);
    int fis_h = (int)(altura  * _escala);
    _logico_w = largura;
    _logico_h = altura;
    _largura  = fis_w;
    _altura   = fis_h;

    Uint32 flags = SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_VULKAN;
    if (sem_borda) flags |= SDL_WINDOW_BORDERLESS;

    _janela = SDL_CreateWindow(
        titulo ? titulo : "TupiEngine",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        fis_w, fis_h, flags
    );
    if (!_janela) {
        fprintf(stderr, "[Renderer] Falha ao criar janela Vulkan: %s\n", SDL_GetError());
        SDL_Quit();
        return 0;
    }

    if (!_vk_init()) {
        fprintf(stderr, "[Renderer] Backend Vulkan nao pode ser inicializado. Verifique suporte a driver/loader e os shaders SPIR-V.\n");
        _vk_shutdown();
        if (_janela) {
            SDL_DestroyWindow(_janela);
            _janela = NULL;
        }
        SDL_Quit();
        return 0;
    }

    _carregar_icone(icone);
    tupi_sprite_iniciar();
    tupi_batcher_registrar_flush(_flush_batcher);

    _vk_get_drawable_size(&_largura, &_altura);
    _configurar_projecao(_largura, _altura);

    _tempo_inicio   = (double)SDL_GetPerformanceCounter() /
                      (double)SDL_GetPerformanceFrequency();
    _tempo_anterior = _tempo_inicio;
    _tick_frame     = SDL_GetPerformanceCounter();

    tupi_input_iniciar(_janela);
    tupi_input_set_dimensoes(&_largura, &_altura, &_logico_w, &_logico_h);

    _janela_aberta = 1;

    printf("[TupiEngine] SDL2 %d.%d.%d + Vulkan\n",
           SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_PATCHLEVEL);
    printf("[TupiEngine] Janela: %dx%d px | mundo: %dx%d | escala: %.2fx\n",
           _largura, _altura, _logico_w, _logico_h, _escala);

    return 1;
}

// --- Loop principal ---

int tupi_janela_aberta(void) {
    return _janela_aberta;
}

void tupi_janela_limpar(void) {
    _fps_frame_inicio();
    _vk_reset_frame_cpu();

    Uint64 freq  = SDL_GetPerformanceFrequency();
    double agora = (double)SDL_GetPerformanceCounter() / (double)freq;
    _delta          = agora - _tempo_anterior;
    _tempo_anterior = agora;

    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
        switch (ev.type) {
            case SDL_QUIT:
                _janela_aberta = 0;
                break;
            case SDL_WINDOWEVENT:
                if (ev.window.event == SDL_WINDOWEVENT_RESIZED ||
                    ev.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                    _vk_get_drawable_size(&_largura, &_altura);
                    if (!_letterbox_ativo) {
                        _escala = (_logico_w > 0) ? (float)_largura / (float)_logico_w : 1.0f;
                    }
                    _configurar_projecao(_largura, _altura);
                    _framebuffer_resized = 1;
                }
                break;
            default:
                break;
        }
        tupi_input_processar_evento(&ev);
    }

    TupiCamera* cam = tupi_camera_ativa();
    if (cam) {
        tupi_camera_frame(cam, _logico_w, _logico_h);
    } else {
        TupiMatriz proj = tupi_projecao_ortografica(_logico_w, _logico_h);
        tupi_sprite_set_viewport(_logico_w, _logico_h);
        tupi_sprite_set_projecao(proj.m);
    }

    tupi_sprite_reset_cor();
    _cor_r = _cor_g = _cor_b = _cor_a = 1.0f;
}

void tupi_janela_atualizar(void) {
    tupi_batch_desenhar();
    tupi_batcher_flush();

    if (_vk.pronto) {
        if (!_vk_garantir_vertex_buffer(_vk.frame_vertex_count + (_letterbox_ativo ? 6u : 0u))) {
            fprintf(stderr, "[Renderer/Vulkan] Falha ao garantir capacidade do VBO persistente.\n");
        } else if (_vk.frame_vertex_count > 0 && _vk.vertex_buffer_mapeado) {
            memcpy(_vk.vertex_buffer_mapeado, _vk.frame_vertices, (size_t)_vk.frame_vertex_count * sizeof(TupiGPUVertice));
        }

        if (_vk.swapchain == VK_NULL_HANDLE || _framebuffer_resized) {
            _vk_recreate_swapchain();
        }
        if (_vk.swapchain != VK_NULL_HANDLE) {
            _vk_desenhar_frame();
        }
    }

    tupi_input_salvar_estado();
    tupi_input_atualizar_mouse();
    _fps_frame_fim();
}

void tupi_janela_fechar(void) {
    tupi_sprite_encerrar();
    _vk_shutdown();
    if (_janela)   { SDL_DestroyWindow(_janela); _janela = NULL; }
    SDL_Quit();
    _janela_aberta = 0;
    printf("[TupiEngine] Encerrado.\n");
}

// --- Controles em runtime ---

void tupi_janela_set_titulo(const char* titulo) {
    if (_janela && titulo) SDL_SetWindowTitle(_janela, titulo);
}

void tupi_janela_set_decoracao(int ativo) {
    if (!_janela) return;
    SDL_SetWindowBordered(_janela, ativo ? SDL_TRUE : SDL_FALSE);
    _flag_sem_borda = !ativo;
}

void tupi_janela_tela_cheia(int ativo) {
    if (!_janela) return;
    if (ativo) {
        SDL_SetWindowFullscreen(_janela, SDL_WINDOW_FULLSCREEN_DESKTOP);
    } else {
        _letterbox_ativo = 0;
        SDL_SetWindowFullscreen(_janela, 0);
    }
    _vk_get_drawable_size(&_largura, &_altura);
    _configurar_projecao(_largura, _altura);
    _framebuffer_resized = 1;
}

void tupi_janela_tela_cheia_letterbox(int ativo) {
    if (!_janela) return;

    if (ativo) {
        _letterbox_ativo = 1;
        SDL_SetWindowFullscreen(_janela, SDL_WINDOW_FULLSCREEN_DESKTOP);
    } else {
        _letterbox_ativo = 0;
        SDL_SetWindowFullscreen(_janela, 0);

        int fis_w = (int)(_logico_w * _escala);
        int fis_h = (int)(_logico_h * _escala);
        SDL_SetWindowSize(_janela, fis_w, fis_h);
    }

    _vk_get_drawable_size(&_largura, &_altura);
    _configurar_projecao(_largura, _altura);
    _framebuffer_resized = 1;
}

int tupi_janela_letterbox_ativo(void) { return _letterbox_ativo; }

// --- Getters ---

float  tupi_janela_escala(void)     { return _escala;   }
double tupi_tempo(void) {
    return (double)SDL_GetPerformanceCounter() /
           (double)SDL_GetPerformanceFrequency() - _tempo_inicio;
}
double tupi_delta_tempo(void)       { return _delta;    }
int    tupi_janela_largura(void)    { return _logico_w; }
int    tupi_janela_altura(void)     { return _logico_h; }
int    tupi_janela_largura_px(void) { return _largura;  }
int    tupi_janela_altura_px(void)  { return _altura;   }

// --- Cor ---

void tupi_cor_fundo(float r, float g, float b) {
    _fundo_r = r; _fundo_g = g; _fundo_b = b;
}

void tupi_cor(float r, float g, float b, float a) {
    _cor_r = r; _cor_g = g; _cor_b = b; _cor_a = a;
    tupi_sprite_set_cor(r, g, b, a);
}

// --- Formas 2D ---

void tupi_retangulo(float x, float y, float largura, float altura) {
    const float cor[4] = {_cor_r, _cor_g, _cor_b, _cor_a};
    _vk_emitir_retangulo(x, y, largura, altura, cor);
}

void tupi_retangulo_borda(float x, float y, float largura, float altura, float espessura) {
    const float cor[4] = {_cor_r, _cor_g, _cor_b, _cor_a};
    _vk_emitir_linha(x,         y,          x + largura, y,           espessura, cor);
    _vk_emitir_linha(x + largura, y,        x + largura, y + altura,  espessura, cor);
    _vk_emitir_linha(x + largura, y + altura, x,         y + altura,  espessura, cor);
    _vk_emitir_linha(x,         y + altura, x,           y,           espessura, cor);
}

void tupi_triangulo(float x1, float y1, float x2, float y2, float x3, float y3) {
    const float cor[4] = {_cor_r, _cor_g, _cor_b, _cor_a};
    _vk_emitir_triangulo_solido(x1, y1, x2, y2, x3, y3, cor);
}

void tupi_circulo(float x, float y, float raio, int segmentos) {
    const float cor[4] = {_cor_r, _cor_g, _cor_b, _cor_a};
    _vk_emitir_circulo(x, y, raio, segmentos, cor);
}

void tupi_circulo_borda(float x, float y, float raio, int segmentos, float espessura) {
    const float cor[4] = {_cor_r, _cor_g, _cor_b, _cor_a};
    _vk_emitir_circulo_borda(x, y, raio, segmentos, espessura, cor);
}

void tupi_linha(float x1, float y1, float x2, float y2, float espessura) {
    const float cor[4] = {_cor_r, _cor_g, _cor_b, _cor_a};
    _vk_emitir_linha(x1, y1, x2, y2, espessura, cor);
}
