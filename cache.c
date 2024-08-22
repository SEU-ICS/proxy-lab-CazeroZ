
#include "csapp.h"
#include "cache.h"

static cache_t cache;
static sem_t mutex, w;
static int readcnt, timestamp;
void init_cache() {
    timestamp = 0;
    readcnt = 0;
    cache.using_cache_num = 0;
    sem_init(&mutex, 0, 1);
    sem_init(&w, 0, 1);
}
int query_cache(rio_t* rio_p, string url) {
    P(&mutex);
    readcnt++;
    if (readcnt == 1) {
        P(&w);
    }
    V(&mutex);
    int hit_flag = 0;
    for (int i = 0; i < MAX_CACHE_NUM;i++) {
        if (!strcmp(cache.cache_files[i].url, url)) {
            P(&mutex);
            cache.cache_files[i].timestamp = timestamp++;
            V(&mutex);
            rio_writen(rio_p->rio_fd, cache.cache_files[i].content, cache.cache_files[i].content_size);
            hit_flag = 1;
            break;
        }
    }
    P(&mutex);
    readcnt--;
    if (readcnt == 0) {
        V(&w);
    }
    V(&mutex);
    if (hit_flag) {
        return 1;
    }
    return 0;
}

int add_cache(string url, char* content, int content_size) {
    P(&w);
    if (cache.using_cache_num == (MAX_CACHE_NUM - 1)) {
        int oldest_index;
        int oldest_timestamp = timestamp;
        for (int i = 0;i < MAX_CACHE_NUM;i++) {
            if (cache.cache_files[i].timestamp < oldest_timestamp) {
                oldest_timestamp = cache.cache_files[i].timestamp;
                oldest_index = i;
            }
        }
        strcpy(cache.cache_files[oldest_index].url, url);
        memcpy(cache.cache_files[oldest_index].content, content, content_size);
        cache.cache_files[oldest_index].content_size = content_size;
        P(&mutex);
        cache.cache_files[oldest_index].timestamp = timestamp++;
        V(&mutex);
    }
    else {
        strcpy(cache.cache_files[cache.using_cache_num].url, url);
        memcpy(cache.cache_files[cache.using_cache_num].content, content, content_size);
        cache.cache_files[cache.using_cache_num].content_size = content_size;
        P(&mutex);
        cache.cache_files[cache.using_cache_num].timestamp = timestamp++;
        V(&mutex);
        cache.using_cache_num++;
    }
    V(&w);
    return 0;
}