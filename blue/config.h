#ifndef BLUE_CONFIG_H
#define BLUE_CONFIG_H
#include <memory>
#include <sstream>
#include <string>
#include <exception>
#include <boost/lexical_cast.hpp>
#include <yaml-cpp/yaml.h>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include "log.h"
#include "mthread.h"

// 配置文件
namespace blue
{
    // 配置基类
    class ConfigVarBase
    {
    public:
        using ConfigVarBasePtr = std::shared_ptr<ConfigVarBase>;
        /**
         * @brief 配置基类的构造函数
         * @param name 配置名称
         * @param description 配置的描述
         * @return
         */
        ConfigVarBase(const std::string &name, const std::string &description = "") : m_name(name), m_description(description)
        {
            std::transform(m_name.begin(), m_name.end(), m_name.begin(), [](unsigned char x)
                           { return std::tolower(x); });
        }

        /**
         * @brief 配置基类的虚析构函数
         * @return
         */
        virtual ~ConfigVarBase() = default;

        /**
         * @brief 获取配置的名称
         * @return 配置的名称
         */
        const std::string &getName() const { return m_name; }

        /**
         * @brief 获取配置的描述
         * @return 配置的描述
         */
        const std::string &getDescription() const { return m_description; }

        /**
         * @brief 纯虚函数,支持序列化(类型T转string)
         * @return
         */
        virtual std::string toString() = 0;

        /**
         * @brief 纯虚函数,支持反序列化(string转到类型T)
         * @return
         */
        virtual bool fromString(const std::string &val) = 0;

        /**
         * @brief 如果有报错,支持得到类型名称
         * @return 错误类型名称
         */
        virtual std::string getTypeName() const = 0;

    protected:
        std::string m_name;        // 配置名称
        std::string m_description; // 关于配置的描述
    };

    // 支持基础类型到string的转换
    template <typename F, typename T>
    class LexicalCast
    {
    public:
        T operator()(const F &val)
        {
            return boost::lexical_cast<T>(val);
        }
    };

    // 一系列特化的序列化和反序列化类模板
    // 特化string -> vector<T>
    template <typename T>
    class LexicalCast<std::string, std::vector<T>>
    {
    public:
        std::vector<T> operator()(const std::string &val)
        {
            YAML::Node node = YAML::Load(val);
            typename std::vector<T> vec; // typename告诉编译器std::vector<T>是一个类型
            std::stringstream ss;
            for (unsigned i = 0; i < node.size(); i++)
            {
                ss.str("");
                ss.clear();
                ss << node[i];
                vec.push_back(LexicalCast<std::string, T>()(ss.str()));
            }
            return vec; // RVO(返回值优化),若返回的类型与局部对象类型相同,且返回的就是局部对象,则c++11会优化
        }
    };

