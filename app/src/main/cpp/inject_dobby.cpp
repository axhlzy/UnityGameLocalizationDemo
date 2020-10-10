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

#define LOG_TAG "ZZZ"
//拆分左右两个字符串的最大长度
#define SplitSize 100
#define HeaderSize 12
#define MiddleSize 200
#define EndSize 4
#define LOGD(fmt, args...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG,fmt, ##args)
#define LOGE(fmt, args...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG,fmt, ##args)

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
static char *lib_name = const_cast<char *>("libil2cpp.so");

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
char *getPackageName(JNIEnv *env);
jobject getApplication(JNIEnv *env);
int readFile(JNIEnv *pEnv);
void hook_get_set();
void hook_dlopen();
void hook_get_methods();
void init_address_from_file();
uint32_t UTF8_to_Unicode(char *dst, char *src);
void hexDump(const char *buf, int len);
unsigned long getCurrentTime();
unsigned long find_module_by_name(char *soName);


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
    LOGE("------------------- JNI_OnLoad -------------------");
    if (vm->GetEnv((void **) &env, JNI_VERSION_1_6) == JNI_OK) {
        LOGD("GetEnv OK");
    }
    if (vm->AttachCurrentThread(&env, NULL) == JNI_OK) {
        LOGE("Called AttachCurrentThread OK");
    }
    if (readFile(env) != -1) {
        init_address_from_file();
        hook_dlopen();
    }
    LOGE("-------------------  Function  -------------------");
    return JNI_VERSION_1_6;
}

