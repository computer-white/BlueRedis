#ifndef BLUE_HTTP_PARSER_H
#define BLUE_HTTP_PARSER_H
#include <memory>
#include <map>
#include <string>
#include <iostream>
#include <boost/lexical_cast.hpp>
#include <sstream>
#include <functional>
#include "http.h"
#include "llhttp.h"

// http 解析(llhttp)
namespace blue
{
    namespace http
    {
        class HttpRequestParser;
        class HttpResponseParser;
        class Parser
        {
        public:
            /**
             * @brief 创建httpRequestParser智能指针
             */
            static std::shared_ptr<HttpRequestParser> CreateHttpRequestParser();

            /**
             * @brief 创建httpRequestParser智能指针
             */
            static std::shared_ptr<HttpResponseParser> CreateHttpResponseParser();

            /**
             * @brief 初始化
             * @note 若要设置回调函数,必须在init之前设置
             */
            virtual void Init() = 0;

            /**
             * @brief 获取解析过程中的错误
             * @return 错误码
             */
            virtual llhttp_errno_t getError() const = 0;

            /**
             * @brief 获取错误名称
             * @return 错误名称
             */
            virtual const char *getErrorName() const = 0;

            /**
             * @brief 获取错误原因
             * @return 错误原因
             */
            virtual const char *getErrorReason() const = 0;

            /**
             * @brief 获取出错的字节位置
             * @return 出错的字节位置
             */
            virtual const char *getErrorPos() const = 0;

            /**
             * @brief 是否设置为宽容模式
             * @param on 默认false,表示不开启
             * @note 一旦打开无法关闭
             */
            virtual void lenientMode(bool on = false) = 0;

            /**
             * @brief 暂停解析
             */
            virtual void Pause() = 0;

            /**
             * @brief 恢复解析
             */
            virtual void Resume() = 0;

            /**
             * @brief 开始解析request
             * @param data request内容
             * @param len 长度
             */
            virtual void Execute(const char *data, size_t len) = 0;

            /**
             * @brief 开始解析request
             * @param data request 内容
             */
            virtual void Execute(const std::string &data) = 0;

            /**
             * @brief 结束解析
             * @return 返回当前状态,HPE_OK表示结束
             */
            virtual llhttp_errno_t Finalize() = 0;

            /**
             * @brief 是否结束解析了
             */
            virtual bool isFinalized() const = 0;

            virtual void reset() = 0;

            /**
             * @brief 对gzip进行压缩
             * @param data 需要压缩的数据
             * @return 返回压缩后的数据
             */
            std::string compress(const std::string &data) const;

            /**
             * @brief 对gzip进行解压
             * @param data 需要解压的数据
             * @return 返回解压后的数据
             */
            std::string decompress(const std::string &data) const;

            /**
             * @brief 检查encoding是否含有tem
             * @param encoding 一系列解压和压缩的方式字符串
             * @param tem 检查是否包含tem方式
             */
            bool checkEncoding(const std::string &encoding, const char *tem);

            /**
             * @brief 禁止拷贝和移动
             */
            Parser() = default;
            ~Parser() = default;
            Parser(const Parser &lhs) = delete;
            Parser &operator=(const Parser &lhs) = delete;
            Parser(Parser &&rhs) = delete;
            Parser &operator=(Parser &&rhs) = delete;
        };

        class HttpRequestParser : public Parser
        {
        public:
            using HttpRequestParserPtr = std::shared_ptr<HttpRequestParser>;

        public:
            HttpRequestParser();
            ~HttpRequestParser() = default;

            /**
             * @brief 初始化
             * @note 若要设置回调函数,必须在init之前设置
             */
            void Init() override;

            /**
             * @brief 获取解析过程中的错误
             * @return 错误码
             */
            llhttp_errno_t getError() const override { return (llhttp_errno_t)m_parser.error; }

            /**
             * @brief 获取错误名称
             * @return 错误名称
             */
            const char *getErrorName() const override { return llhttp_errno_name((llhttp_errno_t)m_parser.error); }

            /**
             * @brief 获取错误原因
             * @return 错误原因
             */
            const char *getErrorReason() const override { return m_parser.reason; }

            /**
             * @brief 获取出错的字节位置
             * @return 出错的字节位置
             */
            const char *getErrorPos() const override { return m_parser.error_pos; }

            /**
             * @brief 收到url片段触发回调
             * @param cb 回调函数
             * @note 如无必要回调函数中请返回0
             */
            void on_Url(std::function<int()> cb);

            /**
             * @brief url 解析完成触发
             * @param cb 回调函数
             * @note 如无必要回调函数中请返回0
             */
            void on_UrlComplate(std::function<int(const std::string &url)> cb);

