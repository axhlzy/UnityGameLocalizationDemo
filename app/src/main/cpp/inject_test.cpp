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

#define LOG_TAG "ZZZ"
#define LOGD(fmt, args...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG,fmt, ##args)
#define LOGE(fmt, args...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG,fmt, ##args)

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
unsigned long getCurrentTime();
void show_toast();
void show_sa10();

//记录启动时间
static long StartTime = getCurrentTime();
//防止连续点击的延时
static long Display_advertising_interval = 1*3*1000;
//开始启动hook的延时
static long Start_time_delay = 1*1*1000;
//每几次触发一次
static int times_delay_s = 1;

char *lib_name = const_cast<char *>("libil2cpp.so");

JNIEnv *env;
JavaVM *g_jvm;
unsigned long base = 0;
unsigned long last_milles = 0;

unsigned int times_delay = 0;

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
    void* ret = old_func_y_6(arg,arg1,arg2,arg3);
    Func_SpecificTreatment(arg,arg1,arg2,arg3);
    return ret;
}

void* new_func_y_5(void* arg,void* arg1,void* arg2,void* arg3){
    LOGD("Enter new_func_y_5");
    show_sa10();
    void* ret = old_func_y_5(arg,arg1,arg2,arg3);
    return ret;
}

void* new_func_y_4(void* arg,void* arg1,void* arg2,void* arg3){
    LOGD("Enter new_func_y_4");
    show_sa10();
    void* ret = old_func_y_4(arg,arg1,arg2,arg3);
    return ret;
}

void* new_func_y_3(void* arg,void* arg1,void* arg2,void* arg3){
    LOGD("Enter new_func_y_3");
//    show_sa10();
    void* ret = old_func_y_3(arg,arg1,arg2,arg3);
    return ret;
}

void* new_func_y_2(void* arg,void* arg1,void* arg2,void* arg3){
    LOGD("Enter new_func_y_2");
    show_sa10();
    void* ret = old_func_y_2(arg,arg1,arg2,arg3);
    return ret;
}

void* new_func_y_1(void* arg,void* arg1,void* arg2,void* arg3){
    LOGD("Enter new_func_y_1");
    show_sa10();
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
//    old_func_y_3(arg,arg1,arg2,arg3);
    show_sa10();
}

void show_sa10() {
    //启动延时
    if (getCurrentTime()-StartTime < Start_time_delay){
        LOGE("\n[*]start-up delay residue ：%d",Start_time_delay + StartTime - getCurrentTime());
    }else{
        //连续点击判断延时
        if (getCurrentTime() - last_milles > Display_advertising_interval) {
            last_milles = getCurrentTime();
            //每times_delay_s次触发一次
            if (++times_delay % times_delay_s == 0){
                if (g_jvm->AttachCurrentThread(&env, NULL) == JNI_OK) {
                    LOGE("\n[*]AttachCurrentThread OK");
                }
                LOGE("called com.was.m.RewardManager.sa10");
                jclass RewardManager = env->FindClass("com/was/m/RewardManager");
                jmethodID sa10 = env->GetStaticMethodID(RewardManager, "sa10", "()V");
                env->CallStaticVoidMethod(RewardManager, sa10, NULL);
            }else{
                LOGD("current times %d ", times_delay);
            }
        } else {
            LOGD("getCurrentTime() - last_milles = %d ", getCurrentTime() - last_milles);
        }
    }

}

//void show_toast() {
//    if (getCurrentTime() - last_milles > Display_advertising_interval) {
//        last_milles = getCurrentTime();
//        if (g_jvm->AttachCurrentThread(&env, NULL) == JNI_OK) {
//            LOGE("\n[*]AttachCurrentThread OK");
//        }
//        jobject context = getApplication(env);
//        jclass player= env->FindClass("com/unity3d/player/UnityPlayerActivity");
//        jmethodID jm_makeText=env->GetStaticMethodID(player,"showToast","(Landroid/app/Application;)V");
//        env->CallStaticObjectMethod(player,jm_makeText,context);
//    } else {
//        LOGD("getCurrentTime() - last_milles = %d < %d    ", getCurrentTime() - last_milles,Display_advertising_interval);
//    }
//}


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
    func_y_6 = base + 0x280720;
    func_y_5 = base + 0x2786bc;
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
