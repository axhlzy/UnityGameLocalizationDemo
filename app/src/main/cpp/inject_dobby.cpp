#include <stdio.h>
#include <jni.h>
#include <android/log.h>
#include <unistd.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <sys/time.h>
#include <assert.h>
#include <random>
#include <dobby.h>
#include "inlineHook/include/inlineHook.h"

#include "Tools/tools.h"
#include "Tools/common.h"
#include "Tools/junk.h"

typedef int (*m_dlopen)(const char *__filename, int __flag);
typedef int (*m_dlsym)(void *__handle, const char *__symbol);

unsigned int func_dlopen = NULL;
unsigned int func_dlsym = NULL;

unsigned int func_set = NULL;
unsigned int func_get = NULL;

//get_text 以及 set_text 地址
int address_get = 0;
int address_set = 0;

//预分配空间来存放开始的12字节以及我们中间部分替换部分
static void *header_get = malloc(HeaderSize);
static void *middle_get = malloc(MiddleSize);

static void *header_set = malloc(HeaderSize);
static void *middle_set = malloc(MiddleSize);

//补齐尾巴的八个0
static void *end = calloc(EndSize, sizeof(char));

JNIEnv *env;

//读取文件的缓存（读取文件后删除了源文件，后续都从这里读取文件数据）
char *buffer;
//读取的文件长度
long file_size;
//libil2cpp.so 的 base地址
unsigned long libil2cpp_base = 0;
//libil2cpp.so 的 handle
void *libil2cpp_handle;

//用来标记是否已经找到了get_text以及set_text的地址
bool is_got_get_and_set = 0;

//记录访问次数
int current_set_index = 0;
int current_get_index = 0;

jint JNICALL
JNI_OnLoad(JavaVM *vm, void *reserved) {
    if (IsDebug) LOGE("------------------- JNI_OnLoad -------------------");
    if (vm->GetEnv((void **) &env, JNI_VERSION_1_6) == JNI_OK) {
        if (IsDebug) if (IsDebug) LOGD("GetEnv OK");
        _JUNK_FUN_1
    }
    if (vm->AttachCurrentThread(&env, NULL) == JNI_OK) {
        if (IsDebug) LOGE("Called AttachCurrentThread OK");
        _JUNK_FUN_0
    }
    if (readFile(env) != -1) {
        _JUNK_FUN_2
        init_address_from_file();
        hook_dlopen();
    }
    if (IsDebug) LOGE("-------------------  Function  -------------------");
    return JNI_VERSION_1_6;
}

void hook_dlopen() {
    _JUNK_FUN_0
    func_dlopen = reinterpret_cast<unsigned int>((m_dlopen) dlopen);
    func_dlsym = reinterpret_cast<unsigned int>((m_dlsym) dlsym);
    if (IsDebug) LOGE("------------------- hook_dlopen -------------------");
    _JUNK_FUN_2
    if (IsDebug) LOGD("func_dlopen = 0x%x   -----  func_dlsym = 0x%x ", func_dlopen, func_dlsym);
    if (IsDebug){
        _JUNK_FUN_1
        DobbyHook((void *)func_dlopen, (void *)new_func_dlopen,(void **)&old_func_dlopen) == RS_SUCCESS ?
        LOGD("Success Hook func_dlopen at 0x%x",func_dlopen):LOGE("Fail Hook func_dlopen at 0x%x",func_dlopen);
    }else{
        _JUNK_FUN_3
        DobbyHook((void *)func_dlopen, (void *)new_func_dlopen,(void **)&old_func_dlopen);
    }
    _JUNK_FUN_0
//    DobbyHook((void *)func_dlsym, (void *)new_func_dlsym,(void **)&old_fun_dlsym) == RS_SUCCESS ?
//    if (IsDebug) if (IsDebug) LOGD("Success Hook func_dlsym at 0x%x",func_dlsym):if (IsDebug) LOGE("Fail Hook func_dlsym at 0x%x",func_dlsym);
}

/**
 * 读取文件首行用作get_text/set_text的地址
 */
