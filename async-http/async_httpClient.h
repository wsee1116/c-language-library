#ifndef ASYNC_HTTPCLIENT_H
#define ASYNC_HTTPCLIENT_H

#include <curl/curl.h>
#include <time.h>

typedef void (*HttpCallback)(char* response, void* user_data);
typedef void (*HttpErrorCallback)(int error_code, const char* error_msg, void* user_data);

// === Request Options ===
typedef struct {
    int timeout;           // milliseconds
    int max_retries;       // auto-retry on failure
    int retry_delay;       // milliseconds between retries
    HttpErrorCallback err_cb;  // error callback
    char* custom_headers;  // "Header1: value\r\nHeader2: value"
} AsyncRequestOptions;

// === Initialization ===
void aio_init();        // 初始化curl_multi和事件循环
void aio_cleanup();     // 清理所有资源

// === Async GET (like await session.get()) ===
void async_http_get(char* url, HttpCallback callback, void* user_data);

// === Async POST (like await session.post()) ===
void async_http_post(char* url, char* data, HttpCallback callback, void* user_data);

// === Advanced: With Options ===
void async_http_get_opt(char* url, AsyncRequestOptions* opts, HttpCallback callback, void* user_data);
void async_http_post_opt(char* url, char* data, AsyncRequestOptions* opts, HttpCallback callback, void* user_data);

// === Event Loop (like asyncio.run()) ===
void aio_run();         // 跑事件循环直到所有请求完成
int aio_pending();      // 返回还有多少请求未完成

// === Batch Operations ===
void aio_run_timeout(int timeout_ms);  // 跑事件循环，最多等待timeout_ms

#endif