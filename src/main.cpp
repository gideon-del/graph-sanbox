#include "graph.hpp"
#include "render-graph/resource-manager.hpp"
#include "render-graph/render-graph.hpp"
#include "resources/resource-graph.hpp"
#include "assets/asset-manager.hpp"
#include <iostream>
#include <string>
#include <optional>

#define CHECK(cond)                                                                 \
    do                                                                              \
    {                                                                               \
        if (!(cond))                                                                \
        {                                                                           \
            std::cerr << "FAIL " << __FILE__ << ":" << __LINE__ << "  " #cond "\n"; \
            std::exit(1);                                                           \
        }                                                                           \
    } while (0)

template <typename T>
int count(const std::vector<AliasCommand> &commands)
{
    return std::count_if(commands.begin(), commands.end(),
                         [&](const auto &c)
                         {
                             return std::holds_alternative<T>(c);
                         });
};

bool phasesOrdered(const std::vector<AliasCommand> &commands)
{
    auto phase = [](const AliasCommand &c)
    {
        return std::visit([](const auto &x) -> int
                          {
        using T = std::decay_t<decltype(x)>;

       if constexpr (std::is_same_v<T, DeleteAliasTextureCommand>)
    return 0;
else if constexpr (std::is_same_v<T, DeleteHeapCommand>)
    return 1;
else if constexpr (std::is_same_v<T, CreateHeapCommand>)
    return 2;
else
    return 3; }, c);
    };

    for (int i = 1; i < commands.size(); i++)
    {
        if (phase(commands[i - 1]) > phase(commands[i]))
        {
            return false;
        }
    }

    return true;
}
void test_basicCorrectness()
{
    Graph g;
    NodeID InitializeWindow = 1;
    NodeID InitializeRenderer = 2;
    NodeID LoadScene = 3;
    NodeID LoadGLTF = 4;
    g.addNode(InitializeRenderer);
    g.addNode(InitializeWindow);
    g.addNode(LoadScene);
    g.addNode(LoadGLTF);
    g.addEdge(InitializeWindow, InitializeRenderer);
    g.addEdge(InitializeRenderer, LoadScene);
    g.addEdge(InitializeRenderer, LoadGLTF);

    std::cout << "Total Nodes: " << g.nodeCount() << "\n";
    std::cout << "Total Edges: " << g.edgeCount() << "\n";
    std::cout << "Has InitializeWindow -> InitializeRenderer: " << (g.hasEdge(InitializeWindow, InitializeRenderer) ? "Yes" : "No") << "\n";
    std::cout << "Has InitializeRenderer -> LoadScene : " << (g.hasEdge(InitializeRenderer, LoadScene) ? "Yes" : "No") << "\n";
    std::cout << "Has LoadScene -> InitializeWindow : " << (g.hasEdge(LoadScene, InitializeWindow) ? "Yes" : "No") << "\n";

    std::vector<NodeID> bfsNode = g.bfs(InitializeWindow);

    std::string bfsResult = "BFS Result ";

    for (auto &node : bfsNode)
    {
        bfsResult = bfsResult + " -> " + std::to_string(node);
    }

    std::cout << bfsResult << "\n";

    std::vector<NodeID> dfsNode = g.dfs(InitializeWindow);

    std::string dfsResult = "DFS Result ";
    for (auto &node : dfsNode)
    {
        dfsResult = dfsResult + " -> " + std::to_string(node);
    }

    std::cout << dfsResult << "\n";
}

void test_graphTraversal()
{
    Graph g;
    NodeID A = 1;
    NodeID B = 2;
    NodeID C = 3;
    NodeID D = 4;

    NodeID X = 10, Y = 11, Z = 12;
    g.addNode(A);
    g.addNode(B);
    g.addNode(C);
    g.addNode(D);

    g.addNode(X);
    g.addNode(Y);
    g.addNode(Z);
    g.addEdge(A, B);
    g.addEdge(A, C);
    g.addEdge(B, D);
    g.addEdge(C, D);
    g.addEdge(X, Y);
    g.addEdge(Y, Z);

    std::vector<NodeID> bfsNode = g.bfs(A);

    std::string bfsResult = "BFS Result ";

    for (auto &node : bfsNode)
    {
        bfsResult = bfsResult + " -> " + std::to_string(node);
    }

    std::cout << bfsResult << "\n";

    std::vector<NodeID> dfsNode = g.dfs(A);

    std::string dfsResult = "DFS Result ";
    for (auto &node : dfsNode)
    {
        dfsResult = dfsResult + " -> " + std::to_string(node);
    }

    std::cout << dfsResult << "\n";

    auto result = g.bfsAll();

    std::string bfsAllResult = "BFS AlL Result ";
    for (auto &node : result)
    {
        bfsAllResult = bfsAllResult + " -> " + std::to_string(node);
    }

    std::cout << bfsAllResult << "\n";
}