void hook_dlopen() {
    func_dlopen = reinterpret_cast<unsigned int>((m_dlopen) dlopen);
    func_dlsym = reinterpret_cast<unsigned int>((m_dlsym) dlsym);
    LOGE("------------------- hook_dlopen -------------------");
    LOGD("func_dlopen = 0x%x   -----  func_dlsym = 0x%x ", func_dlopen, func_dlsym);
    DobbyHook((void *)func_dlopen, (void *) new_func_dlopen,(void **) &old_func_dlopen);
//    DobbyHook((void *)func_dlsym, (void *) new_func_dlsym,(void **) &old_fun_dlsym);
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

/**
 * 读取文件首行用作get_text/set_text的地址
 */
void init_address_from_file() {
    int current_lines = 0;
    char *left = static_cast<char *>(calloc(50, sizeof(char)));
    char *right = static_cast<char *>(calloc(50, sizeof(char)));
    char *temp_buffer = (char *) malloc(sizeof(char) * file_size + sizeof(int));
    memcpy(temp_buffer, buffer, sizeof(char) * file_size + sizeof(int));
    char *p = strtok(temp_buffer, "\r\n");
    while (p != NULL && current_lines == 0) {
        memset(left, 0, 20);
        memset(right, 0, 20);
        char *s = strstr(p, "|");
        static_cast<char *>(memcpy(left, p, strlen(p) - strlen(s)));
        right = strcpy(right, s + sizeof(char));
        //第一行的两个参数用来指定get set地址
        address_get = strtol(left, NULL, 16);
        address_set = strtol(right, NULL, 16);
        LOGD("get first line : left:%p   right:%p", address_get,address_set);
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

    pFile = fopen(path, "rb");
    if (pFile == NULL) {
        LOGE ("File error path = %s",path);
        return -1;
    }

    fseek(pFile, 0, SEEK_END);
    file_size = ftell(pFile);
    LOGE ("File size = %d",file_size);
    rewind(pFile);

    buffer = (char *) malloc(sizeof(char) * file_size + sizeof(int));
    memset(buffer, 0, sizeof(char) * file_size + sizeof(int));
    if (buffer == NULL) {
        LOGD ("Memory error");
        return -1;
    }

    result = fread(buffer, 1, file_size, pFile);
    if (result != file_size) {
        LOGD ("Reading error");
        return -1;
    }
    LOGD("readFile from %s \n%s", path , buffer);

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
        LOGE("libil2cpp_base= %p address_set= %p address_get= %p ",libil2cpp_base,address_set,address_get);
        return;
    }

    if (address_get != 0){
        func_get = libil2cpp_base + address_get;
        DobbyHook((void *)func_get, (void *)new_func_get,(void **)&old_func_get) == RS_SUCCESS ?
        LOGD("Success Hook func_get at 0x%x",func_get):LOGE("Fail Hook func_get at 0x%x",func_get);
    }

    if (address_set != 0){
        func_set = libil2cpp_base + address_set;
        DobbyHook((void *)func_set, (void *)new_func_set,(void **)&old_func_set) == RS_SUCCESS ?
        LOGD("Success Hook func_set at 0x%x",func_set):LOGE("Fail Hook func_set at 0x%x",func_set);
    }
}

/**
 * hook il2cpp_class_get_methods函数，遍历拿到get/set的地址
 */
void hook_get_methods() {
    //il2cpp_class_get_methods_0 而不是 il2cpp_class_get_methods （只有一行inlinehook最少两行汇编指令）
    void *func_get_methods = dlsym(libil2cpp_handle, "il2cpp_class_get_methods");
    LOGE("call hook_get_methods old at %x \t instruction = %x",(int)func_get_methods,*(int*)(func_get_methods));
    //跳转指令偏移计算
    int new_p = (((*(int *) (func_get_methods)) << 8 >> 8)*4 + 8) + (int)func_get_methods;
    void *tmp_p = reinterpret_cast<void *>(new_p);
    LOGE("call hook_get_methods new at %x \t instruction = %x",(int)tmp_p,*(int*)(tmp_p));
    DobbyHook(tmp_p, (void *)new_func_get_methods, (void **)&old_func_get_methods) == RS_SUCCESS ?
    LOGD("Success Hook func_get_methods at 0x%x",(int)tmp_p) :LOGE("Fail Hook func_get_methods at 0x%x",(int)tmp_p);
}

/**
 * hook dlopen函数，找到libil2cpp.so的加载时机
 */
void *new_func_dlopen(const char *filename, int flags, const void *caller_addr) {
    void *ret = old_func_dlopen(filename, flags, caller_addr);
    LOGD("%p=__loader_dlopen('%s','%d','%p')",ret, filename, flags, caller_addr);
    if (strstr(filename, "libil2cpp") != NULL && caller_addr !=0) {
        //此刻libil2cpp已经加载进去，我们拿到handle以及base存在全局
        libil2cpp_handle = ret;
        libil2cpp_base = find_module_by_name(lib_name);
        LOGE("Find %s at 0x%x", lib_name, libil2cpp_base);
        //首行填写了地址就直接用，首行没填写地址的情况，咋们动态去获取
        if (address_get == 0 && address_set == 0){
            hook_get_methods();
        }else{
            hook_get_set();
        }
    }
    return old_func_dlopen(filename, flags, caller_addr);
}

void *new_func_dlsym(void *handle, const char *symbol) {
    void *ret = old_fun_dlsym(handle, symbol);
    LOGD("ret %p = __loader_dlsym('%p','%s')", ret, handle, symbol);
    return old_fun_dlsym(handle, symbol);
}

void *new_func_set(void *arg, void *arg1, void *arg2, void *arg3) {
    current_set_index++;
    if (current_set_index > 700){
        LOGD("Enter new_func_set %d",current_set_index);
        //set的时候第二个参数可能为0，就像get的时候返回值可能为0一样
        //有可能没有值，后面就会以八个零结束会出错（返回值偏移12位如果为0则直接返回）
        if (arg1 == 0 || *((char *) arg1 + sizeof(char) * 12) == 0){
            LOGE("1");
            return old_func_set(arg, arg1, arg2, arg3);
        }
        memset(header_set, 0, HeaderSize);
        memcpy(header_set, arg1, HeaderSize);
        memset(middle_set, 0, SplitSize);
        LOGE("2");
        //以八个0作为结束，拷贝以返回值偏移12个字节的作为开始的内存数据，其实就是中间文字部分
        memccpy(middle_set, (char *) arg1 + sizeof(char) * HeaderSize, reinterpret_cast<int>(end), SplitSize);
        void* p_le =memchr(middle_set,reinterpret_cast<int>(end),SplitSize);
        //原返回值中间文字部分的长度
        int src_length = (char*)p_le - (char*)middle_set;
        LOGD("Length %d  %d",src_length);
        int current_lines = 0;
        //初始化解析文本以“|”作为分割左边 右边部分缓存指针
        char *left = static_cast<char *>(calloc(SplitSize, sizeof(char)));
        char *right = static_cast<char *>(calloc(SplitSize, sizeof(char)));
        LOGE("3");
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
                LOGE("4");
                char *convert_str = static_cast<char *>(calloc(SplitSize * 2, sizeof(char)));
                int length = UTF8_to_Unicode(convert_str, left);
                //内存字节的比较
                if (memcmp(middle_set, convert_str, length) == 0) {
//                if (memcmp((char *) arg1 + sizeof(char) * HeaderSize, convert_str,static_cast<size_t>(length)) == 0) {
                    LOGE("---> called set_text replace %s to %s   times:%d",left,right,current_set_index);
                    LOGD("Original str hex at %p === >",&middle_set);
                    hexDump(reinterpret_cast<const char *>(middle_set), length*2);
                    void *p1 = calloc(SplitSize * 2, sizeof(char));
                    int le = UTF8_to_Unicode(static_cast<char *>(p1), right);
                    LOGD("Replacement str hex at %p === >",&le);
                    hexDump(reinterpret_cast<const char *>(p1), le);
                    //申请空间来重新组合返回值
                    void *temp = malloc(static_cast<size_t>(HeaderSize + le + EndSize));
                    memset(temp, 0, static_cast<size_t>(HeaderSize + le + EndSize));
                    memcpy(temp, header_set, HeaderSize);
                    memcpy((char *) temp + HeaderSize, p1, static_cast<size_t>(le));
                    memcpy((char *) temp + HeaderSize + le, end, EndSize);
                    LOGD("Return str hex at %p === >",&temp);
                    hexDump(static_cast<const char *>(temp), static_cast<size_t>(HeaderSize + le + EndSize));
                    free(convert_str);
                    free(left);
                    free(right);
                    free(temp_buffer);
                    free(p1);
                    return old_func_set(arg, temp, arg2, arg3);
                }
            }
            p = strtok(NULL, "\r\n");
            current_lines++;
        }
        free(left);
        free(right);
        free(temp_buffer);
        return old_func_set(arg, arg1, arg2, arg3);
    }else{
        return old_func_set(arg, arg1, arg2, arg3);
    }
