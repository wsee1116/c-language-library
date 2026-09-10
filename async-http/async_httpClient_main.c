#include "async_httpClient.h"
#include <stdio.h>
#include <string.h>

void on_response(char* html, void* user_data) {
    int id = *(int*)user_data;
    printf("[任务2%d] 完成! 网页长度: %lu\n", id, strlen(html));
}

int main() {
    aio_init();

    int id1=1, id2=2, id3=3;
    
    printf("开始3个异步请求...\n");
    async_http_get("https://www.baidu.com", on_response, &id1);
    async_http_get("https://www.google.com", on_response, &id2);
    async_http_get("https://www.github.com", on_response, &id3);

    aio_run(); // 1次就跑完3个

    aio_cleanup();
    return 0;
}