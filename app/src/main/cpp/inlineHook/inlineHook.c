/*
thumb16 thumb32 arm32 inlineHook
author: ele7enxxh
mail: ele7enxxh@qq.com
website: ele7enxxh.com
modified time: 2015-01-23
created time: 2015-11-30

 相关介绍
 http://ele7enxxh.com/Android-Arm-Inline-Hook.html
 https://www.sunmoonblog.com/2019/07/15/inline-hook-basic/
 https://gtoad.github.io/2018/07/13/Android-Inline-Hook-Fix/
 https://gtoad.github.io/2018/07/06/Android-Native-Hook-Practice/
 Demo项目
 https://github.com/zhuotong/Android_InlineHook


*/

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <dirent.h>
#include <signal.h>
#include <sys/mman.h>
// #include <asm/ptrace.h>
#include <sys/wait.h>
#include <sys/ptrace.h>

#include "relocate.h"
#include "include/inlineHook.h"

#ifndef PAGE_SIZE
#define PAGE_SIZE 4096
#endif

#define PAGE_START(addr)	(~(PAGE_SIZE - 1) & (addr))
// 设置bit[0]的值为1
#define SET_BIT0(addr)		(addr | 1)
// 设置bit[0]的值为0
#define CLEAR_BIT0(addr)	(addr & 0xFFFFFFFE)
//测试bit[0]的值，若为1则返回真，若为0则返回假
#define TEST_BIT0(addr)		(addr & 1)

#define ACTION_ENABLE	0
#define ACTION_DISABLE	1

/*
 * Arm处理器采用3级流水线来增加处理器指令流的速度，也就是说程序计数器R15(PC)总是指向“正在取指”的指令，
 * 而不是指向“正在执行”的，即PC总是指向当前正在执行的指令地址再加2条指令的地址
 *
 * Arm的B系列指令跳转范围只有4M，Thumb的B系列指令跳转范围只有256字节，
 * 然而大多数情况下跳转范围都会大于4M，故我们采用LDR PC, [PC, ?]构造跳转指令
 *
 *
 */
	
enum hook_status {
	REGISTERED,
	HOOKED,
};

struct inlineHookItem {
	uint32_t target_addr;
	uint32_t new_addr;
	uint32_t **proto_addr;
	void *orig_instructions;
	int orig_boundaries[4];
	int trampoline_boundaries[20];
	int count;
	void *trampoline_instructions;
	int length;
	int status;
	int mode;
};

struct inlineHookInfo {
	struct inlineHookItem item[1024];
	int size;
};

static struct inlineHookInfo info = {0};

static int getAllTids(pid_t exclude_tid, pid_t *tids)
{
	char dir_path[32];
	DIR *dir;
	int i;
	struct dirent *entry;
	pid_t tid;

	if (exclude_tid < 0) {
		snprintf(dir_path, sizeof(dir_path), "/proc/self/task");
	}
	else {
		snprintf(dir_path, sizeof(dir_path), "/proc/%d/task", exclude_tid);
	}

	dir = opendir(dir_path);
    if (dir == NULL) {
    	return 0;
    }

    i = 0;
    while((entry = readdir(dir)) != NULL) {
    	tid = atoi(entry->d_name);
    	if (tid != 0 && tid != exclude_tid) {
    		tids[i++] = tid;
    	}
    }
    closedir(dir);
    return i;
}

static bool doProcessThreadPC(struct inlineHookItem *item, struct pt_regs *regs, int action)
{
	int offset;
	int i;

	switch (action)
	{
		case ACTION_ENABLE:
			offset = regs->ARM_pc - CLEAR_BIT0(item->target_addr);
			for (i = 0; i < item->count; ++i) {
				if (offset == item->orig_boundaries[i]) {
					regs->ARM_pc = (uint32_t) item->trampoline_instructions + item->trampoline_boundaries[i];
					return true;
				}
			}
			break;
		case ACTION_DISABLE:
			offset = regs->ARM_pc - (int) item->trampoline_instructions;
			for (i = 0; i < item->count; ++i) {
				if (offset == item->trampoline_boundaries[i]) {
					regs->ARM_pc = CLEAR_BIT0(item->target_addr) + item->orig_boundaries[i];
					return true;
				}
			}
			break;
	}

	return false;
}

static void processThreadPC(pid_t tid, struct inlineHookItem *item, int action)
{
	struct pt_regs regs;

	if (ptrace(PTRACE_GETREGS, tid, NULL, &regs) == 0) {
		if (item == NULL) {
			int pos;

			for (pos = 0; pos < info.size; ++pos) {
				if (doProcessThreadPC(&info.item[pos], &regs, action) == true) {
					break;
				}
			}
		}
		else {
			doProcessThreadPC(item, &regs, action);
		}

		ptrace(PTRACE_SETREGS, tid, NULL, &regs);
	}
}

