#ifndef ASYNC_HTTPCLIENT_H
#define ASYNC_HTTPCLIENT_H

#include <curl/curl.h>

typedef void (*HttpCallback)(char* response, void* user_data);

void aio_init(); // 初始化curl_multi
void aio_cleanup(); // 清理

// 异步GET 就像 await session.get()
void async_http_get(char* url, HttpCallback callback, void* user_data);

// 跑事件循环 就像 asyncio.run()
void aio_run();

#endif