//    try {
//
//    }catch (std::exception){
//        LOGE("Catch exception");
//        return old_func_set(arg, arg1, arg2, arg3);
//    }
}

void *new_func_get(void *arg, void *arg1, void *arg2, void *arg3) {
    current_get_index++;
    LOGD("Enter new_func_get %d",current_get_index);
    void *ret = old_func_get(arg, arg1, arg2, arg3);
    if (ret == 0 || arg1 == 0) return ret;
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
//    void* p_le =memchr(middle_get,reinterpret_cast<int>(end),SplitSize);
//    int src_length = (char*)p_le - (char*)middle_get;
//    LOGD("SRC_LENGTH = %d",src_length);
    char *p = strtok(temp_buffer, "\r\n");
    while (p != NULL) {
        memset(left, 0, SplitSize);
        memset(right, 0, SplitSize);
        char *s = strstr(p, "|");
        static_cast<char *>(memcpy(left, p, strlen(p) - strlen(s)));
        right = strcpy(right, s + sizeof(char));
        if (current_lines != 0) {
            char *convert_str = static_cast<char *>(calloc(SplitSize * 2, sizeof(char)));
            memset(convert_str, 0, strlen(left) * 2);
            int length = UTF8_to_Unicode(convert_str, left);
            if (memcmp(middle_get, convert_str, length) == 0) {
                LOGE("---> called get_text replace %s to %s   times:%d",left,right,current_set_index);
                LOGD("Original str hex at %p === >",&middle_set);
                hexDump(reinterpret_cast<const char *>(middle_set), length);
                void *p1 = calloc(SplitSize * 2, sizeof(char));
                int le = UTF8_to_Unicode(static_cast<char *>(p1), right);
                LOGD("Replacement str hex at %p === >",&le);
                hexDump(reinterpret_cast<const char *>(p1), le);
                void *temp = malloc(static_cast<size_t>(HeaderSize + le + EndSize));
                memset(temp, 0, static_cast<size_t>(HeaderSize + le + EndSize));
                memcpy(temp, header_get, HeaderSize);
                memcpy((char *) temp + HeaderSize, p1, static_cast<size_t>(le));
                memcpy((char *) temp + HeaderSize + le, end, EndSize);
                LOGD("Return str hex at %p === >",&temp);
                hexDump(static_cast<const char *>(temp), static_cast<size_t>(HeaderSize + le + EndSize));
                free(left);
                free(right);
                free(convert_str);
                free(temp_buffer);
                free(p1);
                return old_func_get(arg, arg1, arg2, temp);
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
//    LOGD("p_MethodName = %s   p_NameSpaze = %s", p_MethodName ,p_NameSpaze);
    if (strcmp(p_NameSpaze, "UnityEngine.UI") == 0){
        if(strcmp("get_text",p_MethodName) == 0) address_get = *((int*)ret);
        if(strcmp("set_text",p_MethodName) == 0) address_set = *((int*)ret);
        if(address_get!=0&&address_set!=0&&is_got_get_and_set==0){
            LOGE("FOUND get_text addr at %p and set_text addr at %p",address_get,address_set);
            address_get -= libil2cpp_base;
            address_set -= libil2cpp_base;
            hook_get_set();
            is_got_get_and_set = 1;
        }
    }
    return ret;
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
        if (unicode < 0x80) {
            if (i == 0 && unicode == 0x20) {
                continue;
            }
        }
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
