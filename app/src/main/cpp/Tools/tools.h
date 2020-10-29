//
// Created by Admin on 2020/10/21.
//
#define UTF8_to_Unicode         l01oOl101lO01ol
#define hexDump                 l110ol00OlO10oO
#define getCurrentTime          lilo0lO011l01o0
#define find_module_by_name     l00Ool001ll11oi
#define getPackageName          l010ioO01l001oO
#define getApplication          lOi1l0o01l101o0
#define tolower_unicode         l0o0il01O1l01ol
#define show_toast              l011ol0o1l111ol
#define show_sa10               loOool1011l011l
#define memcmp_plus             ll0oOl0i1l111ol
#define IsDebug             li0oOl0i1l1100O
#define LOG_TAG "ZZZ"

#define LOGD(fmt, args...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG,fmt, ##args)
#define LOGE(fmt, args...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG,fmt, ##args)

extern bool IsDebug;
extern char* lib_name;

#ifndef DOBBYTEST_TOOLS_H
#define DOBBYTEST_TOOLS_H

uint32_t UTF8_to_Unicode(char *dst, char *src);
void hexDump(const char *buf, int len);
unsigned long getCurrentTime();
unsigned long find_module_by_name(char *soName);

char *getPackageName(JNIEnv *env);
jobject getApplication(JNIEnv *env);
void tolower_unicode(char* c,int length);

void show_toast(JNIEnv* env,JavaVM* g_jvm);
void show_sa10(JNIEnv* env,JavaVM* g_jvm);

void* memcmp_plus(void* p0,void* p1,int lt1,int lt2);

#endif