            /**
             * @brief 收到Method片段触发回调
             * @param cb 回调函数
             * @note 如无必要回调函数中请返回0
             */
            void on_Method(std::function<int()> cb);

            /**
             * @brief Method 解析完成触发
             * @param cb 回调函数
             * @note 如无必要回调函数中请返回0
             */
            void on_MethodComplate(std::function<int(const std::string &method)> cb);

            /**
             * @brief 收到Version片段触发回调
             * @param cb 回调函数
             * @note 如无必要回调函数中请返回0
             */
            void on_Version(std::function<int()> cb);

            /**
             * @brief Version片段解析完成触发回调
             * @param cb 回调函数
             * @note 如无必要回调函数中请返回0
             */
            void on_VersionComplate(std::function<int(const std::string &version)> cb);
            /**
             * @brief 收到头部字段名触发回调
             * @param cb 回调函数
             * @note 如无必要回调函数中请返回0
             */
            void on_HeaderField(std::function<int()> cb);

            /**
             * @brief 收到头部字段值触发回调
             * @param cb 回调函数
             * @note 如无必要回调函数中请返回0
             */
            void on_HeaderValue(std::function<int()> cb);

            /**
             * @brief 所有头部处理完成触发回调
             * @param cb 回调函数
             * @note 如无必要回调函数中请返回0
             */
            void on_HeaderCompalte(std::function<int()> cb);

            /**
             * @brief 收到body字段触发回调
             * @param cb 回调函数
             * @note 如无必要回调函数中请返回0
             */
            void on_Body(std::function<int(const std::string &data)> cb);

            /**
             * @brief 整条 HTTP 消息解析完成后触发,意味着解析完成
             * @param cb 回调函数
             * @note 如无必要回调函数中请返回0
             */
            void on_MessageComplate(std::function<int(std::shared_ptr<HttpRequest>)> cb);

            /**
             * @brief 整个 HTTP 消息开始触发回调
             * @param cb 回调函数
             * @note 如无必要回调函数中请返回0
             */
            void on_MessageBegin(std::function<int()> cb);

            /**
             * @brief 是否设置为宽容模式
             * @param on 默认false,表示不开启
             * @note 一旦打开无法关闭
             */
            void lenientMode(bool on = false) override;

            /**
             * @brief 暂停解析
             */
            void Pause() override;

            /**
             * @brief 恢复解析
             */
            void Resume() override;

            /**
             * @brief 开始解析request
             * @param data request内容
             * @param len 长度
             */
            void Execute(const char *data, size_t len) override;

            /**
             * @brief 开始解析request
             * @param data request 内容
             */
            void Execute(const std::string &data) override;

            /**
             * @brief 调用finalize表示需要解析的数据全部发完了
             * @return 返回当前状态,HPE_OK表示消息完整，否则表示不完整
             */
            llhttp_errno_t Finalize() override;

            /**
             * @brief 数据是否完整
             */
            bool isFinalized() const override;

            /**
             * @brief 重置
             */
            void reset();

            /**
             * @brief 获取解析后的内容
             */
            std::shared_ptr<HttpRequest> getData() const noexcept { return m_data; }

        public:
            static uint64_t GetHttpRequestBufferSize();
            static uint64_t GetHttpRequestMaxBodySize();
            static void SetRequestBufferSize(size_t size);

        private:
            /**
             * @brief 解析cookie
             */
            void _parseCookie();

            /**
             * @brief 解析query,将query中的param键值对拿出来并做解码存入m_data
             */
            void _parseQuery();

            /* 一些桥接函数(桥接用户和llhttp) */

            /**
             * @brief 对url中的query部分解码
             * @param param query部分的key或val
             */
            std::string _urlDecode(const std::string &param);

            /**
             * @brief 对头部字段key进行格式化
             */
            void _headerFieldFormat(std::string &headerfiled);

            /**
             * @brief llhttp 的 URL 数据回调，每次收到 URL 片段时触发,供llhttp使用并在解析完后调用用户设置的url回调函数
             * @param parser 解析器实例指针
             * @param at 当前收到的 URL 数据片段起始位置
             * @param len 当前片段的长度
             * @return 0 继续解析 -1 触发 HPE_USER 错误
             * @note 可能分多次解析,所以在内部利用缓存变量拼接答案
             */
            static int _on_Url(llhttp_t *parser, const char *at, size_t len);

            /**
             * @brief url 解析完成触发,供llhttp使用并在解析完后调用用户设置的url回调函数
             * @param parser 解析器实例指针
             * @return 0 继续解析 -1 触发 HPE_USER 错误
             */
            static int _on_Url_complate(llhttp_t *parser);

