//
// Created by Admin on 2020/10/21.
//

#define old_func_dlopen         ll1ool001ll01ol
#define old_fun_dlsym           ll1lol001ll01ol
#define old_func_set            ll1ooll01ll01ol
#define old_func_get            ll1olll1ll0001o
#define old_func_get_methods    ll1olll1ll00l1o

#define new_func_dlopen         ll1ol0l1ll00l1o
#define new_func_dlsym          ll1oll11l10ooll
#define new_func_set            ll1olliii00lo0l
#define new_func_get            ll1ol0000oololl
#define new_func_get_methods    ll1ol00100l0ol0

#define readFile                ll1oo0110l0loli
#define hook_get_set            ll1oll01ll00o0i
#define hook_dlopen             ll1oli01ll0l00l
#define hook_get_methods        ll1ol0o1lo0loil
#define init_address_from_file  lllll001llllo0l

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

#endif
