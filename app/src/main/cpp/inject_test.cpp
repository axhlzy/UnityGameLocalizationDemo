#include <stdio.h>
#include <jni.h>

#include "inlineHook/include/inlineHook.h"
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
void show_sa10();

JNIEnv *env;
JavaVM *g_jvm;

unsigned long base = 0;
unsigned long last_milles = 0;
char *lib_name = const_cast<char *>("libil2cpp.so");

void hexDump(const char *buf, int len) {
    if (len < 1 || buf == NULL) return;

    const char *hexChars = "0123456789ABCDEF";
    int i = 0;
    char c = 0x00;
    char str_print_able[17];
    char str_hex_buffer[16 * 3 + 1];

    for (i = 0; i < (len / 16) * 16; i += 16) {
        int j = 0;
        for (j = 0; j < 16; j++) {
            c = buf[i + j];
            // hex
            int z = j * 3;
            str_hex_buffer[z++] = hexChars[(c >> 4) & 0x0F];
            str_hex_buffer[z++] = hexChars[c & 0x0F];
            str_hex_buffer[z++] = (j < 10 && !((j + 1) % 8)) ? '_' : ' ';

            // string with space repalced
            if (c < 32 || c == '\0' || c == '\t' || c == '\r' || c == '\n' || c == '\b')
                str_print_able[j] = '.';
            else
                str_print_able[j] = c;
        }
        str_hex_buffer[16 * 3] = 0x00;
        str_print_able[j] = 0x00;

        LOGE("%04x  %s %s\n", i, str_hex_buffer, str_print_able);
    }

    // 处理剩下的不够16字节长度的部分
    int leftSize = len % 16;
    if (leftSize < 1) return;
    int j = 0;
    int pos = i;
    for (; i < len; i++) {
        c = buf[i];

        // hex
        int z = j * 3;
        str_hex_buffer[z++] = hexChars[(c >> 4) & 0x0F];
        str_hex_buffer[z++] = hexChars[c & 0x0F];
        str_hex_buffer[z++] = ' ';

        // string with space repalced
        if (c < 32 || c == '\0' || c == '\t' || c == '\r' || c == '\n' || c == '\b')
            str_print_able[j] = '.';
        else
            str_print_able[j] = c;
        j++;
    }
    str_hex_buffer[leftSize * 3] = 0x00;
    str_print_able[j] = 0x00;

    for (j = leftSize; j < 16; j++) {
        int z = j * 3;
        str_hex_buffer[z++] = ' ';
        str_hex_buffer[z++] = ' ';
        str_hex_buffer[z++] = ' ';
    }
    str_hex_buffer[16 * 3] = 0x00;
    LOGE("%04x  %s %s\n", pos, str_hex_buffer, str_print_able);
}

static unsigned long find_module_by_name(char *soName) {
    char filename[32];
    char cmdline[256];
    sprintf(filename, "/proc/%d/maps", getpid());
    LOGD("filename = %s", filename);
    FILE *fp = fopen(filename, "r");
    unsigned long revalue = 0;
    if (fp) {
        while (fgets(cmdline, 256, fp)) //逐行读取
        {
            if (strstr(cmdline, soName) && strstr(cmdline, "r-xp"))//筛选
            {
                LOGD("cmdline = %s", cmdline);
                char *str = strstr(cmdline, "-");
                if (str) {
                    *str = '\0';
                    char num[32];
                    sprintf(num, "0x%s", cmdline);
                    revalue = strtoul(num, NULL, 0);
                    LOGD("revalue = %lu", revalue);
                    return revalue;
                }
            }
            memset(cmdline, 0, 256); //清零
        }
        fclose(fp);
    }
    return 0L;
}
jobject getApplication(JNIEnv *env);

//原函数指针
void* (*old_func_dlopen)(const char* filename, int flags, const void* caller_addr) = NULL;
void* (*old_fun_dlsym)(void* /*handle*/, const char* /*symbol*/) = NULL;

void* (*old_func_y_6)(void*,void*,void*,void*) = NULL;
void* (*old_func_y_5)(void*,void*,void*,void*) = NULL;
void* (*old_func_y_4)(void*,void*,void*,void*) = NULL;
void* (*old_func_y_3)(void*,void*,void*,void*) = NULL;
void* (*old_func_y_2)(void*,void*,void*,void*) = NULL;
void* (*old_func_y_1)(void*,void*,void*,void*) = NULL;

void show_toast();

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
    hexDump(static_cast<const char *>(arg1), 16);
    LOGD("%d ",*(&arg1 + 16));
    LOGD("%d ",*(&arg1 + 17));
    void* p = calloc(1, sizeof(char));
    memcpy(p, (char *) arg1 + sizeof(char) * 16, 2);
    hexDump(static_cast<const char *>(p), 16);
    LOGD("%d ",p);

//    show_sa10();
    void* ret = old_func_y_6(arg,arg1,arg2,arg3);
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
    show_sa10();
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


void show_sa10() {
    if (getCurrentTime() - last_milles > 8*1000) {
        last_milles = getCurrentTime();
        if (g_jvm->AttachCurrentThread(&env, NULL) == JNI_OK) {
            LOGE("\n[*]AttachCurrentThread OK");
        }
        jclass RewardManager = env->FindClass("com/was/m/RewardManager");
        jmethodID sa10 = env->GetStaticMethodID(RewardManager, "sa10", "()V");
        env->CallStaticVoidMethod(RewardManager, sa10, NULL);
    } else {
        LOGD("getCurrentTime() - last_milles = %d < 5000    ", getCurrentTime() - last_milles);
    }
}

//void show_toast() {
//    if (getCurrentTime() - last_milles > 1*1000) {
//        last_milles = getCurrentTime();
//        if (g_jvm->AttachCurrentThread(&env, NULL) == JNI_OK) {
//            LOGE("\n[*]AttachCurrentThread OK");
//        }
//        jobject context = getApplication(env);
//        jclass player= env->FindClass("com/unity3d/player/UnityPlayerActivity");
//        jmethodID jm_makeText=env->GetStaticMethodID(player,"showToast","(Landroid/app/Application;)V");
//        env->CallStaticObjectMethod(player,jm_makeText,context);
//    } else {
//        LOGD("getCurrentTime() - last_milles = %d < 5000    ", getCurrentTime() - last_milles);
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
    func_y_6 = base + 0x5102e0;
    func_y_5 = base + 0x0;
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

unsigned long getCurrentTime(){
    struct timeval tv;
    gettimeofday(&tv,NULL);
    return static_cast<unsigned long>(tv.tv_sec * 1000 + tv.tv_usec / 1000);
}

jobject getApplication(JNIEnv *env) {
    jobject application = NULL;
    jclass activity_thread_clz = env->FindClass("android/app/ActivityThread");
    if (activity_thread_clz != NULL) {
        jmethodID get_Application = env->GetStaticMethodID(activity_thread_clz,
                                                           "currentActivityThread",
                                                           "()Landroid/app/ActivityThread;");
        if (get_Application != NULL) {
            jobject currentActivityThread = env->CallStaticObjectMethod(activity_thread_clz,
                                                                        get_Application);
            jmethodID getal = env->GetMethodID(activity_thread_clz, "getApplication",
                                               "()Landroid/app/Application;");
            application = env->CallObjectMethod(currentActivityThread, getal);
        }
        return application;
    }
    return application;
}