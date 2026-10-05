/*
 * BlueRedis - High Performance Redis Server based on C++20 Coroutine
 * Copyright (C) 2026 blue
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
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
            if (prefix.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz._012345678") != std::string::npos)
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

    void Config::LoadFromYAML(const std::string &yaml_file_path)
    {
        YAML::Node root;
        try
        {
            root = YAML::LoadFile(yaml_file_path);
        }
        catch (const YAML::Exception &e)
        {
            BLUE_LOG_ERROR(BLUE_LOG_MASSAGE_ROOT())
                << "LoadFromYAML: cannot load " << yaml_file_path
                << " : " << e.what();
            throw;
        }
        LoadFromYAML(root);
    }

    inline YAML::Node jsonToYaml(const nlohmann::json &j)
    {
        YAML::Node node;
        if (j.is_object())
        {
            for (auto it = j.begin(); it != j.end(); ++it)
            {
                node[it.key()] = jsonToYaml(it.value());
            }
        }
        else if (j.is_array())
        {
            for (const auto &item : j)
            {
                node.push_back(jsonToYaml(item));
            }
        }
        else if (j.is_string())
        {
            node = j.get<std::string>();
        }
        else if (j.is_boolean())
        {
            node = j.get<bool>();
        }
        else if (j.is_number_integer())
        {
            node = j.get<int64_t>();
        }
        else if (j.is_number_float())
        {
            node = j.get<double>();
        }
        else if (j.is_null())
        {
            node = YAML::Node(YAML::NodeType::Null);
        }
        else
        {
            node = j.dump();
        }
        return node;
    }

    void Config::LoadFromJson(const std::string &json_file_path)
    {
        std::ifstream ifs(json_file_path);
        if (!ifs.is_open())
        {
            std::string msg = "LoadFromJson: cannot open " + json_file_path;
            BLUE_LOG_ERROR(BLUE_LOG_MASSAGE_ROOT()) << msg;
            throw std::invalid_argument(msg);
        }

        nlohmann::json root;
        try
        {
            ifs >> root;
        }
        catch (const std::exception &e)
        {
            std::string msg = "LoadFromJson: parse error in " + json_file_path
                        + " : " + e.what();
            BLUE_LOG_ERROR(BLUE_LOG_MASSAGE_ROOT()) << msg;
            throw std::invalid_argument(msg);
        }
        auto node = blue::jsonToYaml(root);
        LoadFromYAML(node);
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