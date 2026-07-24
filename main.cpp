#include "./includes/graph.hpp"
#include "./includes/resources/resource-graph.hpp"
#include "./includes/assets/asset-manager.hpp"
#include <iostream>
#include <string>
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

    TextureHandle texHandle = manager.textures.registerAsset(Texture{});
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
    TextureHandle wood = manager.importTexture("wood.jpg");
    std::cout << "Wood Id " << wood.id << "\n";
    auto woodTexture = manager.textures.get(wood);
    std::cout << (int)woodTexture->pixels[0] << " "
              << (int)woodTexture->pixels[1] << " "
              << (int)woodTexture->pixels[2] << " "
              << (int)woodTexture->pixels[3] << "\n";

    TextureHandle missing = manager.importTexture("missing.jpg");
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

    TextureHandle handle = manager.requestTextureLoad("wood.jpg");

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
int main()
{
    // test_basicCorrectness();
    // test_graphTraversal();
    // test_topoSort();
    // test_cycleDetection();

    // test_asciiVisualization();

    // test_assetHandles();
    // test_assetImport();
    // test_assetGraph();
    // test_assetIncrementalBuild();
    // test_assetAsyncLoader();

    // test_resourceGraphModel();

    test_resourceGraphFanInFanOut();
}