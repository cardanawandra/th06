#include "FixedFunctionGL.hpp"
#include "SDLCompat.hpp"
#include <vulkan/vulkan.h>
#include <SDL_vulkan.h>
#include <vector>
#include <unordered_map>
#include <cstring>

#define MAX_FRAMES 2

// =======================================================
// PIPELINE KEY
// =======================================================
struct PipelineKey {
    bool blend;
    bool depthTest;
    bool alphaTest;
    bool textured;

    bool operator==(const PipelineKey& o) const {
        return blend == o.blend &&
               depthTest == o.depthTest &&
               alphaTest == o.alphaTest &&
               textured == o.textured;
    }
};

struct PipelineHash {
    size_t operator()(const PipelineKey& k) const {
        return (k.blend) |
               (k.depthTest << 1) |
               (k.alphaTest << 2) |
               (k.textured << 3);
    }
};

// =======================================================
// TEXTURE
// =======================================================
struct VKTexture {
    VkImage image{};
    VkDeviceMemory memory{};
    VkImageView view{};
    VkSampler sampler{};
};

// =======================================================
// VERTEX
// =======================================================
struct VKVertex {
    float pos[3];
    float uv[2];
    uint32_t color;
};

// =======================================================
// MAIN CLASS FIELDS (inside FixedFunctionGL via reinterpretation)
// =======================================================

static VkInstance instance;
static VkDevice device;
static VkPhysicalDevice phys;
static VkSurfaceKHR surface;

static VkQueue graphicsQueue;
static VkQueue presentQueue;

static VkSwapchainKHR swapchain;
static VkFormat swapFormat;
static VkExtent2D swapExtent;

static std::vector<VkImage> swapImages;
static std::vector<VkImageView> swapViews;
static std::vector<VkFramebuffer> framebuffers;

static VkRenderPass renderPass;

static VkCommandPool cmdPool;
static VkCommandBuffer cmd;

static VkSemaphore imageAvailable;
static VkSemaphore renderFinished;
static VkFence inFlightFence;

static VkPipelineLayout pipelineLayout;

static std::unordered_map<PipelineKey, VkPipeline, PipelineHash> pipelines;
static VkPipeline currentPipeline;

static std::vector<VKVertex> vertexStream;

static std::vector<VKTexture> textures;
static uint32_t boundTexture = 0;

// UBO (matrices)
static struct {
    float model[16];
    float view[16];
    float proj[16];
} ubo;

// =======================================================
// UTIL: ERROR CHECK
// =======================================================
#define VK_CHECK(x) if ((x) != VK_SUCCESS) { SDL_Log("Vulkan error"); abort(); }

// =======================================================
// PIPELINE CREATION (core of fixed-function emulation)
// =======================================================
static VkPipeline CreatePipeline(const PipelineKey& key)
{
    // NOTE: shaders MUST exist
    VkShaderModule vertShader; // load SPV externally
    VkShaderModule fragShader;

    // (placeholder: assume created elsewhere)

    VkPipelineShaderStageCreateInfo stages[2]{};

    // Vertex stage
    stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = vertShader;
    stages[0].pName = "main";

    // Fragment stage
    stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = fragShader;
    stages[1].pName = "main";

    VkPipelineVertexInputStateCreateInfo vi{};
    vi.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    VkPipelineInputAssemblyStateCreateInfo ia{};
    ia.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkPipelineViewportStateCreateInfo vp{};
    vp.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;

    VkPipelineRasterizationStateCreateInfo rs{};
    rs.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rs.polygonMode = VK_POLYGON_MODE_FILL;
    rs.cullMode = VK_CULL_MODE_NONE;
    rs.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo ms{};
    ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo ds{};
    ds.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    ds.depthTestEnable = key.depthTest;
    ds.depthWriteEnable = key.depthTest;

    VkPipelineColorBlendAttachmentState blend{};
    blend.colorWriteMask = 0xF;

    if (key.blend)
    {
        blend.blendEnable = VK_TRUE;
        blend.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        blend.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        blend.colorBlendOp = VK_BLEND_OP_ADD;
    }

    VkPipelineColorBlendStateCreateInfo cb{};
    cb.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    cb.attachmentCount = 1;
    cb.pAttachments = &blend;

    VkGraphicsPipelineCreateInfo ci{};
    ci.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    ci.stageCount = 2;
    ci.pStages = stages;
    ci.pVertexInputState = &vi;
    ci.pInputAssemblyState = &ia;
    ci.pViewportState = &vp;
    ci.pRasterizationState = &rs;
    ci.pMultisampleState = &ms;
    ci.pDepthStencilState = &ds;
    ci.pColorBlendState = &cb;
    ci.layout = pipelineLayout;
    ci.renderPass = renderPass;

    VkPipeline pipeline;
    vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &ci, nullptr, &pipeline);

    return pipeline;
}

