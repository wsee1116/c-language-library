#include "async_httpClient.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

CURLM *multi_handle;
int still_running;

typedef struct {
    char* url;
    char* response;
    size_t size;
    HttpCallback cb;
    void* user_data;
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
}

void async_http_get(char* url, HttpCallback callback, void* user_data) {
    CURL *eh = curl_easy_init();
    RequestData* data = malloc(sizeof(RequestData));
    data->url = url; data->response = malloc(1); data->size = 0;
    data->cb = callback; data->user_data = user_data;

    curl_easy_setopt(eh, CURLOPT_URL, url);
    curl_easy_setopt(eh, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(eh, CURLOPT_WRITEDATA, data);
    curl_multi_add_handle(multi_handle, eh);
}

void aio_run() {
    do {
        curl_multi_perform(multi_handle, &still_running);
        if(still_running) curl_multi_wait(multi_handle, NULL, 0, 1000, NULL);
        
        // 检查哪个完成了
        CURLMsg *msg;
        int msgs_left;
        while ((msg = curl_multi_info_read(multi_handle, &msgs_left))) {
            if (msg->msg == CURLMSG_DONE) {
                CURL *e = msg->easy_handle;
                RequestData* data;
                curl_easy_getinfo(e, CURLINFO_PRIVATE, &data);
                data->cb(data->response, data->user_data); // 回调
                curl_multi_remove_handle(multi_handle, e);
                curl_easy_cleanup(e);
                free(data->response);
                free(data);
            }
        }
    } while(still_running);
}

void aio_cleanup() {
    curl_multi_cleanup(multi_handle);
    curl_global_cleanup();
}