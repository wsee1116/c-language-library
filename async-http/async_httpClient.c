#include "async_httpClient.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

CURLM *multi_handle;
int still_running;
int pending_requests = 0;

// Enhanced RequestData with retry support
typedef struct {
    char* url;
    char* post_data;
    char* response;
    size_t size;
    HttpCallback cb;
    HttpErrorCallback err_cb;
    void* user_data;
    int retries;
    int max_retries;
    int retry_delay;
    int timeout;
    char* custom_headers;
    CURL* handle;
} RequestData;

size_t write_cb(char *ptr, size_t size, size_t nmemb, RequestData* data) {
    size_t realsize = size * nmemb;
    data->response = realloc(data->response, data->size + realsize + 1);
    memcpy(&(data->response[data->size]), ptr, realsize);
    data->size += realsize;
    data->response[data->size] = 0;
    return realsize;
}

void aio_init() {
    curl_global_init(CURL_GLOBAL_ALL);
    multi_handle = curl_multi_init();
    still_running = 0;
    pending_requests = 0;
}

// Internal function to setup curl handle
void _setup_curl_handle(CURL* eh, RequestData* data, const char* method) {
    curl_easy_setopt(eh, CURLOPT_URL, data->url);
    curl_easy_setopt(eh, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(eh, CURLOPT_WRITEDATA, data);
    curl_easy_setopt(eh, CURLOPT_PRIVATE, data);
    
    // Timeout
    if(data->timeout > 0) {
        curl_easy_setopt(eh, CURLOPT_TIMEOUT_MS, data->timeout);
    }
    
    // Custom headers
    if(data->custom_headers) {
        struct curl_slist* headers = NULL;
        headers = curl_slist_append(headers, data->custom_headers);
        curl_easy_setopt(eh, CURLOPT_HTTPHEADER, headers);
    }
    
    // POST method
    if(strcmp(method, "POST") == 0 && data->post_data) {
        curl_easy_setopt(eh, CURLOPT_POST, 1L);
        curl_easy_setopt(eh, CURLOPT_POSTFIELDS, data->post_data);
    }
}

void async_http_get(char* url, HttpCallback callback, void* user_data) {
    AsyncRequestOptions opts = {0, 0, 0, NULL, NULL};
    async_http_get_opt(url, &opts, callback, user_data);
}

void async_http_post(char* url, char* data, HttpCallback callback, void* user_data) {
    AsyncRequestOptions opts = {0, 0, 0, NULL, NULL};
    async_http_post_opt(url, data, &opts, callback, user_data);
}

void async_http_get_opt(char* url, AsyncRequestOptions* opts, HttpCallback callback, void* user_data) {
    CURL *eh = curl_easy_init();
    RequestData* data = malloc(sizeof(RequestData));
    
    data->url = url;
    data->post_data = NULL;
    data->response = malloc(1);
    data->size = 0;
    data->cb = callback;
    data->err_cb = opts ? opts->err_cb : NULL;
    data->user_data = user_data;
    data->retries = 0;
    data->max_retries = opts ? opts->max_retries : 0;
    data->retry_delay = opts ? opts->retry_delay : 0;
    data->timeout = opts ? opts->timeout : 0;
    data->custom_headers = opts && opts->custom_headers ? strdup(opts->custom_headers) : NULL;
    data->handle = eh;
    
    _setup_curl_handle(eh, data, "GET");
    curl_multi_add_handle(multi_handle, eh);
    pending_requests++;
}

void async_http_post_opt(char* url, char* post_data, AsyncRequestOptions* opts, HttpCallback callback, void* user_data) {
    CURL *eh = curl_easy_init();
    RequestData* data = malloc(sizeof(RequestData));
    
    data->url = url;
    data->post_data = post_data ? strdup(post_data) : NULL;
    data->response = malloc(1);
    data->size = 0;
    data->cb = callback;
    data->err_cb = opts ? opts->err_cb : NULL;
    data->user_data = user_data;
    data->retries = 0;
    data->max_retries = opts ? opts->max_retries : 0;
    data->retry_delay = opts ? opts->retry_delay : 0;
    data->timeout = opts ? opts->timeout : 0;
    data->custom_headers = opts && opts->custom_headers ? strdup(opts->custom_headers) : NULL;
    data->handle = eh;
    
    _setup_curl_handle(eh, data, "POST");
    curl_multi_add_handle(multi_handle, eh);
    pending_requests++;
}

int aio_pending() {
    return pending_requests;
}

void aio_run() {
    aio_run_timeout(0);  // 0 = infinite timeout
}

void aio_run_timeout(int timeout_ms) {
    time_t start_time = time(NULL);
    
    do {
        curl_multi_perform(multi_handle, &still_running);
        if(still_running) {
            int wait_timeout = timeout_ms > 0 ? timeout_ms : 1000;
            curl_multi_wait(multi_handle, NULL, 0, wait_timeout, NULL);
        }
        
        // Check for completed requests
        CURLMsg *msg;
        int msgs_left;
        while ((msg = curl_multi_info_read(multi_handle, &msgs_left))) {
            if (msg->msg == CURLMSG_DONE) {
                CURL *e = msg->easy_handle;
                RequestData* data = NULL;
                curl_easy_getinfo(e, CURLINFO_PRIVATE, &data);
                
                if(msg->data.result == CURLE_OK) {
                    // Success
                    data->cb(data->response, data->user_data);
                } else {
                    // Error - try retry
                    if(data->retries < data->max_retries) {
                        data->retries++;
                        if(data->retry_delay > 0) {
                            usleep(data->retry_delay * 1000);
                        }
                        // Retry: reset and re-add handle
                        curl_multi_remove_handle(multi_handle, e);
                        free(data->response);
                        data->response = malloc(1);
                        data->size = 0;
                        
                        if(data->post_data) {
                            _setup_curl_handle(e, data, "POST");
                        } else {
                            _setup_curl_handle(e, data, "GET");
                        }
                        curl_multi_add_handle(multi_handle, e);
                        still_running++;
                        continue;
                    } else {
                        // Final error
                        if(data->err_cb) {
                            data->err_cb(msg->data.result, curl_easy_strerror(msg->data.result), data->user_data);
                        }
                    }
                }
                
                curl_multi_remove_handle(multi_handle, e);
                curl_easy_cleanup(e);
                free(data->response);
                if(data->custom_headers) free(data->custom_headers);
                if(data->post_data) free(data->post_data);
                free(data);
                pending_requests--;
            }
        }
        
        // Check timeout
        if(timeout_ms > 0) {
            time_t elapsed = time(NULL) - start_time;
            if(elapsed * 1000 >= timeout_ms) break;
        }
    } while(still_running);
}

void aio_cleanup() {
    curl_multi_cleanup(multi_handle);
    curl_global_cleanup();
}