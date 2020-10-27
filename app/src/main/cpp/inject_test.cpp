#include <stdio.h>
#include <jni.h>

#include "inlineHook/include/inlineHook.h"
#include "Tools/tools.h"
#include <android/log.h>

#include <unistd.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <sys/time.h>
#include <cstring>

typedef int (*m_dlopen)(const char* __filename, int __flag);
typedef int (*m_dlsym)(void* __handle, const char* __symbol);

unsigned int func_dlopen = NULL;
unsigned int func_dlsym = NULL;

unsigned int func_y_6 = NULL;
unsigned int func_y_5 = NULL;
unsigned int func_y_4 = NULL;
unsigned int func_y_3 = NULL;
unsigned int func_y_2 = NULL;
unsigned int func_y_1 = NULL;

void hook();

JNIEnv *env;
JavaVM *g_jvm;
unsigned long base = 0;


//特值处理
void Func_SpecificTreatment(void* arg,void* arg1,void* arg2,void* arg3);
//原函数指针
void* (*old_func_dlopen)(const char* filename, int flags, const void* caller_addr) = NULL;

void* (*old_fun_dlsym)(void* /*handle*/, const char* /*symbol*/) = NULL;
void* (*old_func_y_6)(void*,void*,void*,void*) = NULL;
void* (*old_func_y_5)(void*,void*,void*,void*) = NULL;
void* (*old_func_y_4)(void*,void*,void*,void*) = NULL;
void* (*old_func_y_3)(void*,void*,void*,void*) = NULL;
void* (*old_func_y_2)(void*,void*,void*,void*) = NULL;

void* (*old_func_y_1)(void*,void*,void*,void*) = NULL;

void* new_func_dlopen(const char *filename, int flags, const void *caller_addr) {
    void* p = old_func_dlopen(filename,flags,caller_addr);
    LOGD("%p = _loader_dlopen('%s','%d','%p')",p,filename,flags,caller_addr);
    if(strstr(filename,"libil2cpp") != NULL && caller_addr !=0){
        base = find_module_by_name(lib_name);
        hook();
        LOGD("find libil2cpp and inject");
    }
    return old_func_dlopen(filename,flags,caller_addr);
}

void* new_func_dlsym(void *handle, const char *symbol){
    LOGD("__loader_dlsym('%p','%s')",handle,symbol);
    void* ret = old_fun_dlsym(handle,symbol);
    LOGD("ret %p = __loader_dlsym('%p','%s')",ret,handle,symbol);
    return ret;
}

void* new_func_y_6(void* arg,void* arg1,void* arg2,void* arg3){
    LOGD("Enter new_func_y_6");
//    void* ret = old_func_y_6(arg,arg1,arg2,arg3);
    Func_SpecificTreatment(arg,arg1,arg2,arg3);
    return 0;
}

void* new_func_y_5(void* arg,void* arg1,void* arg2,void* arg3){
    LOGD("Enter new_func_y_5");
//    show_sa10(env,g_jvm);
    void* ret = old_func_y_5(arg,arg1,arg2,arg3);
    return ret;
}

void* new_func_y_4(void* arg,void* arg1,void* arg2,void* arg3){
    LOGD("Enter new_func_y_4");
    show_sa10(env,g_jvm);
    void* ret = old_func_y_4(arg,arg1,arg2,arg3);
    return ret;
}

void* new_func_y_3(void* arg,void* arg1,void* arg2,void* arg3){
    LOGD("Enter new_func_y_3");
//    show_sa10(env,g_jvm);
    void* ret = old_func_y_3(arg,arg1,arg2,arg3);
    return ret;
}

void* new_func_y_2(void* arg,void* arg1,void* arg2,void* arg3){
    LOGD("Enter new_func_y_2");
    show_sa10(env,g_jvm);
    void* ret = old_func_y_2(arg,arg1,arg2,arg3);
    return ret;
}

void* new_func_y_1(void* arg,void* arg1,void* arg2,void* arg3){
    LOGD("Enter new_func_y_1");
    show_sa10(env,g_jvm);
    void* ret = old_func_y_1(arg,arg1,arg2,arg3);
    return ret;
}

void Func_SpecificTreatment(void* arg,void* arg1,void* arg2,void* arg3){
//    hexDump(static_cast<const char *>(arg1), 16);
//    LOGD("%d ",*(&arg1 + 16));
//    LOGD("%d ",*(&arg1 + 17));
//    void* p = calloc(1, sizeof(char));
//    memcpy(p, (char *) arg1 + sizeof(char) * 16, 2);
//    hexDump(static_cast<const char *>(p), 16);
//    LOGD("%d ",p);
    old_func_y_5(arg,arg1,arg2,arg3);
//    show_sa10(env,g_jvm);
}

