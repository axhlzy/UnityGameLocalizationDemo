//
// Created by Admin on 2020/10/21.
//

#define LOG_TAG "ZZZ"
//拆分左右两个字符串的最大长度
#define SplitSize 100
#define HeaderSize 12
#define MiddleSize 200
#define EndSize 4
#define LOGD(fmt, args...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG,fmt, ##args)
#define LOGE(fmt, args...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG,fmt, ##args)

#ifndef DOBBYTEST_COMMON_H
#define DOBBYTEST_COMMON_H

void *(*old_func_dlopen)(const char *filename, int flags, const void *caller_addr) = NULL;
void *(*old_fun_dlsym)(void * /*handle*/, const char * /*symbol*/) = NULL;
void *(*old_func_set)(void *, void *, void *, void *) = NULL;
void *(*old_func_get)(void *, void *, void *, void *) = NULL;
void *(*old_func_get_methods)(void *, void *) = NULL;

void *new_func_dlopen(const char *filename, int flags, const void *caller_addr);
void *new_func_dlsym(void *handle, const char *symbol);
void *new_func_set(void *arg, void *arg1, void *arg2, void *arg3);
void *new_func_get(void *arg, void *arg1, void *arg2, void *arg3);
void *(new_func_get_methods)(void *, void *);
int readFile(JNIEnv *pEnv);
void hook_get_set();
void hook_dlopen();
void hook_get_methods();
void init_address_from_file();


#endif //DOBBYTEST_COMMON_H
