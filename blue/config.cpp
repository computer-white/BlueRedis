#include <iostream>
#include "config.h"

// 配置
namespace blue
{
    ConfigVarBase::ConfigVarBasePtr Config::LookUpBase(const std::string &name)
    {
        // 从全局m_datas里面按照名称查找
        static auto &m_datas = _GetConfigVarMaps();
        RWmutexType::ReadlockSco lock(_GetMutex());
        auto it = m_datas.find(name);
        return it == m_datas.end() ? nullptr : it->second;
    }

    // 遍历YAML节点，生成所有配置的完整路径，将YAML的树形结构扁平化成链表
    static void ListAllMember(const std::string &prefix,
                              const YAML::Node &node,
                              std::list<std::pair<std::string, const YAML::Node>> &output)
    {
        if (!prefix.empty())
        {
            if (prefix.find_first_not_of("abcdefghijklmnopqrstuvwxyz._012345678") != std::string::npos)
            {
                BLUE_LOG_ERROR(BLUE_LOG_MASSAGE_ROOT()) << "Config invalid name : " << prefix << ":" << node;
                return;
            }
        }
        output.emplace_back(prefix, node);
        if (node.IsMap())
        {
            for (auto it = node.begin(); it != node.end(); ++it)
            {
                ListAllMember(prefix.empty() ? it->first.Scalar() : prefix + "." + it->first.Scalar(), it->second, output);
            }
        }
        return;
    }

    // 从root中读取数据存到Config<T>::m_val
    void Config::LoadFromYAML(const YAML::Node &root)
    {
        std::list<std::pair<std::string, const YAML::Node>> all_nodes;
        // 对配置进行扁平化处理
        ListAllMember("", root, all_nodes);
        for (auto &[key, node] : all_nodes)
        {
            if (key.empty())
            {
                continue;
            }
            // 同一转化为小写字母
            std::transform(key.begin(), key.end(), key.begin(), [](unsigned char x)
                           { return std::tolower(x); });
            ConfigVarBase::ConfigVarBasePtr ConfVarBasePtr = LookUpBase(key);

            if (ConfVarBasePtr)
            {
                if (node.IsScalar())
                {
                    // 如果是简单类型Scalar,直接存储到配置系统
                    ConfVarBasePtr->fromString(node.Scalar());
                }
                else
                {
                    // 复杂类型写入流中，再统一存储到配置系统
                    std::stringstream ss;
                    ss << node;
                    ConfVarBasePtr->fromString(ss.str());
                }
            }
        }
    }

    void Config::Visit(std::function<void(ConfigVarBase::ConfigVarBasePtr)> cb)
    {
        RWmutexType::ReadlockSco lock(_GetMutex());
        ConfigVarMaps &m = _GetConfigVarMaps();
        for (auto &[key, val] : m)
        {
            cb(val);
        }
    }

}