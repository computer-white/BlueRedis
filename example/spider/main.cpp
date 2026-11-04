#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <cstring>
#include "crawler.h"
#include "fetcher.h"
#include "src/core/io_manager.h"
#include "src/util/log.h"
#include "example/spider/fetcher.h"
#include "example/spider/crawler.h"

namespace
{
    struct Options
    {
        std::vector<std::string> seeds;
        int concurrency = 4;
        double rate = 50.0;
        int maxDepth = 3;
        int threads = 4;
        std::string output = "/tmp/spider_out";
        std::string userAgent = "BlueSpider/1.0";
        uint64_t timeoutMs = 5000;
    };

    void printUsage(const char* prog)
    {
        std::cout
            << "Usage: " << prog << " [options]\n"
            << "\nOptions:\n"
            << "  --seeds <url>[,<url>...]   起始 URL，逗号分隔，可多次指定（必填）\n"
            << "  --concurrency <n>          worker 并发数（默认 4）\n"
            << "  --rate <n>                 每秒请求上限（默认 50）\n"
            << "  --max-depth <n>            最大抓取深度（默认 3）\n"
            << "  --threads <n>              IOManager 线程数（默认 4）\n"
            << "  --output <dir>             落盘目录（默认 ./spider_out）\n"
            << "  --user-agent <str>         User-Agent（默认 BlueSpider/1.0）\n"
            << "  --timeout <ms>             单请求超时 ms（默认 5000）\n"
            << "  -h, --help                 显示帮助\n"
            << "\nExample:\n"
            << "  " << prog << " --seeds http://127.0.0.1:8082/ --concurrency 32 --rate 500 --max-depth 10\n";
    }

    // 逗号分隔字符串拆成 vector
    std::vector<std::string> splitComma(const std::string& s)
    {
        std::vector<std::string> out;
        size_t start = 0;
        while (start <= s.size())
        {
            size_t pos = s.find(',', start);
            std::string item = (pos == std::string::npos)
                                   ? s.substr(start)
                                   : s.substr(start, pos - start);
            if (!item.empty()) out.push_back(item);
            if (pos == std::string::npos) break;
            start = pos + 1;
        }
        return out;
    }

    bool parseArgs(int argc, char** argv, Options& opt)
    {
        for (int i = 1; i < argc; ++i)
        {
            std::string arg = argv[i];
            auto needValue = [&](const char* name) -> const char* {
                if (i + 1 >= argc)
                {
                    std::cerr << "error: " << name << " requires a value\n";
                    return nullptr;
                }
                return argv[++i];
            };

            if (arg == "-h" || arg == "--help")
            {
                printUsage(argv[0]);
                std::exit(0);
            }
            else if (arg == "--seeds")
            {
                auto* v = needValue("--seeds");
                if (!v) return false;
                auto urls = splitComma(v);
                opt.seeds.insert(opt.seeds.end(), urls.begin(), urls.end());
            }
            else if (arg == "--concurrency")
            {
                auto* v = needValue("--concurrency");
                if (!v) return false;
                opt.concurrency = std::atoi(v);
                if (opt.concurrency <= 0)
                {
                    std::cerr << "error: --concurrency must be > 0\n";
                    return false;
                }
            }
            else if (arg == "--rate")
            {
                auto* v = needValue("--rate");
                if (!v) return false;
                opt.rate = std::atof(v);
                if (opt.rate <= 0)
                {
                    std::cerr << "error: --rate must be > 0\n";
                    return false;
                }
            }
            else if (arg == "--max-depth")
            {
                auto* v = needValue("--max-depth");
                if (!v) return false;
                opt.maxDepth = std::atoi(v);
                if (opt.maxDepth < 0)
                {
                    std::cerr << "error: --max-depth must be >= 0\n";
                    return false;
                }
            }
            else if (arg == "--threads")
            {
                auto* v = needValue("--threads");
                if (!v) return false;
                opt.threads = std::atoi(v);
                if (opt.threads <= 0)
                {
                    std::cerr << "error: --threads must be > 0\n";
                    return false;
                }
            }
            else if (arg == "--output")
            {
                auto* v = needValue("--output");
                if (!v) return false;
                opt.output = v;
            }
            else if (arg == "--user-agent")
            {
                auto* v = needValue("--user-agent");
                if (!v) return false;
                opt.userAgent = v;
            }
            else if (arg == "--timeout")
            {
                auto* v = needValue("--timeout");
                if (!v) return false;
                opt.timeoutMs = std::strtoull(v, nullptr, 10);
            }
            else
            {
                std::cerr << "error: unknown option " << arg << "\n";
                printUsage(argv[0]);
                return false;
            }
        }

        if (opt.seeds.empty())
        {
            std::cerr << "error: --seeds is required\n";
            printUsage(argv[0]);
            return false;
        }
        return true;
    }
}

int main(int argc, char** argv)
{
    Options opt;
    if (!parseArgs(argc, argv, opt))
    {
        return 1;
    }

    std::cout << "seeds=" << opt.seeds.size()
              << " concurrency=" << opt.concurrency
              << " rate=" << opt.rate
              << " max_depth=" << opt.maxDepth
              << " threads=" << opt.threads
              << " output=" << opt.output << "\n";

    auto iom = std::make_shared<blue::IOManager>(opt.threads);
    auto fetcher = std::make_shared<blue::spider::Fetcher>(opt.userAgent);
    auto crawler = std::make_shared<blue::spider::Crawler>(
        fetcher,
        opt.concurrency,
        opt.rate,
        opt.maxDepth,
        opt.output);

    iom->schedule(crawler->crawl(opt.seeds));
    iom->wait_all();
    return 0;
}