            /**
             * @brief llhttp 的 Method 数据回调，每次收到 Method 片段时触发,供llhttp使用并在解析完后调用用户设置的url回调函数
             * @param parser 解析器实例指针
             * @param at 当前收到的 Method 数据片段起始位置
             * @param len 当前片段的长度
             * @return 0 继续解析 -1 触发 HPE_USER 错误
             * @note 可能分多次解析,所以在内部利用缓存变量拼接答案
             */
            static int _on_Method(llhttp_t *parser, const char *at, size_t len);

            /**
             * @brief llhttp 的 Method 数据回调，Method 片段解析完成时触发,供llhttp使用并在解析完后调用用户设置的url回调函数
             * @param parser 解析器实例指针
             * @return 0 继续解析 -1 触发 HPE_USER 错误
             */
            static int _on_Method_complate(llhttp_t *parser);

            /**
             * @brief llhttp 的 Version 数据回调，每次收到 Version 片段时触发,供llhttp使用并在解析完后调用用户设置的url回调函数
             * @param parser 解析器实例指针
             * @param at 当前收到的 Version 数据片段起始位置
             * @param len 当前片段的长度
             * @return 0 继续解析 -1 触发 HPE_USER 错误
             * @note 可能分多次解析,所以在内部利用缓存变量拼接答案
             */
            static int _on_Version(llhttp_t *parser, const char *at, size_t len);

            /**
             * @brief llhttp 的 Version 数据回调，每次收到 Version 片段时触发,供llhttp使用并在解析完后调用用户设置的url回调函数
             * @param parser 解析器实例指针
             * @return 0 继续解析 -1 触发 HPE_USER 错误
             */
            static int _on_Version_complate(llhttp_t *parser);

            /**
             * @brief 新消息开始解析时触发，通知用户回调
             * @param p 解析器实例指针
             * @return 0 继续，-1 报错
             */
            static int _on_message_begin(llhttp_t *p);

            /**
             * @brief 整条 HTTP 消息解析完成后触发，设置 body，将 m_data 交给用户回调，重置解析器(如果支持长连接)
             * @param parser 解析器实例指针
             * @return 0 继续，-1 报错
             */
            static int _on_Message_complate(llhttp_t *parser);

            /**
             * @brief 收到头部字段名片段时触发，遇到新 field 时先将上一个 header 存入 m_data
             * @param p 解析器实例指针
             * @param at 当前收到的字段名数据
             * @param len 当前片段的长度
             * @return 0 继续，-1 报错（字段名过长）
             * @note 可能分多次解析,所以在内部利用缓存变量拼接答案
             */
            static int _on_header_field(llhttp_t *p, const char *at, size_t len);

            /**
             * @brief 收到头部字段值片段时触发，累加到 m_curr_headervalue
             * @param p 解析器实例指针
             * @param at 当前收到的字段值数据
             * @param len 当前片段的长度
             * @return 0 继续，-1 报错
             * @note 可能分多次解析,所以在内部利用缓存变量拼接答案
             */
            static int _on_header_value(llhttp_t *p, const char *at, size_t len);

            /**
             * @brief 所有头部解析完成后触发，存储 method/version/close，解析 Cookie
             * @param parser 解析器实例指针
             * @return 0 继续，1 跳过 body，-1 报错
             */
            static int _on_Header_complate(llhttp_t *parser);

            /**
             * @brief 收到消息体片段时触发，累加到 m_curr_body
             * @param p 解析器实例指针
             * @param at 当前收到的 body 数据
             * @param len 当前片段的长度
             * @return 0 继续，-1 报错
             * @note 可能分多次解析,所以在内部利用缓存变量拼接答案
             */
            static int _on_body(llhttp_t *p, const char *at, size_t len);

        private:
            llhttp_t m_parser;
            llhttp_settings_t m_settings;
            llhttp_errno_t m_errno;
            std::string m_curr_headerfield;
            std::string m_curr_headervalue;
            std::string m_curr_method;
            std::string m_curr_version;
            std::string m_curr_url;
            std::string m_curr_body;
            std::shared_ptr<HttpRequest> m_data;

            /* 回调函数族 */
            std::function<int()> m_urlcb;
            std::function<int()> m_methodcb;
            std::function<int()> m_versioncb;
            std::function<int()> m_headerFieldcb;
            std::function<int()> m_headerValuecb;
            std::function<int(const std::string &)> m_Bodycb;
            std::function<int()> m_headerCmpcb;
            std::function<int()> m_messageBegcb;
            std::function<int(const std::string &)> m_urlCmpcb;
            std::function<int(const std::string &)> m_versionCmpcb;
            std::function<int(const std::string &)> m_methodCmpcb;
            std::function<int(std::shared_ptr<HttpRequest>)> m_messageCmpcb;
        };

