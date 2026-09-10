# Async HTTP Client - Like Python's `asyncio`

Make **non-blocking concurrent HTTP requests** in C with a simple, Pythonic API.

## 🚀 Quick Start

```c
#include "async_httpClient.h"

void on_response(char* html, void* user_data) {
    printf("Got response: %lu bytes\n", strlen(html));
}

int main() {
    aio_init();
    
    // Start 3 concurrent requests (non-blocking!)
    async_http_get("https://www.github.com", on_response, NULL);
    async_http_get("https://www.google.com", on_response, NULL);
    async_http_get("https://www.baidu.com", on_response, NULL);
    
    // Run event loop until all complete
    aio_run();
    
    aio_cleanup();
    return 0;
}
```

## 📋 API Reference

### Initialization

```c
void aio_init();      // Initialize async runtime
void aio_cleanup();   // Clean up resources
```

### Simple Requests

```c
// Async GET (like: await session.get(url))
void async_http_get(char* url, HttpCallback callback, void* user_data);

// Async POST (like: await session.post(url, data))
void async_http_post(char* url, char* data, HttpCallback callback, void* user_data);
```

### Advanced Requests with Options

```c
// GET with options (timeout, retry, custom headers, error handling)
void async_http_get_opt(char* url, AsyncRequestOptions* opts, 
                        HttpCallback callback, void* user_data);

// POST with options
void async_http_post_opt(char* url, char* data, AsyncRequestOptions* opts,
                         HttpCallback callback, void* user_data);
```

### Event Loop

```c
// Run until all requests complete
void aio_run();

// Run with timeout (milliseconds)
void aio_run_timeout(int timeout_ms);

// Check pending requests
int aio_pending();
```

## 🔧 AsyncRequestOptions

```c
typedef struct {
    int timeout;                    // milliseconds (0 = no timeout)
    int max_retries;                // auto-retry count (0 = no retry)
    int retry_delay;                // milliseconds between retries
    HttpErrorCallback err_cb;       // error callback function
    char* custom_headers;           // "Header: value\r\nHeader2: value"
} AsyncRequestOptions;
```

## 📝 Examples

### Example 1: Simple Concurrent GET

```c
void on_response(char* html, void* user_data) {
    int id = *(int*)user_data;
    printf("[Task %d] Length: %lu\n", id, strlen(html));
}

int main() {
    aio_init();
    
    int ids[3] = {1, 2, 3};
    async_http_get("https://www.github.com", on_response, &ids[0]);
    async_http_get("https://www.google.com", on_response, &ids[1]);
    async_http_get("https://www.baidu.com", on_response, &ids[2]);
    
    aio_run();
    aio_cleanup();
    return 0;
}
```

### Example 2: POST with Error Handling & Retry

```c
void on_error(int code, const char* msg, void* user_data) {
    printf("Error: %d - %s\n", code, msg);
}

void on_response(char* html, void* user_data) {
    printf("POST successful: %lu bytes\n", strlen(html));
}

int main() {
    aio_init();
    
    AsyncRequestOptions opts = {
        .timeout = 5000,        // 5 second timeout
        .max_retries = 3,       // Retry 3 times
        .retry_delay = 1000,    // 1 second between retries
        .err_cb = on_error,
        .custom_headers = "Content-Type: application/json"
    };
    
    async_http_post_opt("https://httpbin.org/post", 
                        "{\"key\":\"value\"}", 
                        &opts, on_response, NULL);
    
    aio_run_timeout(15000);  // Max 15 seconds
    aio_cleanup();
    return 0;
}
```

### Example 3: Batch Operations

```c
void on_response(char* html, void* user_data) {
    int id = *(int*)user_data;
    printf("[Task %d] Done! Pending: %d\n", id, aio_pending());
}

int main() {
    aio_init();
    
    char* urls[] = {
        "https://www.github.com",
        "https://www.google.com",
        "https://www.baidu.com",
        // ... more URLs
    };
    
    printf("Starting %d requests...\n", 10);
    for(int i = 0; i < 10; i++) {
        int* id = malloc(sizeof(int));
        *id = i + 1;
        async_http_get(urls[i], on_response, id);
    }
    
    printf("Pending: %d\n", aio_pending());
    aio_run();
    
    aio_cleanup();
    return 0;
}
```