static pid_t freeze(struct inlineHookItem *item, int action)
{
	int count;
	pid_t tids[1024];
	pid_t pid;

	pid = -1;
	count = getAllTids(gettid(), tids);
	if (count > 0) {
		pid = fork();

		if (pid == 0) {
			int i;

			for (i = 0; i < count; ++i) {
				if (ptrace(PTRACE_ATTACH, tids[i], NULL, NULL) == 0) {
					waitpid(tids[i], NULL, WUNTRACED);
					processThreadPC(tids[i], item, action);
				}
			}
			
			raise(SIGSTOP);

			for (i = 0; i < count; ++i) {
				ptrace(PTRACE_DETACH, tids[i], NULL, NULL);
			}

			raise(SIGKILL);
		}

		else if (pid > 0) {
			waitpid(pid, NULL, WUNTRACED);
		}
	}

	return pid;
}

static void unFreeze(pid_t pid)
{
	if (pid < 0) {
		return;
	}

	kill(pid, SIGCONT);
	wait(NULL);
}

static bool isExecutableAddr(uint32_t addr)
{
	FILE *fp;
	char line[1024];
	uint32_t start;
	uint32_t end;

	fp = fopen("/proc/self/maps", "r");
	if (fp == NULL) {
		return false;
	}

	while (fgets(line, sizeof(line), fp)) {
		if (strstr(line, "r-xp") || strstr(line, "rwxp")) {
			start = strtoul(strtok(line, "-"), NULL, 16);
			end = strtoul(strtok(NULL, " "), NULL, 16);
			if (addr >= start && addr <= end) {
				fclose(fp);
				return true;
			}
		}
	}

	fclose(fp);

	return false;
}

static struct inlineHookItem *findInlineHookItem(uint32_t target_addr)
{
	int i;

	for (i = 0; i < info.size; ++i) {
		if (info.item[i].target_addr == target_addr) {
			return &info.item[i];
		}
	}

	return NULL;
}

static struct inlineHookItem *addInlineHookItem() {
	struct inlineHookItem *item;

	if (info.size >= 1024) {
		return NULL;
	}

	item = &info.item[info.size];
	++info.size;

	return item;
}

static void deleteInlineHookItem(int pos)
{
	info.item[pos] = info.item[info.size - 1];
	--info.size;
}

enum ele7en_status registerInlineHook(uint32_t target_addr, uint32_t new_addr, uint32_t **proto_addr)
{
	struct inlineHookItem *item;

	if (!isExecutableAddr(target_addr) || !isExecutableAddr(new_addr)) {
		return ELE7EN_ERROR_NOT_EXECUTABLE;
	}

	//调用 findInlineHookItem() 方法检查是否已有相关记录。有则直接返回，否则进入下一步
	item = findInlineHookItem(target_addr);
	if (item != NULL) {
		if (item->status == REGISTERED) {
			return ELE7EN_ERROR_ALREADY_REGISTERED;
		}
		else if (item->status == HOOKED) {
			return ELE7EN_ERROR_ALREADY_HOOKED;
		}
		else {
			return ELE7EN_ERROR_UNKNOWN;
		}
	}

	//调用 addInlineHookItem() 方法为指定的 target_addr 来创建一个新的 inlineHookItem
	item = addInlineHookItem();

	//初始化创建的 inlineHookItem
	item->target_addr = target_addr;
	item->new_addr = new_addr;
	item->proto_addr = proto_addr;

	item->length = TEST_BIT0(item->target_addr) ? 12 : 8;
	item->orig_instructions = malloc(item->length);
	memcpy(item->orig_instructions, (void *) CLEAR_BIT0(item->target_addr), item->length);

	item->trampoline_instructions = mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_ANONYMOUS | MAP_PRIVATE, 0, 0);
	relocateInstruction(item->target_addr, item->orig_instructions, item->length, item->trampoline_instructions, item->orig_boundaries, item->trampoline_boundaries, &item->count);

	item->status = REGISTERED;

	return ELE7EN_OK;
}