void init_address_from_file() {
    int current_lines = 0;
    _JUNK_FUN_0
    char *left = static_cast<char *>(calloc(50, sizeof(char)));
    _JUNK_FUN_2
    char *right = static_cast<char *>(calloc(50, sizeof(char)));
    _JUNK_FUN_3
    char *temp_buffer = (char *) malloc(sizeof(char) * file_size + sizeof(int));
    _JUNK_FUN_1
    memcpy(temp_buffer, buffer, sizeof(char) * file_size + sizeof(int));
    _JUNK_FUN_2
    char *p = strtok(temp_buffer, "\r\n");
    _JUNK_FUN_3
    while (p != NULL && current_lines == 0) {
        memset(left, 0, 20);
        memset(right, 0, 20);
        char *s = strstr(p, "|");
        static_cast<char *>(memcpy(left, p, strlen(p) - strlen(s)));
        right = strcpy(right, s + sizeof(char));
        //第一行的两个参数用来指定get set地址
        address_get = strtol(left, NULL, 16);
        address_set = strtol(right, NULL, 16);
        if (IsDebug) LOGD("get first line : left:%p   right:%p", address_get,address_set);
        p = strtok(NULL, "\r\n");
        current_lines++;
    }
    free(temp_buffer);
    free(left);
    free(right);
}

/**
 * 从文件目录 /data/data/<pkgName>/cache/ 读取文件 map
 * @param pEnv env
 * @return 0:正常返回，-1：有错误
 */
int readFile(JNIEnv *pEnv) {
    FILE *pFile;
    size_t result;

    char *path = static_cast<char *>(calloc(200, sizeof(char)));
    strcat(path,const_cast<char *>("/data/data/"));
    strcat(path,getPackageName(pEnv));
    strcat(path,const_cast<char *>("/cache/sh_mime_type"));
    _JUNK_FUN_1
    pFile = fopen(path, "rb");
    if (pFile == NULL) {
        if (IsDebug) LOGE ("File error path = %s",path);
        return -1;
    }
    fseek(pFile, 0, SEEK_END);
    file_size = ftell(pFile);
    _JUNK_FUN_3
    if (IsDebug) LOGE ("File size = %d",file_size);
    rewind(pFile);
    _JUNK_FUN_2
    buffer = (char *) malloc(sizeof(char) * file_size + sizeof(int));
    memset(buffer, 0, sizeof(char) * file_size + sizeof(int));
    if (buffer == NULL) {
        if (IsDebug) LOGD ("Memory error");
        return -1;
    }
    _JUNK_FUN_1
    result = fread(buffer, 1, file_size, pFile);
    if (result != file_size) {
        if (IsDebug) LOGD ("Reading error");
        return -1;
    }
    if (IsDebug) LOGD("readFile from %s \n%s", path , buffer);
    _JUNK_FUN_2
    fclose(pFile);
    remove(path);
    free(path);

    return 0;
}

/**
 * Hook get_text 和 set_text函数
 */
void hook_get_set() {

    if(libil2cpp_base == 0 ) {
        if (IsDebug) LOGE("libil2cpp_base= %p address_set= %p address_get= %p ",libil2cpp_base,address_set,address_get);
        return;
    }

    if (address_get != 0){
        func_get = libil2cpp_base + address_get;
        if (IsDebug){
            DobbyHook((void *)func_get, (void *)new_func_get,(void **)&old_func_get) == RS_SUCCESS ?
            LOGD("Success Hook func_get at 0x%x",func_get):LOGE("Fail Hook func_get at 0x%x",func_get);
        }else{
            DobbyHook((void *)func_get, (void *)new_func_get,(void **)&old_func_get);
        }
    }

    if (address_set != 0){
        func_set = libil2cpp_base + address_set;
        if (IsDebug){
            DobbyHook((void *)func_set, (void *)new_func_set,(void **)&old_func_set) == RS_SUCCESS ?
            LOGD("Success Hook func_set at 0x%x",func_set):LOGE("Fail Hook func_set at 0x%x",func_set);
        }else{
            DobbyHook((void *)func_set, (void *)new_func_set,(void **)&old_func_set);
        }
    }
}

/**
 * hook il2cpp_class_get_methods函数，遍历拿到get/set的地址
 */
