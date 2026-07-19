#include <algorithm>
#include <random>
#include "blue/log.h"
#include "cluster_node.h"

namespace blue
{
    namespace cluster
    {
        static blue::Logger::LoggerPtr g_logger = BLUE_LOG_NAME("system");
        std::string nodeStateToString(NodeState state)
        {
            switch (state)
            {
            case NodeState::OFFLINE:
                return "offline";
            case NodeState::ONLINE:
                return "online";
            case NodeState::FATL:
                return "fail";
            case NodeState::HANDSHAKE:
                return "handshake";
            case NodeState::LEAVING:
                return "leaving";
            case NodeState::SYNCING:
                return "syncing";
            default:
                return "unknown";
            }
        }

        std::atomic<uint32_t> ClusterNodeManager::m_next_node_id{1};

        void ClusterNodeManager::addNode(const ClusterNode &node)
        {
            std::unique_lock<std::shared_mutex> lock(m_mutex);
            m_nodes[node.id] = node;
            BLUE_LOG_INFO(g_logger) << "Cluster node added: " << node.id
                                    << " (" << node.ip << ":" << node.port << ")";
        }

        bool ClusterNodeManager::removeNode(uint32_t id)
        {
            std::unique_lock<std::shared_mutex> lock(m_mutex);
            auto it = m_nodes.find(id);
            if (it == m_nodes.end())
            {
                return false;
            }
            m_nodes.erase(it);
            BLUE_LOG_INFO(g_logger) << "Cluster node removed: " << id;
            return true;
        }

        ClusterNode *ClusterNodeManager::getNode(uint32_t node_id)
        {
            std::shared_lock<std::shared_mutex> lock(m_mutex);
            auto it = m_nodes.find(node_id);
            return it != m_nodes.end() ? &it->second : nullptr;
        }

        const ClusterNode *ClusterNodeManager::getNode(uint32_t node_id) const
        {
            std::shared_lock<std::shared_mutex> lock(m_mutex);
            auto it = m_nodes.find(node_id);
            return it != m_nodes.end() ? &it->second : nullptr;
        }

        std::vector<ClusterNode> ClusterNodeManager::getAllNodes() const
        {
            std::shared_lock<std::shared_mutex> lock(m_mutex);
            std::vector<ClusterNode> res;
            res.reserve(m_nodes.size());
            for (const auto &[id, node] : m_nodes)
            {
                res.push_back(node);
            }
            return res;
        }

        std::vector<ClusterNode> ClusterNodeManager::getAliveNodes() const
        {
            std::shared_lock<std::shared_mutex> lock(m_mutex);
            std::vector<ClusterNode> res;
            for (const auto &[id, node] : m_nodes)
            {
                if (node.isAlive())
                {
                    res.push_back(node);
                }
            }
            return res;
        }

        std::vector<ClusterNode> ClusterNodeManager::getMasterNodes() const
        {
            std::shared_lock<std::shared_mutex> lock(m_mutex);
            std::vector<ClusterNode> res;
            for (const auto &[id, node] : m_nodes)
            {
                if (node.is_master && node.isAlive())
                {
                    res.push_back(node);
                }
            }
            return res;
        }

        std::vector<ClusterNode> ClusterNodeManager::getSlaveNodes() const
        {
            std::shared_lock<std::shared_mutex> lock(m_mutex);
            std::vector<ClusterNode> res;
            for (const auto &[id, node] : m_nodes)
            {
                if (!node.is_master && node.isAlive())
                {
                    res.push_back(node);
                }
            }
            return res;
        }

        void ClusterNodeManager::updateNodeState(uint32_t node_id, NodeState state)
        {
            std::unique_lock<std::shared_mutex> lock(m_mutex);
            auto it = m_nodes.find(node_id);
            if (it != m_nodes.end())
            {
                it->second.state->store(state, std::memory_order_release);
                BLUE_LOG_INFO(g_logger) << "Node " << node_id
                                        << " state: " << nodeStateToString(state);
            }
        }

        void ClusterNodeManager::updateNodeSlots(uint32_t node_id, const std::vector<uint16_t> &slots)
        {
            std::unique_lock<std::shared_mutex> lock(m_mutex);
            auto it = m_nodes.find(node_id);
            if (it != m_nodes.end())
            {
                it->second.slots = slots;
                BLUE_LOG_INFO(g_logger) << "Node " << node_id
                                        << " slots updated: " << slots.size() << " slots";
            }
        }

        size_t ClusterNodeManager::getNodeCount() const
        {
            std::shared_lock<std::shared_mutex> lock(m_mutex);
            return m_nodes.size();
        }

        size_t ClusterNodeManager::getAliveNodeCount() const
        {
            size_t count = 0;
            std::shared_lock<std::shared_mutex> lock(m_mutex);
            for (const auto& [_, node] : m_nodes)
            {
                if (node.isAlive())
                {
                    count++;
                }
            }
            return count;
        }

        uint32_t ClusterNodeManager::generateNodeId()
        {
            return m_next_node_id++;
        }

        void ClusterNodeManager::cleanupTimeoutNodes(int timeout_seconds)
        {
            std::unique_lock<std::shared_mutex> lock(m_mutex);
            auto now = std::chrono::steady_clock::now();
            auto it = m_nodes.begin();
            while (it != m_nodes.end())
            {
                auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - it->second.last_seen);
                if (elapsed.count() > timeout_seconds)
                {
                    it->second.state->store(NodeState::FATL, std::memory_order_release);
                    BLUE_LOG_WARN(g_logger) << "Node " << it->first << " timeout ("
                                            << elapsed.count() << "s), marked as FAIL";
                }
                ++it;
            }
        }

    } // namespace cluster
}