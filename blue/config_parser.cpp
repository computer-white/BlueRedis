#include <cctype>
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <unordered_map>
#include "blue/config_parser.h"

namespace blue
{
    namespace util
    {
        // 大小单位映射
        static const std::vector<SizeUnit> SIZE_UNITS = {
            {"T", 1024ULL * 1024 * 1024 * 1024},
            {"G", 1024ULL * 1024 * 1024},
            {"M", 1024ULL * 1024},
            {"K", 1024ULL},
            {"B", 1ULL}};

        // 时间单位映射
        static const std::vector<TimeUnit> TIME_UNITS = {
            {"h", 60ULL * 60 * 1000 * 1000},
            {"m", 60ULL * 1000 * 1000},
            {"s", 1000ULL * 1000},
            {"ms", 1000ULL},
            {"us", 1ULL}};

        std::optional<size_t> ConfigParser::ParseSize(const std::string &val)
        {
            if (val.empty())
            {
                return std::nullopt;
            }

            std::string str = val;
            // 去除前后空格
            str.erase(0, str.find_first_not_of(" \t\n\r"));
            str.erase(str.find_last_not_of(" \t\n\r") + 1);
            if (str.empty())
            {
                return std::nullopt;
            }

            // 分离数字和单位
            size_t i = 0;
            while (i < str.length() && (std::isdigit(str[i]) || str[i] == '.'))
            {
                i++;
            }

            if (i == 0)
            {
                return std::nullopt;
            }

            // 解析数字部分
            double value;
            try
            {
                value = std::stod(str.substr(0, i));
            }
            catch (...)
            {
                return std::nullopt;
            }

            if (value < 0)
            {
                return std::nullopt;
            }

            // 解析单位部分（可选）
            std::string unit_str = str.substr(i);
            std::transform(unit_str.begin(), unit_str.end(), unit_str.begin(), ::toupper);

            // 如果没有单位，默认就是字节
            if (unit_str.empty())
            {
                return static_cast<size_t>(value);
            }

            // 查找单位（支持 "MB" 和 "M" 两种写法）
            for (const auto &unit : SIZE_UNITS)
            {
                if (unit_str == unit.suffix ||
                    unit_str == unit.suffix + "B")
                {
                    return static_cast<size_t>(value * unit.multiplier);
                }
            }

            return std::nullopt;
        }

        std::string ConfigParser::FormatSize(size_t val, int precision)
        {
            if (val == 0)
            {
                return "0B";
            }
            double size = static_cast<double>(val);

            for (const auto &unit : SIZE_UNITS)
            {
                if (val >= unit.multiplier && val % unit.multiplier == 0)
                {
                    return std::to_string(val / unit.multiplier) + unit.suffix;

                }
            }

            std::ostringstream oss;
            if (val >= SIZE_UNITS[0].multiplier)
            {
                double tem = size / static_cast<double>(SIZE_UNITS[0].multiplier);
                oss << std::fixed << std::setprecision(precision) << tem << SIZE_UNITS[0].suffix;
                return oss.str();
            }
            else if (val >= SIZE_UNITS[1].multiplier)
            {
                double tem = size / static_cast<double>(SIZE_UNITS[1].multiplier);
                oss << std::fixed << std::setprecision(precision) << tem << SIZE_UNITS[1].suffix;
                return oss.str();
            }
            else if (val >= SIZE_UNITS[2].multiplier)
            {
                double tem = size / static_cast<double>(SIZE_UNITS[2].multiplier);
                oss << std::fixed << std::setprecision(precision) << tem << SIZE_UNITS[2].suffix;
                return oss.str();
            }
            else if (val >= SIZE_UNITS[3].multiplier)
            {
                double tem = size / static_cast<double>(SIZE_UNITS[3].multiplier);
                oss << std::fixed << std::setprecision(precision) << tem << SIZE_UNITS[3].suffix;
                return oss.str();
            }

            return std::to_string(val) + SIZE_UNITS[4].suffix;
        }

