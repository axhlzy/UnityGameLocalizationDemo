//
// Created by lzy on 2020/10/21.
//
#include <jni.h>
#include <string>
#include <android/log.h>
#include <sys/param.h>
#include <unistd.h>
#include <logging/logging.h>

#include "tools.h"

uint32_t UTF8_to_Unicode(char *dst, char *src) {
    uint32_t i = 0, unicode = 0, ii, iii;
    int codeLen = 0;
    while (*src) {
        //1. UTF-8 ---> Unicode
        if (0 == (src[0] & 0x80)) {
            // 单字节
            codeLen = 1;
            unicode = src[0];
        } else if (0xC0 == (src[0] & 0xE0) && 0x80 == (src[1] & 0xC0)) {// 双字节
            codeLen = 2;
            unicode = (uint32_t) ((((uint32_t) src[0] & 0x001F) << 6) |
                                  ((uint32_t) src[1] & 0x003F));
        } else if (0xE0 == (src[0] & 0xF0) && 0x80 == (src[1] & 0xC0) &&
                   0x80 == (src[2] & 0xC0)) {// 三字节
            codeLen = 3;
            ii = (((uint32_t) src[0] & 0x000F) << 12);
            iii = (((uint32_t) src[1] & 0x003F) << 6);
            unicode = ii | iii | ((uint32_t) src[2] & 0x003F);
            unicode = (uint32_t) ((((uint32_t) src[0] & 0x000F) << 12) |
                                  (((uint32_t) src[1] & 0x003F) << 6) |
                                  ((uint32_t) src[2] & 0x003F));
        } else if (0xF0 == (src[0] & 0xF0) && 0x80 == (src[1] & 0xC0) && 0x80 == (src[2] & 0xC0) &&
                   0x80 == (src[3] & 0xC0)) {// 四字节
            codeLen = 4;
            unicode = (((int) (src[0] & 0x07)) << 18) | (((int) (src[1] & 0x3F)) << 12) |
                      (((int) (src[2] & 0x3F)) << 6) | (src[3] & 0x3F);
        } else {
            break;
        }
        src += codeLen;
        //这里注释的代码是为了判断开始的0x20（空格）的显示
//        if (unicode < 0x80) {
//            if (i == 0 && unicode == 0x20) {
//                continue;
//            }
//        }
        i += 2;
        *dst++ = (u_int8_t) ((unicode & 0xff));
        *dst++ = (u_int8_t) (((unicode >> 8) & 0xff));
    } // end while
    *dst = 0;
    return i;
}

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

unsigned long getCurrentTime() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

unsigned long find_module_by_name(char *soName) {
    char filename[32];
    char cmdline[256];
    sprintf(filename, "/proc/%d/maps", getpid());
    FILE *fp = fopen(filename, "r");
    unsigned long revalue = 0;
    if (fp) {
        while (fgets(cmdline, 256, fp))
        {
            if (strstr(cmdline, soName) && strstr(cmdline, "r-xp"))
            {
                char *str = strstr(cmdline, "-");
                if (str) {
                    *str = '\0';
                    char num[32];
                    sprintf(num, "0x%s", cmdline);
                    revalue = strtoul(num, NULL, 0);
                    return revalue;
                }
            }
            memset(cmdline, 0, 256);
        }
        fclose(fp);
    }
    return 0L;
}

char *getPackageName(JNIEnv *env) {
    jobject context = getApplication(env);
    if (context == NULL) {
        LOGE("context is null!");
        return NULL;
    }
    jclass activity = env->GetObjectClass(context);
    jmethodID methodId_pack = env->GetMethodID(activity, "getPackageName", "()Ljava/lang/String;");
    jstring name_str = static_cast<jstring >( env->CallObjectMethod(context, methodId_pack));
    const char *src_str = env->GetStringUTFChars(name_str, NULL);
    char *str_tmp = static_cast<char *>(calloc(strlen(src_str), sizeof(char)));
    strcpy(str_tmp,src_str);
    return str_tmp;
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

void tolower_unicode(char* c,int length)
{
    int des = 'a' - 'A';
    int current_pos = 0;
    while (current_pos <= length){
        if (*(c+current_pos+1) == 0 && *(c+current_pos) != 0){
            if ((*(c+current_pos) >= 'A') && (*(c+current_pos) <= 'Z')){
                *(c+current_pos) = *(c+current_pos) + des;
            }
        }
        current_pos +=2;
    }
}