### Example 4: Mixed GET & POST

```c
int main() {
    aio_init();
    
    // Both GET and POST with different options
    AsyncRequestOptions get_opts = {.timeout = 3000};
    AsyncRequestOptions post_opts = {.timeout = 5000, .max_retries = 2};
    
    async_http_get_opt("https://www.github.com", &get_opts, 
                       on_response, (void*)"GET-1");
    async_http_post_opt("https://httpbin.org/post", "{}", &post_opts,
                        on_response, (void*)"POST-1");
    async_http_get_opt("https://www.google.com", &get_opts,
                       on_response, (void*)"GET-2");
    
    aio_run();
    aio_cleanup();
    return 0;
}
```

## 🔄 Python vs C Comparison

| Python | C |
|--------|---|
| `async def` | `async_http_get()` |
| `await session.get(url)` | Non-blocking request |
| `asyncio.run()` | `aio_run()` |
| Error handling | `err_cb` callback |
| Timeout | `timeout` option |
| Retry logic | `max_retries` option |

## 📦 Compilation

```bash
# Compile examples
gcc -o async_client async_httpClient.c async_httpClient_main.c -lcurl

# Run specific example
./async_client 1    # Simple GET
./async_client 2    # POST with retry
./async_client 3    # Batch operations
./async_client 4    # Mixed requests

# Run all examples
./async_client
```

## 🎯 Key Features

✅ **Non-blocking** - Multiple requests run concurrently  
✅ **Simple API** - Just like Python's `asyncio`  
✅ **Automatic Retry** - Built-in retry logic with configurable delays  
✅ **Error Handling** - Custom error callbacks  
✅ **Timeouts** - Per-request and global timeouts  
✅ **Custom Headers** - Add any HTTP headers  
✅ **Batch Operations** - Handle 10+ concurrent requests  
✅ **Progress Tracking** - Check pending requests with `aio_pending()`

## 🚀 Perfect For

- Web scraping multiple sites concurrently
- API client libraries
- Microservices communication
- Real-time data fetching
- IoT applications
- Learning async patterns in C

## 📚 Advanced Usage

### Custom User Data

```c
typedef struct {
    int id;
    char* name;
} Context;

void on_response(char* html, void* user_data) {
    Context* ctx = (Context*)user_data;
    printf("[%d] %s: %lu bytes\n", ctx->id, ctx->name, strlen(html));
    free(ctx);
}

int main() {
    aio_init();
    
    Context* ctx = malloc(sizeof(Context));
    ctx->id = 1;
    ctx->name = "Task1";
    
    async_http_get("https://www.github.com", on_response, ctx);
    aio_run();
    aio_cleanup();
    return 0;
}
```

### Error Recovery

```c
void on_error(int code, const char* msg, void* user_data) {
    printf("Request failed: %s\n", msg);
    // Will auto-retry up to max_retries
}

int main() {
    aio_init();
    
    AsyncRequestOptions opts = {
        .max_retries = 5,
        .retry_delay = 500,
        .err_cb = on_error
    };
    
    // This will retry up to 5 times before calling on_error
    async_http_get_opt("https://api.example.com/unstable", &opts, 
                       on_response, NULL);
    
    aio_run();
    aio_cleanup();
    return 0;
}
```

## ⚠️ Notes

- Requires libcurl (`-lcurl` when compiling)
- All URLs must be valid HTTP/HTTPS URLs
- Callbacks are called in the main thread (safe for UI updates)
- Use `aio_pending()` to check if all requests completed

## 📄 License

MIT - Free to use for any purpose!

## 👨‍💻 Author

Created by wsee1116 - An easy way to do async HTTP in C!