        std::optional<std::chrono::microseconds> ConfigParser::ParseTime(const std::string &input)
        {
            if (input.empty())
            {
                return std::nullopt;
            }

            // 去除前后空格
            std::string str = input;
            str.erase(0, str.find_first_not_of(" \t\n\r"));
            str.erase(str.find_last_not_of(" \t\n\r") + 1);

            if (str.empty())
            {
                return std::nullopt;
            }

            // 分离数字和单位
            size_t i = 0;
            while (i < str.length() && (std::isdigit(str[i]) || str[i] == '.'))
            {
                i++;
            }

            if (i == 0)
            {
                return std::nullopt;
            }

            // 解析数字部分
            double value;
            try
            {
                value = std::stod(str.substr(0, i));
            }
            catch (...)
            {
                return std::nullopt;
            }

            if (value < 0)
            {
                return std::nullopt;
            }

            // 解析单位部分
            std::string unit_str = str.substr(i);
            std::transform(unit_str.begin(), unit_str.end(), unit_str.begin(), ::tolower);

            // 如果没有单位，默认是微秒
            if (unit_str.empty())
            {
                return std::chrono::microseconds(static_cast<long long>(value));
            }

            // 查找单位
            for (const auto &unit : TIME_UNITS)
            {
                if (unit_str == unit.suffix)
                {
                    return std::chrono::microseconds(
                        static_cast<long long>(value * unit.multiplier));
                }
            }

            return std::nullopt;
        }

        std::string ConfigParser::FormatTime(std::chrono::microseconds us, int precision)
        {
            long long total_us = us.count();

            if (total_us == 0)
            {
                return "0us";
            }

            // 从大到小尝试
            for (const auto &unit : TIME_UNITS)
            {
                if (total_us >= unit.multiplier && total_us % unit.multiplier == 0)
                {
                    return std::to_string(total_us / unit.multiplier) + unit.suffix;
                }
            }

            std::ostringstream oss;
            if (total_us >= TIME_UNITS[0].multiplier)
            {
                double hours = total_us / static_cast<double>(TIME_UNITS[0].multiplier);
                oss << std::fixed << std::setprecision(precision) << hours << TIME_UNITS[0].suffix;
                return oss.str();
            }
            else if (total_us >= TIME_UNITS[1].multiplier)
            {
                double min = total_us / static_cast<double>(TIME_UNITS[1].multiplier);
                oss << std::fixed << std::setprecision(precision) << min << TIME_UNITS[1].suffix;
                return oss.str();
            }
            else if (total_us >= TIME_UNITS[2].multiplier)
            {
                double sec = total_us / static_cast<double>(TIME_UNITS[2].multiplier);
                oss << std::fixed << std::setprecision(precision) << sec << TIME_UNITS[2].suffix;
                return oss.str();
            }
            else if (total_us >= TIME_UNITS[3].multiplier)
            {
                double ms = total_us / static_cast<double>(TIME_UNITS[3].multiplier);
                oss << std::fixed << std::setprecision(precision) << ms << TIME_UNITS[3].suffix;
                return oss.str();
            }

            return std::to_string(total_us) + TIME_UNITS[4].suffix;
        }

        bool ConfigParser::ParseBool(const std::string &input)
        {
            std::string str = input;
            std::transform(str.begin(), str.end(), str.begin(), ::tolower);

            static const std::unordered_map<std::string, bool> true_map = {
                {"true", true}, {"yes", true}, {"on", true}, {"1", true}, {"enabled", true}};
            static const std::unordered_map<std::string, bool> false_map = {
                {"false", false}, {"no", false}, {"off", false}, {"0", false}, {"disabled", false}};

            auto it = true_map.find(str);
            if (it != true_map.end())
            {
                return it->second;
            }

            it = false_map.find(str);
            if (it != false_map.end())
            {
                return it->second;
            }

            return false; // 默认 false
        }

    }
}