void test_topoSort()
{
    Graph g;

    NodeID A = 1;
    NodeID B = 2;
    NodeID C = 3;
    NodeID D = 4;

    g.addNode(A);
    g.addNode(C);
    g.addNode(B);
    g.addNode(D);
    g.addEdge(A, C);
    g.addEdge(A, B);
    g.addEdge(B, D);
    g.addEdge(C, D);

    auto result = g.topoSort();

    std::string bfsAllResult = "Topological sort Result ";
    for (auto &node : result)
    {
        bfsAllResult = bfsAllResult + " -> " + std::to_string(node);
    }

    std::cout << bfsAllResult << "\n";

    Graph cycleGraph;

    cycleGraph.addNode(A);
    cycleGraph.addNode(B);
    cycleGraph.addNode(C);
    cycleGraph.addEdge(A, B);
    cycleGraph.addEdge(B, C);
    cycleGraph.addEdge(C, A);

    auto cycleResult = cycleGraph.topoSort();
    std::string cycleResultMessage = "Topological sort cycle result ";
    for (auto &node : cycleResult)
    {
        cycleResultMessage = cycleResultMessage + " -> " + std::to_string(node);
    }

    std::cout << cycleResultMessage << "\n";
}

void test_cycleDetection()
{
    Graph g;

    NodeID A = 1;
    NodeID B = 2;
    NodeID C = 3;
    NodeID D = 4;
    NodeID X = 10, Y = 11, Z = 12;
    g.addNode(A);
    g.addNode(C);
    g.addNode(B);
    g.addNode(D);

    g.addEdge(A, B);
    g.addEdge(B, C);
    g.addEdge(C, A);

    std::cout << "Has cycle: " << (g.hasCycles() ? "Yes" : "No") << std::endl;

    g.addNode(X);
    g.addNode(Y);
    g.addNode(Z);

    g.addEdge(X, Y);
    g.addEdge(Y, Z);
    g.addEdge(Z, X);

    auto cycles = g.findCycles();

    for (auto &cycle : cycles)
    {
        std::cout << "Cycle: ";

        for (auto &node : cycle)
        {
            std::cout << std::to_string(node) << " -> ";
        }
        std::cout << "\n";
    }

    std::unordered_set<NodeID> seen{};

    g.printTree(A, 0, seen);
}

void test_asciiVisualization()
{
    Graph g;
    NodeID ShadowMap = 1;
    NodeID GBuffer = 2;
    NodeID Lighting = 3;
    NodeID PostProcess = 4;

    g.addNode(GBuffer, "GBuffer");
    g.addNode(PostProcess, "PostProcess");
    g.addNode(Lighting, "Lighting");
    g.addNode(ShadowMap, "Shadow");

    g.addEdge(GBuffer, Lighting);
    g.addEdge(ShadowMap, GBuffer);
    g.addEdge(Lighting, PostProcess);

    g.print();
    g.printTopoOrder();
    g.printStats();
    std::unordered_set<NodeID> seen{};
    g.printTree(ShadowMap, 0, seen);
}

void test_assetHandles()
{
    AssetManager manager;

    AssetTextureHandle texHandle = manager.textures.registerAsset(Texture{});
    MeshHandle meshHandle = manager.meshes.registerAsset(Mesh{});
    MaterialHandle materialHandle = manager.materials.registerAsset(Material{});
    std::cout << "Texture Handle id " << texHandle.id << "\n";
    std::cout << "Mesh Handle id " << meshHandle.id << "\n";
    std::cout << "Material Handle id " << materialHandle.id << "\n";
    // Uncomment this line to confirm it fails to compile:
    // MeshHandle wrongHandle = texHandle;
}

