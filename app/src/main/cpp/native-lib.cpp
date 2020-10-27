#include <jni.h>
#include <string>
#include <dobby.h>
#include <android/log.h>
#include <sys/param.h>
#include "Tools/tools.h"

void MainEnter(std::string string, int string1);

extern "C" JNIEXPORT jstring JNICALL
Java_com_lzy_dobbytest_MainActivity_stringFromJNI(
        JNIEnv *env,
        jobject /* this */) {
    std::string hello = "Hello from C++";
    MainEnter(hello, 5000);
    return env->NewStringUTF(hello.c_str());
}

int tolower1(int c)
{
    if ((c >= 'A') && (c <= 'Z'))
        return c + ('a' - 'A');
    return c;
}

void MainEnter(std::string dd, int ff) {
    char* ctsr = "lOOK ! GhJ iT is 中却未能 a test string";
    char* newStr = static_cast<char *>(calloc(80, sizeof(char)));
    memset(newStr,0,80);
    LOGD("%s  %d",dd.c_str(),ff);
    UTF8_to_Unicode(newStr, ctsr);

    hexDump(ctsr,80);
    LOGD("-------------------------1");
    hexDump(newStr,80);

    tolower_unicode(newStr,80);

    LOGD("-------------------------2");
    hexDump(newStr,80);


}

extern "C"
void _init(void) {
    char* ctsr = "lOOK ! GhJ iT is 中却未能 a test string";
    hexDump(ctsr,80);
}

__attribute__((constructor))
void func_constructor(void) {
    char* ctsr = "12312312";
    hexDump(ctsr,80);
}

__attribute__((constructor))
void func1_constructor(void) {
    char* ctsr = "12312312";
    hexDump(ctsr,80);
}

__attribute__((constructor))
void func2_constructor(void) {
    char* ctsr = "12312312";
    hexDump(ctsr,80);
}