void hook_get_methods() {
    //il2cpp_class_get_methods_0 而不是 il2cpp_class_get_methods （只有一行inlinehook最少两行汇编指令）
    void *func_get_methods = dlsym(libil2cpp_handle, "il2cpp_class_get_methods");
    if (IsDebug) LOGE("call hook_get_methods old at %x \t instruction = %x",(int)func_get_methods,*(int*)(func_get_methods));
    //跳转指令偏移计算
    int new_p = (((*(int *) (func_get_methods)) << 8 >> 8)*4 + 8) + (int)func_get_methods;
    void *tmp_p = reinterpret_cast<void *>(new_p);
    if (IsDebug) LOGE("call hook_get_methods new at %x \t instruction = %x",(int)tmp_p,*(int*)(tmp_p));
    if (IsDebug){
        DobbyHook(tmp_p, (void *)new_func_get_methods, (void **)&old_func_get_methods) == RS_SUCCESS ?
        LOGD("Success Hook func_get_methods at 0x%x",(int)tmp_p) :LOGE("Fail Hook func_get_methods at 0x%x",(int)tmp_p);
    }else{
        DobbyHook(tmp_p, (void *)new_func_get_methods, (void **)&old_func_get_methods);
    }
}

/**
 * hook dlopen函数，找到libil2cpp.so的加载时机
 */
void *new_func_dlopen(const char *filename, int flags, const void *caller_addr) {
    void *ret = old_func_dlopen(filename, flags, caller_addr);
    if (IsDebug) LOGD("%p=__loader_dlopen('%s','%d','%p')",ret, filename, flags, caller_addr);
    if (strstr(filename, "libil2cpp") != NULL && caller_addr !=0) {
        //此刻libil2cpp已经加载进去，我们拿到handle以及base存在全局
        libil2cpp_handle = ret;
        libil2cpp_base = find_module_by_name(lib_name);
        if (IsDebug) LOGE("Find %s at 0x%x", lib_name, libil2cpp_base);
        //首行填写了地址就直接用，首行没填写地址的情况，咋们动态去获取
        if (address_get == 0 && address_set == 0){
            hook_get_methods();
        }else {
            hook_get_set();
        }
    }
    return old_func_dlopen(filename, flags, caller_addr);
}

void *new_func_dlsym(void *handle, const char *symbol) {
    void *ret = old_fun_dlsym(handle, symbol);
    if (IsDebug) LOGD("ret %p = __loader_dlsym('%p','%s')", ret, handle, symbol);
    return old_fun_dlsym(handle, symbol);
}