// =======================================================
// INIT
// =======================================================
GfxInterface *FixedFunctionGL::Init()
{
    auto* gfx = new FixedFunctionGL();

    SDL_Init(SDL_INIT_VIDEO);

    gfx->window = SDL_CreateWindow(
        TH_WINDOW_TITLE,
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        g_GameWindow.GAME_WINDOW_WIDTH_REAL,
        g_GameWindow.GAME_WINDOW_HEIGHT_REAL,
        SDL_WINDOW_VULKAN | SDL_WINDOW_SHOWN
    );

    // INSTANCE
    VkApplicationInfo app{};
    app.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app.apiVersion = VK_API_VERSION_1_2;

    VkInstanceCreateInfo ci{};
    ci.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    ci.pApplicationInfo = &app;

    SDL_Vulkan_CreateInstance(gfx->window, &ci, nullptr, &instance);
    SDL_Vulkan_CreateSurface(gfx->window, instance, &surface);

    // DEVICE
    uint32_t count = 1;
    vkEnumeratePhysicalDevices(instance, &count, &phys);

    float prio = 1.0f;
    VkDeviceQueueCreateInfo qci{};
    qci.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    qci.queueCount = 1;
    qci.pQueuePriorities = &prio;

    VkDeviceCreateInfo dci{};
    dci.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    dci.queueCreateInfoCount = 1;
    dci.pQueueCreateInfos = &qci;

    vkCreateDevice(phys, &dci, nullptr, &device);
    vkGetDeviceQueue(device, 0, 0, &graphicsQueue);

    // SWAPCHAIN
    VkSwapchainCreateInfoKHR sci{};
    sci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    sci.surface = surface;
    sci.minImageCount = 2;
    sci.imageFormat = VK_FORMAT_B8G8R8A8_UNORM;
    sci.imageExtent = {(u32)g_GameWindow.GAME_WINDOW_WIDTH_REAL,
                       (u32)g_GameWindow.GAME_WINDOW_HEIGHT_REAL};

    vkCreateSwapchainKHR(device, &sci, nullptr, &swapchain);

    // COMMAND POOL
    VkCommandPoolCreateInfo pci{};
    pci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    vkCreateCommandPool(device, &pci, nullptr, &cmdPool);

    VkCommandBufferAllocateInfo ai{};
    ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    ai.commandPool = cmdPool;
    ai.commandBufferCount = 1;
    ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

    vkAllocateCommandBuffers(device, &ai, &cmd);

    // SYNC
    VkSemaphoreCreateInfo si{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    vkCreateSemaphore(device, &si, nullptr, &imageAvailable);
    vkCreateSemaphore(device, &si, nullptr, &renderFinished);

    VkFenceCreateInfo fi{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    fi.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    vkCreateFence(device, &fi, nullptr, &inFlightFence);

    return gfx;
}

// =======================================================
// SET TRANSFORM
// =======================================================
void FixedFunctionGL::SetTransformMatrix(TransformMatrix type, const ZunMatrix& m)
{
    if (type == MODELVIEW_MATRIX)
        memcpy(ubo.model, &m, sizeof(m));
    if (type == PROJECTION_MATRIX)
        memcpy(ubo.proj, &m, sizeof(m));
}

// =======================================================
// ENABLE STATE
// =======================================================
void FixedFunctionGL::Enable(Capabilities cap)
{
    if (cap == CAPS_BLEND) currentState.blend = true;
    if (cap == CAPS_DEPTH_TEST) currentState.depthTest = true;
}

// =======================================================
// BLEND
// =======================================================
void FixedFunctionGL::SetBlendMode(BlendMode mode)
{
    currentState.blend = true;
}

// =======================================================
// DRAW
// =======================================================
void FixedFunctionGL::Draw(PrimitiveType type, i32 start, i32 count)
{
    auto it = pipelines.find(currentState);

    if (it == pipelines.end())
    {
        currentPipeline = CreatePipeline(currentState);
        pipelines[currentState] = currentPipeline;
    }
    else currentPipeline = it->second;

    vkBeginCommandBuffer(cmd, &VkCommandBufferBeginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO});

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, currentPipeline);

    vkCmdDraw(cmd, count, 1, start, 0);

    vkEndCommandBuffer(cmd);
}

// =======================================================
// SWAP
// =======================================================
void FixedFunctionGL::SwapBuffers()
{
    VkSubmitInfo si{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    si.commandBufferCount = 1;
    si.pCommandBuffers = &cmd;

    vkQueueSubmit(graphicsQueue, 1, &si, inFlightFence);

    VkPresentInfoKHR pi{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
    pi.swapchainCount = 1;
    pi.pSwapchains = &swapchain;

    vkQueuePresentKHR(presentQueue, &pi);

    vkWaitForFences(device, 1, &inFlightFence, VK_TRUE, UINT64_MAX);
    vkResetFences(device, 1, &inFlightFence);

    vkResetCommandBuffer(cmd, 0);
}