jint JNICALL
JNI_OnLoad(JavaVM *vm, void *reserved) {

    LOGE("------------------- JNI_OnLoad -------------------");

    if (vm->GetEnv( (void**)&env, JNI_VERSION_1_6) == JNI_OK) {
        LOGD("GetEnv OK");
    }
    if(vm->AttachCurrentThread(&env, NULL)  ==  JNI_OK)
    {
        LOGE("\n[*]Called AttachCurrentThread OK");
    }
    g_jvm = vm;
    func_dlopen = reinterpret_cast<unsigned int>((m_dlopen) dlopen);
    func_dlsym = reinterpret_cast<unsigned int>((m_dlsym) dlsym);

    LOGD("func_dlopen = 0x%x   -----  func_dlsym = 0x%x ",func_dlopen,func_dlsym);
    LOGE("------------------- InlineHook -------------------");

    //注册Hook信息
    registerInlineHook((uint32_t) func_dlopen, (uint32_t) new_func_dlopen, (uint32_t **) &old_func_dlopen)==ELE7EN_OK ?
    LOGD("Success Hook func_dlopen at 0x%x",func_dlopen):LOGE("Fail Hook func_dlopen at 0x%x",func_dlopen);

//    registerInlineHook((uint32_t) func_dlsym, (uint32_t) new_func_dlsym, (uint32_t **) &old_fun_dlsym)==ELE7EN_OK ?
//    LOGD("Success Hook func_dlsym at 0x%x",func_dlsym):LOGE("Fail Hook func_dlsym at 0x%x",func_dlsym);


    base = find_module_by_name(lib_name);
    if (base == 0){
        LOGE("Find %s at 0x%x", lib_name, base);
        LOGE("Enable hook dlopen");
        inlineHook(func_dlopen);
    }else{
        hook();
    }
//    inlineHook(func_dlsym);

    LOGE("-------------------  Function  -------------------");

    return JNI_VERSION_1_6;
}

void hook() {
    func_y_6 = base + 0x7dc664;
    func_y_5 = base + 0x7DC898;
    func_y_4 = base + 0x0;
    func_y_3 = base + 0x0;
    func_y_2 = base + 0x0;
    func_y_1 = base + 0x0;

    if (func_y_6 != base)
    registerInlineHook((uint32_t) func_y_6, (uint32_t) new_func_y_6,
                       (uint32_t **) &old_func_y_6) == ELE7EN_OK ?
    LOGD("Success Hook func_y_6 at 0x%x", func_y_6) : LOGE(
            "Fail Hook func_y_6 at 0x%x", func_y_6);

    if (func_y_5 != base)
    registerInlineHook((uint32_t) func_y_5, (uint32_t) new_func_y_5,
                       (uint32_t **) &old_func_y_5) == ELE7EN_OK ?
    LOGD("Success Hook func_y_5 at 0x%x", func_y_5) : LOGE(
            "Fail Hook func_y_5 at 0x%x", func_y_5);

    if (func_y_4 != base)
    registerInlineHook((uint32_t) func_y_4, (uint32_t) new_func_y_4,
                       (uint32_t **) &old_func_y_4) == ELE7EN_OK ?
    LOGD("Success Hook func_y_4 at 0x%x", func_y_4) : LOGE(
            "Fail Hook func_y_4 at 0x%x", func_y_4);

    if (func_y_3 != base)
    registerInlineHook((uint32_t) func_y_3, (uint32_t) new_func_y_3,
                       (uint32_t **) &old_func_y_3) == ELE7EN_OK ?
    LOGD("Success Hook func_y_3 at 0x%x", func_y_3) : LOGE(
            "Fail Hook func_y_3 at 0x%x", func_y_3);

    if (func_y_2 != base)
    registerInlineHook((uint32_t) func_y_2, (uint32_t) new_func_y_2,
                       (uint32_t **) &old_func_y_2) == ELE7EN_OK ?
    LOGD("Success Hook func_y_2 at 0x%x", func_y_2) : LOGE(
            "Fail Hook func_y_2 at 0x%x", func_y_2);

    if (func_y_1 != base)
    registerInlineHook((uint32_t) func_y_1, (uint32_t) new_func_y_1,
                       (uint32_t **) &old_func_y_1) == ELE7EN_OK ?
    LOGD("Success Hook func_y_1 at 0x%x", func_y_1) : LOGE(
            "Fail Hook func_y_1 at 0x%x", func_y_1);

    inlineHookAll();
}