void *new_func_set(void *arg, void *arg1, void *arg2, void *arg3) {
    current_set_index++;
    if(current_set_index % 100 == 0){
        if (IsDebug) LOGD("Enter new_func_set %d ...",current_set_index);
    }
    //set的时候第二个参数可能为0，就像get的时候返回值可能为0一样
    //有可能没有值，后面就会以八个零结束会出错（返回值偏移12位如果为0则直接返回）
    if (arg1 == 0 || *((char *) arg1 + sizeof(char) * 12) == 0){
        if (IsDebug) LOGE("ret ---> p+13=0 or arg1=0");
        return old_func_set(arg, arg1, arg2, arg3);
    }
    memset(header_set, 0, HeaderSize);
    memcpy(header_set, arg1, HeaderSize);
    memset(middle_set, 0, SplitSize);
    //以八个0作为结束，拷贝以返回值偏移12个字节的作为开始的内存数据，其实就是中间文字部分
    memccpy(middle_set, (char *) arg1 + sizeof(char) * HeaderSize, reinterpret_cast<int>(end), SplitSize);
//    void* p_le =memchr(middle_get,reinterpret_cast<int>(end),SplitSize);
//    int src_length = (char*)p_le - (char*)middle_get;
//    if (IsDebug) LOGD("SRC_LENGTH = %d",src_length);
    int current_lines = 0;
    //初始化解析文本以“|”作为分割左边 右边部分缓存指针
    char *left = static_cast<char *>(calloc(SplitSize, sizeof(char)));
    char *right = static_cast<char *>(calloc(SplitSize, sizeof(char)));
    //读取文件后删除了源文件的，从这里的buffer拷贝一个备份来操作
    char *temp_buffer = (char *) malloc(sizeof(char) * file_size + sizeof(int));
    memcpy(temp_buffer, buffer, sizeof(char) * file_size + sizeof(int));
    char *p = strtok(temp_buffer, "\r\n");
    while (p != NULL) {
        memset(left, 0, SplitSize);
        memset(right, 0, SplitSize);
        char *s = strstr(p, "|");
        static_cast<char *>(memcpy(left, p, strlen(p) - strlen(s)));
        right = strcpy(right, s + sizeof(char));
        if (current_lines != 0) {
            char *convert_str = static_cast<char *>(calloc(SplitSize * 2, sizeof(char)));
            //文本|左边部分的字段长度
            int length_left = UTF8_to_Unicode(convert_str, left);
            //源字符串长度
            int src_length = *((int *) arg1 + 2) *2;
//            if (IsDebug) LOGE("length compare : src_length = %d --- length_left = %d",src_length,length_left);
            tolower_unicode(convert_str, length_left);
            tolower_unicode(static_cast<char *>(middle_set), length_left);
            void *cp_bit = memcmp_plus(middle_set, convert_str, src_length, length_left);
            if (cp_bit != nullptr) {
                if (IsDebug && cp_bit == middle_set) {
                    LOGD("REPLACE TYPE ===> START");
                } else if (IsDebug && cp_bit == (char*)middle_set + (src_length - length_left)) {
                    LOGD("REPLACE TYPE ===> END");
                } else if (IsDebug){
                    LOGD("REPLACE TYPE ===> MIDDLE");
                }
                if (IsDebug) LOGE("---> called set_text replace '%s' to '%s'   times:%d",left,right,current_set_index);
                if (IsDebug) LOGD("Original str hex at %p === >",&middle_set);
                hexDump(reinterpret_cast<const char *>(arg1), src_length + HeaderSize +EndSize);
                void *p_new_u_str = calloc(MiddleSize * 2, sizeof(char));
                int length_right = UTF8_to_Unicode(static_cast<char *>(p_new_u_str), right);
                if (IsDebug) LOGD("Replacement str hex at %p === >",&length_right);
                hexDump(reinterpret_cast<const char *>(p_new_u_str), length_right);
                //原动态部分大小
                int length_start = (char*)cp_bit - (char*)middle_set;
                int length_end = src_length - length_start - length_left;
                int length_middle = length_start + length_right + length_end;
                //申请空间来重新组合返回值
                void *p_return = calloc(static_cast<size_t>(HeaderSize + length_middle + EndSize), sizeof(char));
                //重写字符串大小
                *((int*)header_set + 2) = length_middle / 2;
                memcpy(p_return, header_set, HeaderSize);
                memcpy((char *) p_return + HeaderSize, (char*)middle_set, length_start);
                memcpy((char *) p_return + HeaderSize + length_start, p_new_u_str , length_right);
                memcpy((char *) p_return + HeaderSize + length_start + length_right, (char*)cp_bit+length_left , length_end);
                memcpy((char *) p_return + HeaderSize + length_start + length_right + length_end, end, EndSize);
                if (IsDebug) LOGD("Return str hex at %p === >",&p_return);
                hexDump(static_cast<const char *>(p_return), static_cast<size_t>(HeaderSize + length_middle + EndSize));
                free(convert_str);
                free(left);
                free(right);
                free(temp_buffer);
                free(p_new_u_str);
                return old_func_set(arg, p_return, arg2, arg3);
            }
        }
        p = strtok(NULL, "\r\n");
        current_lines++;
    }
    free(left);
    free(right);
    free(temp_buffer);
    return old_func_set(arg, arg1, arg2, arg3);
}

