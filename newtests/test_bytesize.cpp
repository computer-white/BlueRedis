#include <iostream>
#include "blue/cluster/cluster_message.h"
#include "blue/cluster/cluster_node.h"
using namespace blue::cluster;
int main()
{
    // 测试消息
    auto ping = MessageFactory::createPing(1, 2);
    auto serialized = ping.serialize();
    auto deserialized = Message::deserialize(serialized);
    
    std::cout << "Message type: " << (int)deserialized.header.type << std::endl;
    std::cout << "Sender: " << deserialized.header.sender_id << std::endl;
    std::cout << "Receiver: " << deserialized.header.receiver_id << std::endl;
    std::cout << "Valid: " << deserialized.isValid() << std::endl;
    
    // 测试节点管理
    ClusterNodeManager manager;
    ClusterNode node1(1, "127.0.0.1", 6379, 16379);
    manager.addNode(node1);
    
    auto nodes = manager.getAllNodes();
    std::cout << "Total nodes: " << nodes.size() << std::endl;
    
    return 0;
}