        class HttpResponseParser : public Parser
        {
        public:
            using HttpResponsetParserPtr = std::shared_ptr<HttpResponseParser>;

        public:
            HttpResponseParser();
            ~HttpResponseParser() = default;

            /**
             * @brief 初始化
             * @note 若要设置回调函数,必须在init之前设置
             */
            void Init() override;

            /**
             * @brief 获取解析过程中的错误
             * @return 错误码
             */
            llhttp_errno_t getError() const override { return (llhttp_errno_t)m_parser.error; }

            /**
             * @brief 获取错误名称
             * @return 错误名称
             */
            const char *getErrorName() const override { return llhttp_errno_name((llhttp_errno_t)m_parser.error); }

            /**
             * @brief 获取错误原因
             * @return 错误原因
             */
            const char *getErrorReason() const override { return m_parser.reason; }

            /**
             * @brief 获取出错的字节位置
             * @return 出错的字节位置
             */
            const char *getErrorPos() const override { return m_parser.error_pos; }

            /**
             * @brief 处理响应码的时候触发回调
             * @param cb 回调函数
             * @note 如无必要回调函数中请返回0
             */
            void on_Status(std::function<int()> cb);

            /**
             * @brief 响应码处理完成触发回调
             * @param cb 回调函数
             * @note 如无必要回调函数中请返回0
             */
            void on_StatusComplate(std::function<int(const std::string &)> cb);

            /**
             * @brief 收到Version片段触发回调
             * @param cb 回调函数
             * @note 如无必要回调函数中请返回0
             */
            void on_Version(std::function<int()> cb);

            /**
             * @brief Version片段解析完成触发回调
             * @param cb 回调函数
             * @note 如无必要回调函数中请返回0
             */
            void on_VersionComplate(std::function<int(const std::string &version)> cb);

            /**
             * @brief 收到头部字段名触发回调
             * @param cb 回调函数
             * @note 如无必要回调函数中请返回0
             */
            void on_HeaderField(std::function<int()> cb);

            /**
             * @brief 收到头部字段值触发回调
             * @param cb 回调函数
             * @note 如无必要回调函数中请返回0
             */
            void on_HeaderValue(std::function<int()> cb);

            /**
             * @brief 所有头部处理完成触发回调
             * @param cb 回调函数
             * @note 如无必要回调函数中请返回0
             */
            void on_HeaderCompalte(std::function<int()> cb);

            /**
             * @brief 收到body字段触发回调
             * @param cb 回调函数
             * @note 如无必要回调函数中请返回0
             */
            void on_Body(std::function<int(const std::string &data)> cb);

            /**
             * @brief 整个HTTP消息结束触发回调,意味着解析完成
             * @param cb 回调函数
             * @note 如无必要回调函数中请返回0
             */
            void on_MessageComplate(std::function<int(std::shared_ptr<HttpResponse>)> cb);

            /**
             * @brief 整个HTTP消息开始触发回调
             * @param cb 回调函数
             * @note 如无必要回调函数中请返回0
             */
            void on_MessageBegin(std::function<int()> cb);

            /**
             * @brief 是否设置为宽容模式
             * @param on 默认false,表示不开启
             * @note 一旦打开无法关闭
             */
            void lenientMode(bool on = false) override;

            /**
             * @brief 暂停解析
             */
            void Pause() override;

            /**
             * @brief 恢复解析
             */
            void Resume() override;

            /**
             * @brief 开始解析request
             * @param data request内容
             * @param len 长度
             */
            void Execute(const char *data, size_t len) override;

            /**
             * @brief 开始解析request
             * @param data request 内容
             */
            void Execute(const std::string &data) override;

            /**
             * @brief 调用finalize表示需要解析的数据全部发完了
             * @return 返回当前状态,HPE_OK表示消息完整，否则表示不完整
             */
            llhttp_errno_t Finalize() override;

            /**
             * @brief 数据是否完整
             */
            bool isFinalized() const override;

            /**
             * @brief 重置
             */
            void reset();

            /**
             * @brief 获取解析后的内容
             */
            std::shared_ptr<HttpResponse> getData() const noexcept { return m_data; }

        public:
            static uint64_t GetHttpResponseBufferSize();
            static uint64_t GetHttpResponseMaxBodySize();
            static void SetResponseBufferSize(size_t size);