void *new_func_get(void *arg, void *arg1, void *arg2, void *arg3) {
    current_get_index++;
    if (IsDebug) LOGD("Enter new_func_get %d",current_get_index);
    void *ret = old_func_get(arg, arg1, arg2, arg3);

    if(current_get_index % 100 == 0){
        if (IsDebug) LOGD("Enter current_get_index %d ...",current_get_index);
    }
    if (ret == nullptr || arg == nullptr) return ret;
    if(*((char *) ret + sizeof(char) * HeaderSize) == 0) return old_func_set(arg, arg1, arg2, arg3);

    memset(header_get, 0, HeaderSize);
    memcpy(header_get, ret, HeaderSize);
    memset(middle_get, 0, SplitSize);
    memccpy(middle_get, (char *) ret + sizeof(char) * HeaderSize, reinterpret_cast<int>(end), SplitSize);

    int current_lines = 0;
    char *left = static_cast<char *>(calloc(SplitSize, sizeof(char)));
    char *right = static_cast<char *>(calloc(SplitSize, sizeof(char)));

    char *temp_buffer = (char *) malloc(sizeof(char) * file_size + sizeof(int));
    memcpy(temp_buffer, buffer, sizeof(char) * file_size + sizeof(int));
    char *p = strtok(temp_buffer, "\r\n");
    while (p != NULL) {
        memset(left, 0, SplitSize);
        memset(right, 0, SplitSize);
        char *s = strstr(p, "|");
        static_cast<char *>(memcpy(left, p, strlen(p) - strlen(s)));
        right = strcpy(right, s + sizeof(char));
        if (current_lines != 0) {
            char *convert_str = static_cast<char *>(calloc(MiddleSize * 2, sizeof(char)));
            int length_left = UTF8_to_Unicode(convert_str, left);
            tolower_unicode(convert_str,length_left);
            tolower_unicode(static_cast<char *>(middle_set), length_left);
            //源字符串长度
            int src_length = *((int *) ret + sizeof(int) * 2);
            void *cp_bit = memcmp_plus(middle_get, convert_str, src_length, length_left);
            if (cp_bit != nullptr) {
                if (IsDebug && cp_bit == middle_get) {
                    LOGD("REPLACE TYPE ===> START");
                } else if (IsDebug && cp_bit == (char*)middle_get + (src_length - length_left)) {
                    LOGD("REPLACE TYPE ===> END");
                } else if (IsDebug){
                    LOGD("REPLACE TYPE ===> MIDDLE");
                }
                if (IsDebug) LOGE("---> called get_text replace '%s' to '%s'   times:%d",left,right,current_set_index);
                if (IsDebug) LOGD("Original str hex at %p === >",&middle_get);
                hexDump(reinterpret_cast<const char *>(arg1), src_length + HeaderSize +EndSize);
                void *p_new_u_str = calloc(MiddleSize * 2, sizeof(char));
                int length_right = UTF8_to_Unicode(static_cast<char *>(p_new_u_str), right);
                if (IsDebug) LOGD("Replacement str hex at %p === >",&length_right);
                hexDump(reinterpret_cast<const char *>(p_new_u_str), length_right);
                //原动态部分大小
                int length_start = (char*)cp_bit - (char*)middle_get;
                int length_end = src_length - length_start - length_left;
                int length_middle = length_start + length_right + length_end;
                //申请空间来重新组合返回值
                void *p_return = calloc(static_cast<size_t>(HeaderSize + length_middle + EndSize), sizeof(char));
                //重写字符串大小
                *((int*)header_get + 2) = length_middle / 2;
                memcpy(p_return, header_get, HeaderSize);
                memcpy((char *) p_return + HeaderSize, (char*)middle_get, length_start);
                memcpy((char *) p_return + HeaderSize + length_start, p_new_u_str , length_right);
                memcpy((char *) p_return + HeaderSize + length_start + length_right, (char*)cp_bit+length_left , length_end);
                memcpy((char *) p_return + HeaderSize + length_start + length_right + length_end, end, EndSize);
                if (IsDebug) LOGD("Return str hex at %p === >",&p_return);
                hexDump(static_cast<const char *>(p_return), static_cast<size_t>(HeaderSize + length_middle + EndSize));
                free(convert_str);
                free(left);
                free(right);
                free(temp_buffer);
                free(p_new_u_str);
                return p_return;
            }
        }
        p = strtok(NULL, "\r\n");
        current_lines++;
    }
    free(left);
    free(right);
    free(temp_buffer);
    return old_func_get(arg, arg1, arg2, arg3);
}

void *new_func_get_methods(void *arg, void *arg1){
    void *ret = old_func_get_methods(arg,arg1);
    if (ret == 0) return ret;
    void *p_Kclass = (void*)*((int*)ret + 3);
    char *p_MethodName = (char *)*((int*)ret + 2);
    char *p_NameSpaze = (char *)*((int*)p_Kclass + 3);
//    if (IsDebug) LOGD("p_MethodName = %s   p_NameSpaze = %s", p_MethodName ,p_NameSpaze);
    if (strcmp(p_NameSpaze, "UnityEngine.UI") == 0){
        if(strcmp("get_text",p_MethodName) == 0) address_get = *((int*)ret);
        if(strcmp("set_text",p_MethodName) == 0) address_set = *((int*)ret);
        if(address_get!=0&&address_set!=0&&is_got_get_and_set==0){
            if (IsDebug) LOGE("FOUND get_text addr at %p and set_text addr at %p",address_get,address_set);
            address_get -= libil2cpp_base;
            address_set -= libil2cpp_base;
            hook_get_set();
            is_got_get_and_set = 1;
        }
    }
    return ret;
}
