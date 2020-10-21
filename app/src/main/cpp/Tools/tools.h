//
// Created by Admin on 2020/10/21.
//

#define LOG_TAG "ZZZ"

#define LOGD(fmt, args...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG,fmt, ##args)
#define LOGE(fmt, args...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG,fmt, ##args)

#ifndef DOBBYTEST_TOOLS_H
#define DOBBYTEST_TOOLS_H

uint32_t UTF8_to_Unicode(char *dst, char *src);
void hexDump(const char *buf, int len);
unsigned long getCurrentTime();
unsigned long find_module_by_name(char *soName);

char *getPackageName(JNIEnv *env);
jobject getApplication(JNIEnv *env);
void tolower_unicode(char* c,int length);

#endif