    // 特化vector<T> -> string
    template <typename T>
    class LexicalCast<std::vector<T>, std::string>
    {
    public:
        std::string operator()(const std::vector<T> &val)
        {
            YAML::Node node;
            for (auto &x : val)
            {
                node.push_back(YAML::Load(LexicalCast<T, std::string>()(x)));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    // 特化list<T> -> string
    template <typename T>
    class LexicalCast<std::list<T>, std::string>
    {
    public:
        std::string operator()(const std::list<T> &val)
        {
            YAML::Node node;
            for (auto &x : val)
            {
                node.push_back(YAML::Load(LexicalCast<T, std::string>()(x)));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    // 特化 string -> list<T>
    template <typename T>
    class LexicalCast<std::string, std::list<T>>
    {
    public:
        std::list<T> operator()(const std::string &val)
        {
            YAML::Node node = YAML::Load(val);
            typename std::list<T> lis; // typename告诉编译器std::list<T>是一个类型
            std::stringstream ss;
            for (unsigned i = 0; i < node.size(); i++)
            {
                ss.str("");
                ss.clear();
                ss << node[i];
                lis.push_back(LexicalCast<std::string, T>()(ss.str()));
            }
            return lis; // RVO(返回值优化),若返回的类型与局部对象类型相同,且返回的就是局部对象,则c++11会优化
        }
    };

    // 特化set<T> -> string
    template <typename T>
    class LexicalCast<std::set<T>, std::string>
    {
    public:
        std::string operator()(const std::set<T> &val)
        {
            YAML::Node node;
            for (auto &x : val)
            {
                node.push_back(YAML::Load(LexicalCast<T, std::string>()(x)));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    // 特化 string -> set<T>
    template <typename T>
    class LexicalCast<std::string, std::set<T>>
    {
    public:
        std::set<T> operator()(const std::string &val)
        {
            YAML::Node node = YAML::Load(val);
            typename std::set<T> set_; // typename告诉编译器std::set<T>是一个类型
            std::stringstream ss;
            for (unsigned i = 0; i < node.size(); i++)
            {
                ss.str("");
                ss.clear();
                ss << node[i];
                set_.insert(LexicalCast<std::string, T>()(ss.str()));
            }
            return set_; // RVO(返回值优化),若返回的类型与局部对象类型相同,且返回的就是局部对象,则c++11会优化
        }
    };

    // 特化unordered_set<T> -> string
    template <typename T>
    class LexicalCast<std::unordered_set<T>, std::string>
    {
    public:
        std::string operator()(const std::unordered_set<T> &val)
        {
            YAML::Node node;
            for (auto &x : val)
            {
                node.push_back(YAML::Load(LexicalCast<T, std::string>()(x)));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    // 特化 string -> unordered_set<T>
    template <typename T>
    class LexicalCast<std::string, std::unordered_set<T>>
    {
    public:
        std::unordered_set<T> operator()(const std::string &val)
        {
            YAML::Node node = YAML::Load(val);
            typename std::unordered_set<T> unorset_; // typename告诉编译器std::set<T>是一个类型
            std::stringstream ss;
            for (unsigned i = 0; i < node.size(); i++)
            {
                ss.str("");
                ss.clear();
                ss << node[i];
                unorset_.insert(LexicalCast<std::string, T>()(ss.str()));
            }
            return unorset_; // RVO(返回值优化),若返回的类型与局部对象类型相同,且返回的就是局部对象,则c++11会优化
        }
    };

    // 特化map<std::string,T> -> string
    template <typename T>
    class LexicalCast<std::map<std::string, T>, std::string>
    {
    public:
        std::string operator()(const std::map<std::string, T> &val)
        {
            YAML::Node node;
            for (auto &[map_key, map_values] : val)
            {
                node[map_key] = YAML::Load(LexicalCast<T, std::string>()(map_values));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    // 特化 string -> map<std::string,T>
    template <typename T>
    class LexicalCast<std::string, std::map<std::string, T>>
    {
    public:
        std::map<std::string, T> operator()(const std::string &val)
        {
            YAML::Node node = YAML::Load(val);
            typename std::map<std::string, T> map_; // typename告诉编译器std::map<std::string,T>是一个类型
            std::stringstream ss;
            for (auto it = node.begin(); it != node.end(); ++it)
            {
                ss.str(""); // 每次清空流
                ss.clear();
                ss << it->second;
                // map_.insert(std::make_pair(it->first.Scalar(),
                // LexicalCast<std::string,T>() (ss.str())));
                map_.emplace(it->first.Scalar(), LexicalCast<std::string, T>()(ss.str()));
            }
            return map_; // RVO(返回值优化),若返回的类型与局部对象类型相同,且返回的就是局部对象,则c++11会优化
        }
    };

    // 特化unordered_map<std::string,T> -> string
    template <typename T>
    class LexicalCast<std::unordered_map<std::string, T>, std::string>
    {
    public:
        std::string operator()(const std::unordered_map<std::string, T> &val)
        {
            YAML::Node node;
            for (auto &[map_key, map_values] : val)
            {
                node[map_key] = YAML::Load(LexicalCast<T, std::string>()(map_values));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    // 特化 string -> unordered_map<std::string,T>
    template <typename T>
    class LexicalCast<std::string, std::unordered_map<std::string, T>>
    {
    public:
        std::unordered_map<std::string, T> operator()(const std::string &val)
        {
            YAML::Node node = YAML::Load(val);
            typename std::unordered_map<std::string, T> map_; // typename告诉编译器std::map<std::string,T>是一个类型
            std::stringstream ss;
            for (auto it = node.begin(); it != node.end(); ++it)
            {
                ss.str(""); // 每次清空流
                ss.clear();
                ss << it->second;
                // map_.insert(std::make_pair(it->first.Scalar(),
                // LexicalCast<std::string,T>() (ss.str())));
                map_.emplace(it->first.Scalar(), LexicalCast<std::string, T>()(ss.str()));
            }
            return map_; // RVO(返回值优化),若返回的类型与局部对象类型相同,且返回的就是局部对象,则c++11会优化
        }
    };

    // 派生类模板,支持T类型的序列化和反序列化
    // FromStr T opertaor() (const std::string&)
    // ToString std::string opertaor(const T&)
    template <typename T,
              typename FromStr = LexicalCast<std::string, T>,
              typename ToString = LexicalCast<T, std::string>>
    class ConfigVar : public ConfigVarBase
    {
    public:
        using RWmutexType = blue::MRWmutex;
        using ConfigVarPtr = std::shared_ptr<ConfigVar<T>>;                           // 智能指针
        using on_change_cb = std::function<void(const T &old_val, const T &new_val)>; // 回调函数

        /**
         * @brief 配置派生类构造函数
         * @param name 配置名称
         * @param default_val 默认的T类型的值
         * @param description 对配置的描述
         * @return
         */
        ConfigVar(const std::string &name, const T &default_val,
                  const std::string &description) : ConfigVarBase(name, description), m_val(default_val)
        {
        }

        /**
         * @brief 配置派生类析构函数
         * @return
         */
        ~ConfigVar() override = default;

        /**
         * @brief 配置类的主要函数,将T类型转换为string类型(序列化)
         * @return 返回序列化后的string
         */
        virtual std::string toString() override
        {
            try
            {
                // return boost::lexical_cast<std::string>(m_val);
                RWmutexType::ReadlockSco lock(m_mutex);
                return ToString()(m_val);
            }
            catch (std::exception &e)
            {
                BLUE_LOG_ERROR(BLUE_LOG_MASSAGE_ROOT()) << "ConfigVar::toString excption"
                                                        << e.what() << "convert: " << typeid(m_val).name() << "to string";
            }
            return std::string();
        }

        /**
         * @brief 配置类的主要函数,将string类型转换为T类型(反序列化)
         * @return 成功返回true
         */
        virtual bool fromString(const std::string &val) override
        {
            try
            {
                // m_val = boost::lexical_cast<T>(val);
                setValue(FromStr()(val));
                return true;
            }
            catch (std::exception &e)
            {
                BLUE_LOG_ERROR(BLUE_LOG_MASSAGE_ROOT()) << "ConfigVar::toString excption "
                                                        << e.what() << " convert: string to " << typeid(m_val).name();
            }
            return false;
        }

        /**
         * @brief 获取T类型的值
         * @return 值
         */
        const T getValue() const
        {
            RWmutexType::ReadlockSco lock(m_mutex);
            return m_val;
        }

        /**
         * @brief 设置T类型的值
         * @return
         * @note 支持回调,当原本在旧值上设立回调函数时,并且新值与旧值不同会触发回调函数
         */
        void setValue(const T &new_val)
        {
            {
                RWmutexType::ReadlockSco lock(m_mutex);
                if (new_val == m_val)
                {
                    return; // 值未变，直接返回
                }
            } // 读锁释放
            T old_val;
            {
                RWmutexType::WritelockSco lock(m_mutex);
                if (new_val == m_val)
                {
                    return;
                }
                old_val = m_val;
                m_val = new_val;
            } // 写锁释放

            // 调用func回调函数(监听器)来通知配置的值已经被更改
            for (auto &[key, func] : m_mapfunc)
            {
                func(old_val, m_val);
            }
        }

        /**
         * @brief 获取错误类型的名称
         * @return 错误类型的名称
         */
        virtual std::string getTypeName() const override { return typeid(T).name(); } // 返回T的类型

        /**
         * @brief 设置监听器,配置被更改后需要做出一些操作,一般是更新删除修改
         * @param cb 回调监听函数
         * @return 监听器的id
         */
        uint64_t addListener(on_change_cb cb)
        {
            static uint64_t s_fun_id = 0; // 静态局部变量的初始化是线程安全的
            RWmutexType::WritelockSco lock(m_mutex);
            ++s_fun_id;
            m_mapfunc[s_fun_id] = cb;
            return s_fun_id;
        }

        /**
         * @brief 删除监听器
         * @param key 监听器id
         * @return
         */
        bool delListener(uint64_t key)
        {
            auto it = m_mapfunc.find(key);
            RWmutexType::WritelockSco lock(m_mutex);
            if (it != m_mapfunc.end())
            {
                m_mapfunc.erase(it);
                return true;
            }
            return false;
        }

        /**
         * @brief 获取监听器
         * @param key 监听器id
         * @return 监听器函数
         */
        on_change_cb getListener(uint64_t key)
        {
            RWmutexType::ReadlockSco lock(m_mutex);
            auto it = m_mapfunc.find(key);
            return it == m_mapfunc.end() ? nullptr : it->second;
        }

        /**
         * @brief 清空监听器
         * @return
         */
        void clearListener()
        {
            RWmutexType::WritelockSco lock(m_mutex);
            m_mapfunc.clear();
        }

    private:
        mutable RWmutexType m_mutex;
        // 为从.yml文件中加载出来的内容的string被转化到T类型，
        // 支持基础类型,set,map,unordered_set,
        // unordered_map,list以及自定义类型(需要自己实现转换)
        T m_val;
        // 变更回调函数组,一般是如果m_val被修改后,我需要调用函数做一些处理
        std::map<uint64_t, on_change_cb> m_mapfunc;
    };

    class Config
    {
    public:
        using ConfigVarMaps = std::map<std::string, ConfigVarBase::ConfigVarBasePtr>;
        using RWmutexType = blue::MRWmutex;

        /**
         * @brief 只查找配置名称
         * @param name 配置名称
         * @return 返回一个配置模板派生类智能指针
         */
        template <class T>
        static typename ConfigVar<T>::ConfigVarPtr Lookup(const std::string &name)
        {
            static auto &m_datas = _GetConfigVarMaps();
            {
                RWmutexType::ReadlockSco lock(_GetMutex());
                auto it = m_datas.find(name);
                if (it != m_datas.end())
                {
                    // name存在,转化为ConfigVar<T>
                    auto config_var = std::dynamic_pointer_cast<ConfigVar<T>>(it->second);
                    if (config_var)
                    {
                        return config_var;
                    }
                    // 转化失败说明，类型不匹配，返回空
                }
                return nullptr;
            }
        }

        /**
         * @brief 查找配置名称,没有就创建
         * @param name 配置名称
         * @param default_val 模板类型T的默认值
         * @param description 对配置的说明
         * @return 返回一个配置模板派生类智能指针
         */
        template <class T>
        static typename ConfigVar<T>::ConfigVarPtr Lookup(const std::string &name,
                                                          const T &default_value,
                                                          const std::string &description = "")
        {
            // 验证名称合法性
            if (name.empty() || name.find_first_not_of("abcdefghijklmnopqrstuvwxyz._0123456789") != std::string::npos)
            {
                BLUE_LOG_ERROR(BLUE_LOG_MASSAGE_ROOT())
                    << "Lookup name invalid: [" << name << "]";
                throw std::invalid_argument(name);
            }
            static auto &m_datas = _GetConfigVarMaps();
            {
                RWmutexType::ReadlockSco lock(_GetMutex());
                auto it = m_datas.find(name);
                if (it != m_datas.end())
                {
                    auto config_var = std::dynamic_pointer_cast<ConfigVar<T>>(it->second);
                    if (config_var)
                    {
                        BLUE_LOG_INFO(BLUE_LOG_MASSAGE_ROOT()) << "Lookup name is ["
                                                               << name << "] exists";
                        return config_var;
                    }
                    // 如果转化失败，说明类型不匹配，返回nullptr
                    BLUE_LOG_ERROR(BLUE_LOG_MASSAGE_ROOT())
                        << "Lookup name is " << name << " exists but type mismatch " << "this type is : " << typeid(T).name() << " "
                        << "the real type is " << it->second->getTypeName() << " "
                        << "the string is " << it->second->toString();
                    return nullptr;
                }
            }

            // 创建新的配置项
            typename ConfigVar<T>::ConfigVarPtr v =
                std::make_shared<ConfigVar<T>>(name, default_value, description);
            {
                auto it = m_datas.find(name);
                RWmutexType::WritelockSco lock(_GetMutex());
                // 再次判断
                // auto it = m_datas.find(name);
                if (it != m_datas.end())
                {
                    auto config_var = std::dynamic_pointer_cast<ConfigVar<T>>(it->second);
                    if (config_var)
                    {
                        BLUE_LOG_INFO(BLUE_LOG_MASSAGE_ROOT()) << "Lookup name is ["
                                                               << name << "] exists";
                        return config_var;
                    }
                    // 如果转化失败，说明类型不匹配，返回nullptr
                    BLUE_LOG_ERROR(BLUE_LOG_MASSAGE_ROOT())
                        << "Lookup name is " << name << " exists but type mismatch " << "this type is : " << typeid(T).name() << " "
                        << "the real type is " << it->second->getTypeName() << " "
                        << "the string is " << it->second->toString();
                    return nullptr;
                }
                m_datas[name] = v;
                return v;
            }
        }

        /**
         * @brief  加载配置,支持对从.yml中加载出来的root进行
         * @param root yaml节点
         * @return
         */
        static void LoadFromYAML(const YAML::Node &root);

        /**
         * @brief 按照名称查找基类指针
         * @param name 配置的名称
         * @return 配置基类指针
         */
        static ConfigVarBase::ConfigVarBasePtr LookUpBase(const std::string &name);

        // 拿出m_datas数据来检查
        /**
         * @brief 取出存储在配置名称和配置信息的maps中的配置信息的内容做检查
         * @param cb 检查配置信息的函数
         * @return
         */
        static void Visit(std::function<void(ConfigVarBase::ConfigVarBasePtr)> cb);

    private:
        /**
         * @brief 获取静态map数据,存放着配置的名称以及配置基类指针
         * @return m_datas
         * @note 必须返回引用或指针,否则相当于每次调用修改的是副本导致错误(他妈的，让我改了一个多小时)
         */
        static ConfigVarMaps &_GetConfigVarMaps()
        {
            static ConfigVarMaps m_datas;
            return m_datas;
        }

        /**
         * @brief 获取静态的互斥变量
         * @return RWmutexType
         */
        static RWmutexType &_GetMutex()
        {
            static RWmutexType s_mutex;
            return s_mutex;
        }
    };
} // namespace blue

#endif // __BLUE_CONFIG_H__