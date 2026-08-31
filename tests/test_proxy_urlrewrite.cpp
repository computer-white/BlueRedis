#include "proxy/url_rewriter.h"
#include <iostream>

int main() {
    blue::proxy::UrlRewriter rewriter("https://www.baidu.com", "/blue");
    
    std::cout << rewriter.rewrite("/news") << std::endl;
    // 输出: /blue/https://www.baidu.com/news
    
    std::cout << rewriter.rewrite("https://www.baidu.com/img/logo.png") << std::endl;
    // 输出: /blue/https://www.baidu.com/img/logo.png
    
    std::cout << rewriter.rewrite("//cdn.baidu.com/static/js.js") << std::endl;
    // 输出: /blue/https://cdn.baidu.com/static/js.js
    
    return 0;
}