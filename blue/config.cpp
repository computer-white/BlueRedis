#include <iostream>
#include "config.h"
#include <nlohmann/json.hpp> 

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

    void Config::LoadFromJson(const std::string &json_file_path)
    {
        std::ifstream ifs(json_file_path);
        if (!ifs.is_open())
        {
            BLUE_LOG_ERROR(BLUE_LOG_MASSAGE_ROOT())
                << "LoadFromJson: cannot open file: " << json_file_path;
            return;
        }

        nlohmann::json root;
        try
        {
            ifs >> root;
        }
        catch (const std::exception &e)
        {
            BLUE_LOG_ERROR(BLUE_LOG_MASSAGE_ROOT())
                << "LoadFromJson: parse error: " << e.what();
            return;
        }

        // 递归遍历 JSON，将 key-value 转为 YAML::Node 再复用 LoadFromYAML
        std::function<void(const nlohmann::json&, const std::string&)> traverse =
            [&](const nlohmann::json &j, const std::string &prefix)
        {
            if (j.is_object())
            {
                for (auto it = j.begin(); it != j.end(); ++it)
                {
                    std::string key = prefix.empty() ? it.key() : prefix + "." + it.key();
                    traverse(it.value(), key);
                }
            }
            else if (j.is_array())
            {
                // 数组序列化为 YAML 格式的字符串，再用 LoadFromYAML 解析
                YAML::Node node;
                for (const auto &item : j)
                {
                    if (item.is_string())
                        node.push_back(item.get<std::string>());
                    else if (item.is_number_integer())
                        node.push_back(item.get<int64_t>());
                    else if (item.is_number_float())
                        node.push_back(item.get<double>());
                    else if (item.is_boolean())
                        node.push_back(item.get<bool>());
                    else
                        node.push_back(item.dump());  // 复杂类型存 JSON 字符串
                }
                // 将 YAML Node 序列化后，通过 fromString 写入配置
                auto &datas = _GetConfigVarMaps();
                auto it = datas.find(prefix);
                if (it != datas.end())
                {
                    std::stringstream ss;
                    ss << node;
                    it->second->fromString(ss.str());
                }
            }
            else
            {
                // 叶子节点：string / int / float / bool
                auto &datas = _GetConfigVarMaps();
                auto it = datas.find(prefix);
                if (it != datas.end())
                {
                    if (j.is_string())
                        it->second->fromString(j.get<std::string>());
                    else if (j.is_number_integer())
                        it->second->fromString(std::to_string(j.get<int64_t>()));
                    else if (j.is_number_float())
                        it->second->fromString(std::to_string(j.get<double>()));
                    else if (j.is_boolean())
                        it->second->fromString(j.get<bool>() ? "1" : "0");
                    else
                        it->second->fromString(j.dump());
                }
            }
        };

        traverse(root, "");
        BLUE_LOG_INFO(BLUE_LOG_MASSAGE_ROOT())
            << "LoadFromJson: " << json_file_path << " loaded successfully";
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