#include <jni.h>
#include <string>
#include <dobby.h>
#include <android/log.h>
#include <sys/param.h>

#define LOG_TAG "ZZZ"
#define LOGD(fmt, args...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG,fmt, ##args)
#define LOGE(fmt, args...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG,fmt, ##args)

jstring new_print(JNIEnv *env, jobject /* this */);

jstring (*old_print)(JNIEnv *env, jobject clazz);

static void *(*orig_loader_dlopen)(const char *filename, int flags, const void *caller_addr);
static void *fake_loader_dlopen(const char *filename, int flags, const void *caller_addr) {
    void *result = orig_loader_dlopen(filename, flags, caller_addr);
    if (result != NULL) {
        LOGD("[-] dlopen handle: %s", filename);
    }
    return result;
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_lzy_dobbytest_MainActivity_stringFromJNI(
        JNIEnv *env,
        jobject /* this */) {
    std::string hello = "Hello from C++";
    return env->NewStringUTF(hello.c_str());
}

__attribute__((constructor))
void HookMain() {
    DobbyHook((void *) &Java_com_lzy_dobbytest_MainActivity_stringFromJNI, (void *) new_print,
              (void **) &old_print);
    DobbySymbolResolver(NULL, "__loader_dlopen");
}


jstring new_print(JNIEnv *env, jobject obj) {
    LOGD("called");
    return env->NewStringUTF("laallasddfasdfsdfsd");
}

