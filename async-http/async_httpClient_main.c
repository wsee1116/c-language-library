#include "async_httpClient.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// ===== Example 1: Simple GET requests =====
void on_response_simple(char* html, void* user_data) {
    int id = *(int*)user_data;
    printf("[Task %d] Completed! Page length: %lu bytes\n", id, strlen(html));
}

void example_simple_get() {
    printf("=== Example 1: Simple Concurrent GET ===\n");
    aio_init();
    
    int id1=1, id2=2, id3=3;
    
    printf("Starting 3 async requests...\n");
    async_http_get("https://www.baidu.com", on_response_simple, &id1);
    async_http_get("https://www.google.com", on_response_simple, &id2);
    async_http_get("https://www.github.com", on_response_simple, &id3);
    
    printf("Running event loop...\n");
    aio_run();
    
    aio_cleanup();
    printf("\n");
}

// ===== Example 2: POST requests with error handling =====
void on_error(int error_code, const char* error_msg, void* user_data) {
    int id = *(int*)user_data;
    printf("[Task %d] ERROR! Code: %d, Message: %s\n", id, error_code, error_msg);
}

void on_response_post(char* response, void* user_data) {
    int id = *(int*)user_data;
    printf("[Task %d] POST completed! Response length: %lu bytes\n", id, strlen(response));
}

void example_post_with_retry() {
    printf("=== Example 2: POST with Retry & Error Handling ===\n");
    aio_init();
    
    int id = 1;
    
    AsyncRequestOptions opts = {
        .timeout = 5000,        // 5 second timeout
        .max_retries = 3,       // Retry 3 times on failure
        .retry_delay = 1000,    // 1 second between retries
        .err_cb = on_error,
        .custom_headers = "Content-Type: application/json"
    };
    
    char* json_data = "{\"key\": \"value\"}";
    async_http_post_opt("https://httpbin.org/post", json_data, &opts, on_response_post, &id);
    
    printf("Running event loop with timeout...\n");
    aio_run_timeout(15000);  // Max 15 seconds
    
    aio_cleanup();
    printf("\n");
}

// ===== Example 3: Batch operations with progress tracking =====
void on_response_batch(char* html, void* user_data) {
    int id = *(int*)user_data;
    printf("[Task %d] Done! Length: %lu, Pending: %d\n", id, strlen(html), aio_pending() - 1);
}

void example_batch_operations() {
    printf("=== Example 3: Batch Operations (10 concurrent) ===\n");
    aio_init();
    
    int ids[10];
    char urls[10][50] = {
        "https://www.github.com",
        "https://www.google.com",
        "https://www.baidu.com",
        "https://www.wikipedia.org",
        "https://www.stackoverflow.com",
        "https://www.amazon.com",
        "https://www.facebook.com",
        "https://www.youtube.com",
        "https://www.twitter.com",
        "https://www.linkedin.com"
    };
    
    printf("Starting 10 concurrent requests...\n");
    for(int i = 0; i < 10; i++) {
        ids[i] = i + 1;
        async_http_get(urls[i], on_response_batch, &ids[i]);
    }
    
    printf("Pending requests: %d\n", aio_pending());
    aio_run();
    printf("All requests completed!\n\n");
    
    aio_cleanup();
}

// ===== Example 4: Mixed GET and POST with different timeouts =====
void on_response_mixed(char* response, void* user_data) {
    char* task = (char*)user_data;
    printf("[%s] Completed! Response length: %lu bytes\n", task, strlen(response));
}

void example_mixed_requests() {
    printf("=== Example 4: Mixed GET/POST with Different Timeouts ===\n");
    aio_init();
    
    // GET with 3 second timeout
    AsyncRequestOptions get_opts = {
        .timeout = 3000,
        .max_retries = 1,
        .err_cb = on_error,
        .custom_headers = NULL
    };
    
    // POST with 5 second timeout and 2 retries
    AsyncRequestOptions post_opts = {
        .timeout = 5000,
        .max_retries = 2,
        .retry_delay = 500,
        .err_cb = on_error,
        .custom_headers = "Content-Type: application/json"
    };
    
    printf("Starting mixed requests...\n");
    async_http_get_opt("https://www.github.com", &get_opts, on_response_mixed, (void*)"GET-1");
    async_http_post_opt("https://httpbin.org/post", "{\"test\":1}", &post_opts, on_response_mixed, (void*)"POST-1");
    async_http_get_opt("https://www.google.com", &get_opts, on_response_mixed, (void*)"GET-2");
    async_http_post_opt("https://httpbin.org/post", "{\"test\":2}", &post_opts, on_response_mixed, (void*)"POST-2");
    
    printf("Pending: %d, Running event loop...\n", aio_pending());
    aio_run();
    
    aio_cleanup();
    printf("\n");
}

// ===== Main: Run all examples =====
int main(int argc, char* argv[]) {
    printf("\n╔═══════════════════════════════════════════════════════╗\n");
    printf("║   C Async HTTP Client - Complete Examples           ║\n");
    printf("║   (Like Python asyncio - Easy & Powerful)           ║\n");
    printf("╚═══════════════════════════════════════════════════════╝\n\n");
    
    if(argc > 1) {
        int example = atoi(argv[1]);
        switch(example) {
            case 1:
                example_simple_get();
                break;
            case 2:
                example_post_with_retry();
                break;
            case 3:
                example_batch_operations();
                break;
            case 4:
                example_mixed_requests();
                break;
            default:
                printf("Usage: %s [1|2|3|4]\n\n", argv[0]);
                printf("  1 = Simple concurrent GET requests (3 sites)\n");
                printf("  2 = POST with retry & error handling\n");
                printf("  3 = Batch operations (10 concurrent requests)\n");
                printf("  4 = Mixed GET/POST with different timeouts\n");
                printf("\nExample: %s 1\n\n", argv[0]);
                return 0;
        }
    } else {
        printf("Running all examples (use './async_client [1-4]' for specific example)\n\n");
        example_simple_get();
        example_batch_operations();
    }
    
    return 0;
}