void test_assetImport()
{
    AssetManager manager;
    AssetTextureHandle wood = manager.importTexture("wood.jpg");
    std::cout << "Wood Id " << wood.id << "\n";
    auto woodTexture = manager.textures.get(wood);
    std::cout << (int)woodTexture->pixels[0] << " "
              << (int)woodTexture->pixels[1] << " "
              << (int)woodTexture->pixels[2] << " "
              << (int)woodTexture->pixels[3] << "\n";

    AssetTextureHandle missing = manager.importTexture("missing.jpg");
    std::cout << "Missing Id " << missing.id << "\n";
}
void test_assetGraph()
{
    AssetManager manager;
    MaterialHandle material = manager.importMaterial("wood.jpg", "cube.glsl");

    manager.printDependencies();
}

void touchFile(const std::filesystem::path &path)
{
    std::ofstream file(path, std::ios::binary | std::ios::app);
    file.put('\0');
}

void test_assetIncrementalBuild()
{
    AssetManager manager;

    MaterialHandle mat = manager.importMaterial("wood.jpg", "cube.glsl");

    manager.printDependencies();

    touchFile("wood.jpg");
    manager.checkForChanges();

    manager.checkForChanges();
}

void test_assetAsyncLoader()
{
    AssetManager manager;

    AssetTextureHandle handle = manager.requestTextureLoad("wood.jpg");

    int maxAttempts = 100;
    while (manager.textures.isPending(handle) && maxAttempts-- > 0)
    {
        manager.tick();
        std::cout << "Loading asset Max Attempt " << maxAttempts << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    assert(!manager.textures.isPending(handle));
}

void test_resourceGraphModel()
{
    ResourceGraph graph;

    ResourceID texture = 1;
    ResourceID filteredTexture = 2;
    ResourceID outputTexture = 3;

    graph.declareResource(ResourceDesc{
        texture,
        "Texture",
        20,
        ResourceType::Texture,
    });
    graph.declareResource(ResourceDesc{
        filteredTexture,
        "Filtered Texture",
        20,
        ResourceType::Texture,
    });
    graph.declareResource(ResourceDesc{
        outputTexture,
        "Output Texture",
        20,
        ResourceType::Texture,
    });

    graph.addNode({"Apply Filter",
                   {filteredTexture},
                   {texture},
                   [&](ResourceContext &ctx) {}});
    graph.addNode({"Print Output",
                   {},
                   {filteredTexture},
                   [&](ResourceContext &ctx) {}});
    graph.addNode({"Read PNG",
                   {texture},
                   {},
                   [&](ResourceContext &ctx) {}});

    graph.compile();
    std::vector<uint32_t> order = graph.executionOrder();

    std::cout << "Execution order: ";
    for (int i = 0; i < order.size(); i++)
    {
        std::cout << graph.getNode(order[i])->name + (i == (order.size() - 1) ? "\n" : "->");
    }

    graph.addNode({"Failed Pass",
                   {},
                   {outputTexture},
                   [&](ResourceContext &ctx) {}});
    graph.compile();
}

void test_resourceGraphFanInFanOut()
{
    ResourceGraph graph;

    graph.declareResource({1, "Albedo", 4 * 1920 * 1080, ResourceType::Texture, true});
    graph.declareResource({2, "Normal", 8 * 1920 * 1080, ResourceType::Texture, true});
    graph.declareResource({3, "Depth", 4 * 1920 * 1080, ResourceType::DepthBuffer, true});
    graph.declareResource({4, "ShadowMap", 4 * 2048 * 2048, ResourceType::DepthBuffer, true});
    graph.declareResource({5, "LitColor", 8 * 1920 * 1080, ResourceType::Texture, true});
    graph.declareResource({6, "PostProcessTemp", 8 * 1920 * 1080, ResourceType::Texture, true});
    graph.declareResource({7, "SSAO", 8 * 1920 * 1080, ResourceType::Texture, true});

    graph.addNode({.name = "GBufferPass",
                   .produces = {1, 2, 3},
                   .consumes = {},
                   .execute = [&](ResourceContext &ctx)
                   {
                       ctx.set<std::string>(1, "Albedo Filled");
                       ctx.set<std::string>(2, "Normal Filled");
                       ctx.set<std::string>(3, "Depth Filled");
                       std::cout << "G-buffer produced";
                   }});
    graph.addNode({.name = "LightingPass",
                   .produces = {5},
                   .consumes = {1, 2, 3, 4},
                   .execute = [&](ResourceContext &ctx)
                   {
                       auto &albedo = ctx.get<std::string>(1);
                       auto &normal = ctx.get<std::string>(2);
                       auto &depth = ctx.get<std::string>(3);

                       std::cout << "  Lighting: consumed albedo " << albedo << "\n";
                       std::cout << "  Lighting: consumed normal " << normal << "\n";
                       std::cout << "  Lighting: consumed depth " << depth << "\n";
                   }});
    graph.addNode({.name = "ShadowPass",
                   .produces = {4},
                   .consumes = {},
                   .execute = [&](ResourceContext &ctx) {}});
    graph.addNode({.name = "PostProcess",
                   .produces = {6},
                   .consumes = {5},
                   .execute = [&](ResourceContext &ctx) {}});
    graph.addNode({.name = "SSAO",
                   .produces = {7},
                   .consumes = {6, 3},
                   .execute = [&](ResourceContext &ctx) {}});

    auto pipeline = graph.compile();

    graph.printResourceFlow();
    graph.printLifetimes();
    // std::cout << "Peak memory: " << graph.peakMemoryBytes() << "\n";

    graph.printAliasingReport();

    graph.execute(pipeline);
};

void setupTextures(ResourceManager &rm)
{

    rm.createTexture("X", GPUTextureDesc{.size = ResourceSize::textureScreenRelative()}, ResourceLifetimeType::Transient);
    rm.createTexture("Y", GPUTextureDesc{.size = ResourceSize::textureScreenRelative()}, ResourceLifetimeType::Transient);
    rm.createTexture("Z", GPUTextureDesc{.size = ResourceSize::textureScreenRelative()}, ResourceLifetimeType::Transient);
    rm.createTexture("Bloom", GPUTextureDesc{.size = ResourceSize::textureScreenRelative()}, ResourceLifetimeType::Transient);
    rm.createTexture("Shadow", GPUTextureDesc{.size = ResourceSize::textureAbsolute(2048, 2048)}, ResourceLifetimeType::Transient);
}

void test_diffCommand()
{

    uint32_t FRAME_WIDTH = 1600;
    uint32_t FRAME_HEIGHT = 1200;

    ResourceManager rm{};

    setupTextures(rm);

    RenderGraphBuilder graph{rm};

    auto x = graph
                 .createTexture("X");
    auto y = graph
                 .createTexture("Y");
    auto z = graph
                 .createTexture("Z");

    graph.addRenderPass("A")
        .write(x, ResourceState::ShaderWrite);
    graph.addRenderPass("B")
        .write(y, ResourceState::ShaderWrite)
        .read(x, ResourceState::ShaderRead);
    graph.addRenderPass("C")
        .write(z, ResourceState::ShaderWrite)
        .read(y, ResourceState::ShaderRead);

    graph.compile();

    auto &lifetimes = graph.lifetimes();

    rm.buildResources({FRAME_WIDTH, FRAME_HEIGHT});

    auto firstCommand = rm.buildTransientResources(FRAME_WIDTH, FRAME_HEIGHT, lifetimes);

    RenderGraphBuilder secondGraph{rm};
    auto sz = secondGraph
                  .createTexture("Z");
    auto sx = secondGraph
                  .createTexture("X");
    auto sy = secondGraph
                  .createTexture("Y");

    secondGraph.addRenderPass("A")
        .write(sx, ResourceState::ShaderWrite);
    secondGraph.addRenderPass("B")
        .write(sy, ResourceState::ShaderWrite)
        .read(sx, ResourceState::ShaderRead);
    secondGraph.addRenderPass("C")
        .write(sz, ResourceState::ShaderWrite)
        .read(sy, ResourceState::ShaderRead);

    secondGraph.compile();
    auto secondLifetime = secondGraph.lifetimes();

    auto secondCommand = rm.buildTransientResources(FRAME_WIDTH, FRAME_HEIGHT, secondLifetime);

    CHECK(count<CreateHeapCommand>(firstCommand) == 2);
    CHECK(count<CreateAliasTextureCommand>(firstCommand) == 3);
    CHECK(count<DeleteAliasTextureCommand>(firstCommand) == 0);
    CHECK(phasesOrdered(firstCommand));
    CHECK(secondCommand.empty());

    auto &plan = rm.aliasPlan();

    CHECK(plan.textureToHeap.at(*rm.getTextureHandle("X")) ==
          plan.textureToHeap.at(*rm.getTextureHandle("Z")));
    CHECK(plan.textureToHeap.at(*rm.getTextureHandle("X")) !=
          plan.textureToHeap.at(*rm.getTextureHandle("Y")));
}
void test_renderGraphReadsAndWrites()
{

    ResourceManager rm{};

    setupTextures(rm);

    RenderGraphBuilder graph{rm};

    auto x = graph
                 .createTexture("X");

    graph.addRenderPass("A")
        .write(x, ResourceState::ShaderWrite);
    graph.addRenderPass("B")
        .read(x, ResourceState::ShaderRead);
    graph.addRenderPass("C")
        .write(x, ResourceState::ShaderWrite);

    graph.compile();

    // RAW
    CHECK(graph.orderOf("A") < graph.orderOf("B"));
    // WAW
    CHECK(graph.orderOf("A") < graph.orderOf("C"));
    // WAR
    CHECK(graph.orderOf("B") < graph.orderOf("C"));

    // No cycle: every pass made it into the order
    CHECK(graph.executionOrder().size() == 3);
}

void buildRenderPass(ResourceManager &rm, bool withBloom, RenderGraphBuilder &g)
{

    auto x = g
                 .createTexture("X");
    auto y = g
                 .createTexture("Y");
    auto z = g
                 .createTexture("Z");
    auto shadow = g
                      .createTexture("Shadow");

    g.addRenderPass("A")
        .write(x, ResourceState::ShaderWrite)
        .write(shadow, ResourceState::ShaderWrite);
    g.addRenderPass("B")
        .write(y, ResourceState::ShaderWrite)
        .read(x, ResourceState::ShaderRead);
    g.addRenderPass("C")
        .write(z, ResourceState::ShaderWrite)
        .read(y, ResourceState::ShaderRead);
    auto bloom = g.createTexture("Bloom");
    if (withBloom)
        g.addRenderPass("Bloom").write(bloom, ResourceState::ShaderWrite).read(z, ResourceState::ShaderRead);
};

void test_renderPassToggle()
{
    ResourceManager rm{};

    setupTextures(rm);

    RenderGraphBuilder graph{rm};

    buildRenderPass(rm, true, graph);
    graph.compile();

    auto &lifetimes = graph.lifetimes();

    uint32_t FRAME_WIDTH = 1600;
    uint32_t FRAME_HEIGHT = 1200;

    auto firstCommand = rm.buildTransientResources(FRAME_WIDTH, FRAME_HEIGHT, lifetimes);

    RenderGraphBuilder secondGraph{rm};

    buildRenderPass(rm, false, secondGraph);
    secondGraph.compile();

    auto &secondLifetime = secondGraph.lifetimes();

    auto secondCommand = rm.buildTransientResources(FRAME_WIDTH, FRAME_HEIGHT, secondLifetime);

    CHECK(count<DeleteAliasTextureCommand>(secondCommand) == 1);
}

void test_resize()
{
    ResourceManager rm{};
    setupTextures(rm);
    RenderGraphBuilder graph{rm};
    buildRenderPass(rm, true, graph);
    graph.compile();
    auto lifetimes = graph.lifetimes();

    rm.buildTransientResources(1600, 1200, lifetimes);
    auto shadow = *rm.getTextureHandle("Shadow");
    auto y = *rm.getTextureHandle("Y");
    auto shadowHeap = rm.aliasPlan().textureToHeap.at(shadow);
    auto yHeap = rm.aliasPlan().textureToHeap.at(y);
    size_t ySizeBefore = rm.aliasPlan().groups.at(yHeap).requirement.size;

    // Shrink: heaps stay, textures recreated smaller
    auto shrink = rm.buildTransientResources(800, 600, lifetimes);
    CHECK(count<DeleteHeapCommand>(shrink) == 0);
    CHECK(count<CreateHeapCommand>(shrink) == 0);
    CHECK(rm.aliasPlan().textureExtents.at(y) == Extent2D(800, 600));

    // Grow back to the original: heaps already fit, nothing recreated
    auto regrow = rm.buildTransientResources(1600, 1200, lifetimes);
    CHECK(count<CreateHeapCommand>(regrow) == 0);

    // Grow beyond: screen-relative heaps grow, same slots
    auto grow = rm.buildTransientResources(2200, 2200, lifetimes);
    CHECK(count<CreateHeapCommand>(grow) > 0);
    CHECK(phasesOrdered(grow));
    CHECK(rm.aliasPlan().textureToHeap.at(y) == yHeap);

    CHECK(rm.aliasPlan().groups.at(yHeap).requirement.size > ySizeBefore);

    // Absolute-size shadow map: untouched by any resize
    CHECK(rm.aliasPlan().textureToHeap.at(shadow) == shadowHeap);
    CHECK(rm.aliasPlan().textureExtents.at(shadow) == Extent2D(2048, 2048));

    // Growing within a shared heap that's already big enough: no heap work, Y just resized
    auto absorbed = rm.buildTransientResources(2000, 2000, lifetimes);
    CHECK(count<CreateHeapCommand>(absorbed) == 0);
    CHECK(rm.aliasPlan().textureExtents.at(y) == Extent2D(2000, 2000));
}

void test_exclusion()
{
    ResourceManager rm{};
    rm.createTexture("Transient", GPUTextureDesc{.size = ResourceSize::textureScreenRelative()}, ResourceLifetimeType::Transient);
    rm.createTexture("Persistent", GPUTextureDesc{.size = ResourceSize::textureScreenRelative()}, ResourceLifetimeType::Persistent);
    rm.createTemporalTexture("History", GPUTextureDesc{.size = ResourceSize::textureScreenRelative()}, ResourceLifetimeType::Transient, 2);
    rm.createPerFrameTexture("PerFrame", GPUTextureDesc{.size = ResourceSize::textureScreenRelative()}, ResourceLifetimeType::Transient);

    RenderGraphBuilder g{rm};
    auto t = g.createTexture("Transient");
    auto p = g.createTexture("Persistent");
    auto h = g.createTemporalTexture("History");
    auto f = g.createTexture("PerFrame");
    g.addRenderPass("A").write(t, ResourceState::ShaderWrite).write(p, ResourceState::ShaderWrite).write(h, ResourceState::ShaderWrite).write(f, ResourceState::ShaderWrite);
    g.compile();

    rm.buildResources({1600, 1200});
    rm.buildTransientResources(1600, 1200, g.lifetimes());
    auto &plan = rm.aliasPlan();

    // Only the transient single texture is aliased
    CHECK(plan.textureToHeap.contains(*rm.getTextureHandle("Transient")));
    CHECK(!plan.textureToHeap.contains(*rm.getTextureHandle("Persistent")));
    CHECK(!plan.textureToHeap.contains(*rm.getTextureHandle("History")));
    CHECK(!plan.textureToHeap.contains(*rm.getTextureHandle("PerFrame")));
    CHECK(plan.groups.size() == 1);

    // Excluded textures still exist, built by buildResources instead
    CHECK(rm.get(*rm.getTextureHandle("Persistent")) != nullptr);
    CHECK(rm.get(*rm.getTextureHandle("History")) != nullptr);
    CHECK(rm.get(*rm.getTextureHandle("PerFrame")) != nullptr);
}
void test_historyValidity()
{
    ResourceManager rm{};
    rm.createTexture("Transient", GPUTextureDesc{.size = ResourceSize::textureScreenRelative()}, ResourceLifetimeType::Transient);
    rm.createTexture("Persistent", GPUTextureDesc{.size = ResourceSize::textureScreenRelative()}, ResourceLifetimeType::Persistent);
    rm.createTemporalTexture("History", GPUTextureDesc{.size = ResourceSize::textureScreenRelative()}, ResourceLifetimeType::Transient, 2);
    rm.createPerFrameTexture("PerFrame", GPUTextureDesc{.size = ResourceSize::textureScreenRelative()}, ResourceLifetimeType::Transient);

    RenderGraphBuilder g{rm};
    auto t = g.createTexture("Transient");
    auto p = g.createTexture("Persistent");
    auto h = g.createTemporalTexture("History");
    auto f = g.createTexture("PerFrame");
    g.addRenderPass("A").write(t, ResourceState::ShaderWrite).write(p, ResourceState::ShaderWrite).write(h, ResourceState::ShaderWrite).write(f, ResourceState::ShaderWrite);
    g.compile();

    rm.buildResources({1600, 1200});
    rm.buildTransientResources(1600, 1200, g.lifetimes());

    // History Invalid at start
    CHECK(!rm.isHistoryValid());

    rm.advanceFrame();

    // History valid in next frame
    CHECK(rm.isHistoryValid());

    rm.resize(800, 600);
    // History valid invalid at resize

    CHECK(!rm.isHistoryValid());
}
void test_temporalRotation()
{
    ResourceManager rm{};
    rm.createTexture("Transient", GPUTextureDesc{.size = ResourceSize::textureScreenRelative()}, ResourceLifetimeType::Transient);
    rm.createTexture("Persistent", GPUTextureDesc{.size = ResourceSize::textureScreenRelative()}, ResourceLifetimeType::Persistent);
    auto historyHandle = rm.createTemporalTexture("History", GPUTextureDesc{.size = ResourceSize::textureScreenRelative()}, ResourceLifetimeType::Transient, 2);
    rm.createPerFrameTexture("PerFrame", GPUTextureDesc{.size = ResourceSize::textureScreenRelative()}, ResourceLifetimeType::Transient);

    RenderGraphBuilder g{rm};
    auto t = g.createTexture("Transient");
    auto p = g.createTexture("Persistent");
    auto h = g.createTemporalTexture("History");
    auto f = g.createTexture("PerFrame");
    g.addRenderPass("A").write(t, ResourceState::ShaderWrite).write(p, ResourceState::ShaderWrite).write(h, ResourceState::ShaderWrite).write(f, ResourceState::ShaderWrite);
    g.compile();

    rm.buildResources({1600, 1200});
    rm.buildTransientResources(1600, 1200, g.lifetimes());

    auto firstFrameResource = rm.current(historyHandle);

    rm.advanceFrame();

    auto previousFrameResource = rm.previous(historyHandle);

    CHECK(firstFrameResource == previousFrameResource);
}

void test_transitionWhole()
{
    ResourceManager rm{};
    TextureState state(6, 1);
    std::vector<TextureAccess> batch;
    TextureHandle h{0};

    // Whole texture from Undefined: one barrier, still uniform
    rm.transition(state, {}, ResourceState::ShaderRead, batch, h, false);
    CHECK(batch.size() == 1);
    CHECK(batch[0].range.from == ResourceState::Undefined);
    CHECK(batch[0].state == ResourceState::ShaderRead);
    CHECK(batch[0].range.baseMip == 0 && batch[0].range.mipCount == 6);
    CHECK(state.isUniform());

    // Same state again: no barrier
    batch.clear();
    rm.transition(state, {}, ResourceState::ShaderRead, batch, h, false);
    CHECK(batch.empty());
}

void test_transitionSingleMip()
{
    ResourceManager rm{};
    TextureState state(6, 1);
    std::vector<TextureAccess> batch;
    TextureHandle h{0};

    rm.transition(state, {}, ResourceState::ShaderRead, batch, h, false);
    batch.clear();

    // Mip 2 only: one barrier for exactly mip 2
    rm.transition(state, {.baseMip = 2, .mipCount = 1}, ResourceState::ShaderWrite, batch, h, false);
    CHECK(batch.size() == 1);
    CHECK(batch[0].range.baseMip == 2 && batch[0].range.mipCount == 1);
    CHECK(batch[0].range.from == ResourceState::ShaderRead);
    CHECK(!state.isUniform());

    // The other mips are untouched
    for (uint32_t mip : {0u, 1u, 3u, 4u, 5u})
        CHECK(state.get(state.subIndex(mip, 0)) == ResourceState::ShaderRead);
    CHECK(state.get(state.subIndex(2, 0)) == ResourceState::ShaderWrite);
}

void test_transitionRunsAndCollapse()
{
    ResourceManager rm{};
    TextureState state(6, 1);
    std::vector<TextureAccess> batch;
    TextureHandle h{0};

    rm.transition(state, {}, ResourceState::ShaderRead, batch, h, false);
    rm.transition(state, {.baseMip = 2, .mipCount = 1}, ResourceState::ShaderWrite, batch, h, false);
    batch.clear();

    // Whole texture to ShaderWrite: mips 0-1 and 3-5 need barriers, mip 2 is already there
    rm.transition(state, {}, ResourceState::ShaderWrite, batch, h, false);
    CHECK(batch.size() == 2);

    CHECK(batch[0].range.baseMip == 0 && batch[0].range.mipCount == 2);
    CHECK(batch[0].range.from == ResourceState::ShaderRead);
    CHECK(batch[1].range.baseMip == 3 && batch[1].range.mipCount == 3);
    CHECK(batch[1].range.from == ResourceState::ShaderRead);

    // Everything matches again: collapsed back to uniform
    CHECK(state.isUniform());
    CHECK(state.get(0) == ResourceState::ShaderWrite);
}

void test_transitionMipChain()
{
    ResourceManager rm{};
    TextureState state(6, 1);
    std::vector<TextureAccess> batch;
    TextureHandle h{0};

    rm.transition(state, {}, ResourceState::ShaderWrite, batch, h, false); // mip 0 written first

    // Downsample chain: read mip i, write mip i+1
    for (uint32_t i = 0; i < 5; i++)
    {
        batch.clear();
        rm.transition(state, {.baseMip = i, .mipCount = 1}, ResourceState::ShaderRead, batch, h, false);
        rm.transition(state, {.baseMip = i + 1, .mipCount = 1}, ResourceState::ShaderWrite, batch, h, false);

        // The read always needs a barrier; the write only once the mip was read before
        CHECK(batch.size() >= 1);
        for (auto &b : batch)
            CHECK(b.range.mipCount == 1); // never more than one mip per barrier
        CHECK(batch[0].range.baseMip == i);
    }

    // After the chain: mips 0-4 read, mip 5 written
    for (uint32_t mip = 0; mip < 5; mip++)
        CHECK(state.get(state.subIndex(mip, 0)) == ResourceState::ShaderRead);
    CHECK(state.get(state.subIndex(5, 0)) == ResourceState::ShaderWrite);
}
void test_aliasedResetOnAdvance()
{
    ResourceManager rm{};
    setupTextures(rm);
    rm.createTexture("Persistent", GPUTextureDesc{.size = ResourceSize::textureScreenRelative()},
                     ResourceLifetimeType::Persistent);

    RenderGraphBuilder g{rm};
    buildRenderPass(rm, true, g);
    auto p = g.createTexture("Persistent");
    g.addRenderPass("P").write(p, ResourceState::ShaderWrite);
    g.compile();

    rm.buildResources({1600, 1200});
    rm.buildTransientResources(1600, 1200, g.lifetimes());

    auto *x = rm.get(*rm.getTextureHandle("X"));
    auto *persistent = rm.get(*rm.getTextureHandle("Persistent"));
    x->state.updateAll(ResourceState::ShaderRead);
    persistent->state.updateAll(ResourceState::ShaderRead);

    rm.advanceFrame();

    CHECK(x->state.get(0) == ResourceState::Undefined);           // aliased: reset
    CHECK(persistent->state.get(0) == ResourceState::ShaderRead); // persistent: kept
}
int main()
{

    test_diffCommand();
    test_renderGraphReadsAndWrites();
    test_renderPassToggle();
    test_resize();
    test_exclusion();
    test_historyValidity();
    test_temporalRotation();
    test_transitionWhole();
    test_transitionSingleMip();
    test_transitionRunsAndCollapse();
    test_transitionMipChain();
    test_aliasedResetOnAdvance();
    std::cout << "All Good" << std::endl;
    return 0;
}
