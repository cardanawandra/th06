#pragma once

#define VK_USE_PLATFORM_SDL2
#include <vulkan/vulkan.h>
#include <SDL.h>
#include <vector>
#include <unordered_map>

#include "GfxInterface.hpp"

struct VulkanBackend : GfxInterface
{
    static GfxInterface* Init();
    static void SetContextFlags();

    void Exit() override;
    ~VulkanBackend() override { Exit(); }

    // API
    void SetFogRange(f32 nearPlane, f32 farPlane) override;
    void SetFogColor(ZunColor color) override;

    void ToggleVertexAttribute(u8 attr, bool enable) override;
    void SetAttributePointer(VertexAttributeArrays attr, size_t stride, void* ptr) override;

    void SetColorOp(TextureOpComponent component, ColorOp op) override;
    void SetTextureFactor(ZunColor factor) override;

    void SetTransformMatrix(TransformMatrix type, const ZunMatrix& matrix) override;

    void SetTextureFilter() override;

    void GetViewport(u32* viewport) override;
    void SetViewport(i32 x, i32 y, i32 w, i32 h) override;

    void Enable(Capabilities cap) override;

    void SetBlendMode(BlendMode mode) override;
    void SetDepthMask(bool enable) override;
    void SetDepthFunc(DepthFunc func) override;

    void SetClearColor(f32 r, f32 g, f32 b, f32 a) override;
    void SetClearDepth(f32 depth) override;
    void Clear(u32 bits) override;

    GfxTextureHandle CreateTexture() override;
    void BindTexture(GfxTextureHandle handle) override;
    void DeleteTexture(GfxTextureHandle handle) override;

    void SetTextureImage(u32 w, u32 h, PixelFormat fmt, PixelDataType type, const void* data) override;
    void SetTextureSubImage(i32 x, i32 y, i32 w, i32 h, const void* data) override;

    void ReadPixels(i32 x, i32 y, i32 w, i32 h, const void* pixels) override;

    void Draw(PrimitiveType type, i32 start, i32 count) override;
    void SwapBuffers() override;
    virtual bool GameLoop(){return true;}

private:
    SDL_Window* window = nullptr;

    // --- Vulkan core ---
    VkInstance instance{};
    VkPhysicalDevice physicalDevice{};
    VkDevice device{};
    VkSurfaceKHR surface{};

    VkQueue graphicsQueue{};
    VkQueue presentQueue{};

    VkSwapchainKHR swapchain{};
    std::vector<VkImage> swapImages;
    std::vector<VkImageView> swapViews;

    VkRenderPass renderPass{};
    std::vector<VkFramebuffer> framebuffers;

    VkCommandPool commandPool{};
    VkCommandBuffer commandBuffer{};

    VkSemaphore imageAvailable{};
    VkSemaphore renderFinished{};
    VkFence inFlightFence{};

    // --- Pipeline system ---
    struct PipelineKey {
        bool blend;
        bool depthTest;
        bool alphaTest;
        bool textured;

        bool operator==(const PipelineKey& o) const {
            return blend==o.blend &&
                   depthTest==o.depthTest &&
                   alphaTest==o.alphaTest &&
                   textured==o.textured;
        }
    };

    struct PipelineKeyHash {
        size_t operator()(const PipelineKey& k) const {
            return (k.blend) |
                   (k.depthTest << 1) |
                   (k.alphaTest << 2) |
                   (k.textured << 3);
        }
    };

    std::unordered_map<PipelineKey, VkPipeline, PipelineKeyHash> pipelines;
    VkPipelineLayout pipelineLayout{};
    VkPipeline currentPipeline{};

    PipelineKey currentState{};

    // --- Buffers ---
    VkBuffer vertexBuffer{};
    VkDeviceMemory vertexMemory{};

    struct Vertex {
        float pos[3];
        float uv[2];
        uint32_t color;
    };

    std::vector<Vertex> cpuVertexBuffer;

    // --- Textures ---
    struct Texture {
        VkImage image{};
        VkDeviceMemory memory{};
        VkImageView view{};
        VkSampler sampler{};
    };

    std::vector<Texture> textures;
    uint32_t boundTexture = 0;

    // --- UBO (matrices) ---
    struct UBO {
        float model[16];
        float view[16];
        float proj[16];
    } ubo;

    VkBuffer uboBuffer{};
    VkDeviceMemory uboMemory{};
};