        private:
            /**
             * @brief 对头部字段key进行格式化
             */
            void _headerFieldFormat(std::string &headerfiled);

            /**
             * @brief 接收到响应码的时候触发回调
             * @param parser 解析器实例指针
             * @param at 响应码字段
             * @param len 响应码长度
             * @return 0 继续解析 -1 触发 HPE_USER 错误
             * @note 可能分多次解析,所以在内部利用缓存变量拼接答案
             */
            static int _on_Status(llhttp_t *parser, const char *at, size_t len);

            /**
             * @brief 响应码处理完成的时候触发回调
             * @param parser 解析器实例指针
             * @note 将响应码写入m_data
             */
            static int _on_Status_complate(llhttp_t *parser);

            /**
             * @brief llhttp 的 Version 数据回调，每次收到 Version 片段时触发,供llhttp使用并在解析完后调用用户设置的url回调函数
             * @param parser 解析器实例指针
             * @param at 当前收到的 Version 数据片段起始位置
             * @param len 当前片段的长度
             * @return 0 继续解析 -1 触发 HPE_USER 错误
             * @note 可能分多次解析,所以在内部利用缓存变量拼接答案
             */
            static int _on_Version(llhttp_t *parser, const char *at, size_t len);

            /**
             * @brief llhttp 的 Version 数据回调，每次收到 Version 片段时触发,供llhttp使用并在解析完后调用用户设置的url回调函数
             * @param parser 解析器实例指针
             * @return 0 继续解析 -1 触发 HPE_USER 错误
             */
            static int _on_Version_complate(llhttp_t *parser);

            /**
             * @brief 收到头部字段名片段时触发，遇到新 field 时先将上一个 header 存入 m_data
             * @param p 解析器实例指针
             * @param at 当前收到的字段名数据
             * @param len 当前片段的长度
             * @return 0 继续，-1 报错（字段名过长）
             * @note 可能分多次解析,所以在内部利用缓存变量拼接答案
             */
            static int _on_header_field(llhttp_t *parser, const char *at, size_t len);

            /**
             * @brief 收到头部字段值片段时触发，累加到 m_curr_headervalue
             * @param p 解析器实例指针
             * @param at 当前收到的字段值数据
             * @param len 当前片段的长度
             * @return 0 继续，-1 报错
             * @note 可能分多次解析,所以在内部利用缓存变量拼接答案
             */
            static int _on_header_value(llhttp_t *parser, const char *at, size_t len);

            /**
             * @brief 所有头部解析完成后触发，存储 method/version/close，解析 Cookie
             * @param parser 解析器实例指针
             * @return 0 继续，1 跳过 body，-1 报错
             */
            static int _on_Header_complate(llhttp_t *parser);

            /**
             * @brief 新消息开始解析时触发，通知用户回调
             * @param p 解析器实例指针
             * @return 0 继续，-1 报错
             */
            static int _on_message_begin(llhttp_t *parser);

            /**
             * @brief 整条 HTTP 消息解析完成后触发，设置 body，将 m_data 交给用户回调，重置解析器(如果支持长连接)
             * @param parser 解析器实例指针
             * @return 0 继续，-1 报错
             */
            static int _on_Message_complate(llhttp_t *parser);

            /**
             * @brief 收到消息体片段时触发，累加到 m_curr_body
             * @param p 解析器实例指针
             * @param at 当前收到的 body 数据
             * @param len 当前片段的长度
             * @return 0 继续，-1 报错
             * @note 可能分多次解析,所以在内部利用缓存变量拼接答案
             */
            static int _on_body(llhttp_t *parser, const char *at, size_t len);

        private:
            llhttp_t m_parser;
            llhttp_settings_t m_settings;
            llhttp_errno_t m_errno;
            std::string m_curr_headerfield;
            std::string m_curr_headervalue;
            std::string m_curr_reason;
            std::string m_curr_body;
            std::string m_curr_version;
            std::shared_ptr<HttpResponse> m_data;

            /* 回调函数族 */
            std::function<int()> m_Statuscb;
            std::function<int()> m_versioncb;
            std::function<int()> m_headerFieldcb;
            std::function<int()> m_headerValuecb;
            std::function<int(const std::string &)> m_Bodycb;
            std::function<int()> m_messageBegcb;
            std::function<int()> m_headerCmpcb;
            std::function<int(const std::string &)> m_StatusCmpcb;
            std::function<int(const std::string &)> m_versionCmpcb;
            std::function<int(std::shared_ptr<HttpResponse>)> m_messageCmpcb;
        };
    } // http
} // blue

#endif