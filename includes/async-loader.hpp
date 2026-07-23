#pragma once
#include "spsc-queue.hpp"
#include <queue>
#include <mutex>
#include <thread>
#include <variant>
#include "./assets/asset-handle.hpp"
#include "./assets/importers/importers.hpp"

struct LoadRequest
{
    AssetID id;
    std::filesystem::path path;
    AssetType type;
};

struct LoadResult
{
    AssetID id;
    AssetType type;
    bool success;
    std::string error;
    std::variant<Texture, Mesh, Shader> asset;
    std::filesystem::path path;
};

class AsyncLoader
{

    std::vector<std::thread> m_workers;
    bool m_shutdown = false;

    SPSCQueue<LoadResult, 256> m_readyQueue;

    std::queue<LoadRequest> m_pendingRequest;
    std::mutex m_mutex;
    std::condition_variable m_cv;

public:
    AsyncLoader()
    {
        m_workers.push_back(std::thread{[&]
                                        {
                                            workerLoop();
                                        }});
    }

    ~AsyncLoader()
    {
        m_shutdown = true;
        m_cv.notify_all();
        for (auto &worker : m_workers)
            worker.join();
    }

    void submitRequest(LoadRequest req)
    {
        {
            std::lock_guard lock(m_mutex);
            m_pendingRequest.push(std::move(req));
            m_cv.notify_one();
        }
    }

    std::optional<LoadResult> pollResult() { return m_readyQueue.pop(); }

private:
    void workerLoop()
    {

        while (true)
        {

            LoadRequest req;
            {
                std::unique_lock lock(m_mutex);
                m_cv.wait(lock, [&]
                          { return !m_pendingRequest.empty() || m_shutdown; });
                if (m_shutdown && m_pendingRequest.empty())
                    return;
                req = std::move(m_pendingRequest.front());
                m_pendingRequest.pop();
            }

            LoadResult result{req.id, req.type, false, "", {}, req.path};

            switch (req.type)
            {
            case AssetType::Texture:
            {
                PNGImporter importer{};
                auto texture = importer.import(req.path);
                result.success = texture.has_value();
                if (result.success)
                    result.asset = std::move(*texture);
            }
            }

            while (!m_readyQueue.push(result))
            {
                std::this_thread::yield();
            }
        }
    }
};