static void doInlineUnHook(struct inlineHookItem *item, int pos)
{
	mprotect((void *) PAGE_START(CLEAR_BIT0(item->target_addr)), PAGE_SIZE * 2, PROT_READ | PROT_WRITE | PROT_EXEC);
	memcpy((void *) CLEAR_BIT0(item->target_addr), item->orig_instructions, item->length);
	mprotect((void *) PAGE_START(CLEAR_BIT0(item->target_addr)), PAGE_SIZE * 2, PROT_READ | PROT_EXEC);
	munmap(item->trampoline_instructions, PAGE_SIZE);
	free(item->orig_instructions);

	deleteInlineHookItem(pos);

	cacheflush(CLEAR_BIT0(item->target_addr), CLEAR_BIT0(item->target_addr) + item->length, 0);
}

enum ele7en_status inlineUnHook(uint32_t target_addr)
{
	int i;

	for (i = 0; i < info.size; ++i) {
		if (info.item[i].target_addr == target_addr && info.item[i].status == HOOKED) {
			pid_t pid;

			pid = freeze(&info.item[i], ACTION_DISABLE);

			doInlineUnHook(&info.item[i], i);

			unFreeze(pid);

			return ELE7EN_OK;
		}
	}

	return ELE7EN_ERROR_NOT_HOOKED;
}

void inlineUnHookAll()
{
	pid_t pid;
	int i;

	pid = freeze(NULL, ACTION_DISABLE);

	for (i = 0; i < info.size; ++i) {
		if (info.item[i].status == HOOKED) {
			doInlineUnHook(&info.item[i], i);
			--i;
		}
	}

	unFreeze(pid);
}

/**
 *  1.处理对原始方法的访问 - 将 trampoline_instructions 强制转型为 void *proto_addr
 *    如果想调用原始方法，调用 proto_addr 指向的方法即可
 * 	2.修改原始方法，写入跳转指令
 */
static void doInlineHook(struct inlineHookItem *item)
{
	mprotect((void *) PAGE_START(CLEAR_BIT0(item->target_addr)), PAGE_SIZE * 2, PROT_READ | PROT_WRITE | PROT_EXEC);

	if (item->proto_addr != NULL) {
		*(item->proto_addr) = TEST_BIT0(item->target_addr) ? (uint32_t *) SET_BIT0((uint32_t) item->trampoline_instructions) : item->trampoline_instructions;
	}

	//Arm Mode
	if (TEST_BIT0(item->target_addr)) {
		int i;

		i = 0;
		if (CLEAR_BIT0(item->target_addr) % 4 != 0) {
			((uint16_t *) CLEAR_BIT0(item->target_addr))[i++] = 0xBF00;  // NOP
		}
		((uint16_t *) CLEAR_BIT0(item->target_addr))[i++] = 0xF8DF;
		((uint16_t *) CLEAR_BIT0(item->target_addr))[i++] = 0xF000;	// LDR.W PC, [PC]
		((uint16_t *) CLEAR_BIT0(item->target_addr))[i++] = item->new_addr & 0xFFFF;
		((uint16_t *) CLEAR_BIT0(item->target_addr))[i++] = item->new_addr >> 16;
	}
	//Thumb Mode
	else {
		((uint32_t *) (item->target_addr))[0] = 0xe51ff004;	// LDR PC, [PC, #-4]
		((uint32_t *) (item->target_addr))[1] = item->new_addr;
	}

	mprotect((void *) PAGE_START(CLEAR_BIT0(item->target_addr)), PAGE_SIZE * 2, PROT_READ | PROT_EXEC);

	item->status = HOOKED;
	
	cacheflush(CLEAR_BIT0(item->target_addr), CLEAR_BIT0(item->target_addr) + item->length, 0);
}

enum ele7en_status inlineHook(uint32_t target_addr)
{
	int i;
	struct inlineHookItem *item;

	item = NULL;
	for (i = 0; i < info.size; ++i) {
		if (info.item[i].target_addr == target_addr) {
			item = &info.item[i];
			break;
		}
	}

	if (item == NULL) {
		return ELE7EN_ERROR_NOT_REGISTERED;
	}

	if (item->status == REGISTERED) {
		pid_t pid;

		pid = freeze(item, ACTION_ENABLE);

		doInlineHook(item);

		unFreeze(pid);

		return ELE7EN_OK;
	}
	else if (item->status == HOOKED) {
		return ELE7EN_ERROR_ALREADY_HOOKED;
	}
	else {
		return ELE7EN_ERROR_UNKNOWN;
	}
}

void inlineHookAll()
{
	pid_t pid;
	int i;

	pid = freeze(NULL, ACTION_ENABLE);

	for (i = 0; i < info.size; ++i) {
		if (info.item[i].status == REGISTERED) {
			doInlineHook(&info.item[i]);
		}
	}

	unFreeze(pid);
}
