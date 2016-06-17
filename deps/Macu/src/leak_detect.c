#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "leak_detect.h"
#include <string.h>
#include <stdio.h>

#undef malloc
#undef calloc
#undef realloc
#undef free

#define PRINT_LEAK_STR "\
-----------------------------\n\
-        Memory Leak        -\n\
-----------------------------\n\
File: %s\n\
Line: %u\n\
Size: %u bytes\n\
Address: %#x\n\
-----------------------------\n"

struct ld_mem_info {
    void* addr;
    size_t size;
    char filename[_MAX_PATH];
    unsigned int line;
};

struct leak_list {
    struct ld_mem_info* info;
    struct leak_list* next;
};

struct leak_detector {
    struct leak_list* leak_list;
    size_t size;
};

/* Leak detection context data */
static struct leak_detector detector;

void ldinit()
{
    detector.leak_list = 0;
    detector.size = 0;
}

void ldshutdown()
{
    struct leak_list* temp = detector.leak_list, *temp2;

    /* Free allocated memory */
    while(temp != 0) {
        /* Save the next node for next loop */
        temp2 = temp->next;

        free(temp->info->addr);
        free(temp->info);
        free(temp);
        (detector.size)--;

        temp = temp2;
    }
}

void add_leak(const struct ld_mem_info* info)
{
    struct ld_mem_info *info_to_add = malloc(sizeof(struct ld_mem_info));
    struct leak_list *leak_to_add = malloc(sizeof(struct leak_list));
    struct leak_list *temp;

    /* Config info */
    info_to_add->addr = info->addr;
    info_to_add->size = info->size;
    strcpy(info_to_add->filename, info->filename);
    info_to_add->line = info->line;

    /* Config leak */
    leak_to_add->info = info_to_add;
    leak_to_add->next = 0;

    /* List is empty */
    if(detector.leak_list == 0)
        detector.leak_list = leak_to_add;
    else {
        /* Skip everything and go to last element of list */
        for(temp = detector.leak_list; temp != 0; temp = temp->next)
            if(temp->next == 0)
                break;

        temp->next = leak_to_add;
    }

    (detector.size)++;
}

void* ldmalloc(size_t size, const char * file, unsigned int line)
{
    struct ld_mem_info mem_info;
    mem_info.addr = malloc(size);
    mem_info.size = size;
    strcpy(mem_info.filename, file);
    mem_info.line = line;
    add_leak(&mem_info);
    return mem_info.addr;
}

void* ldcalloc(size_t num, size_t size, const char* file, unsigned int line)
{
    struct ld_mem_info mem_info;
    mem_info.addr = calloc(num, size);
    mem_info.size = num * size;
    strcpy(mem_info.filename, file);
    mem_info.line = line;
    add_leak(&mem_info);
    return mem_info.addr;
}

void* ldrealloc(void* ptr, size_t size, const char* file, unsigned int line)
{
    struct leak_list *cur = detector.leak_list;
    while(cur != 0) {
        /* Found a match! */
        if(cur->info->addr == ptr) {
            cur->info->addr = realloc(ptr, size);
            cur->info->size = size;
            strcpy(cur->info->filename, file);
            cur->info->line = line;
            break;
        }
        cur = cur->next;
    }
    return (cur == 0) ? 0 : cur->info->addr;
}

void ldfree(void* addr)
{
    struct leak_list *cur, *prev, *temp;

    /* Check if the first element is the one we need */
    if(detector.leak_list->info->addr == addr) {
        temp = detector.leak_list->next;
        free(detector.leak_list->info->addr);
        free(detector.leak_list->info);
        free(detector.leak_list);
        detector.leak_list = temp;
    }
    else {
        /* Find the right leak in the list to remove */
        cur = detector.leak_list;
        prev = cur;
        while(cur != 0) {
            /* Found a match! */
            if(cur->info->addr == addr) {
                prev->next = cur->next;
                free(cur->info->addr);
                free(cur->info);
                free(cur);
                cur = 0;
                break;
            }

            prev = cur;
            cur = cur->next;
        }
    }

    (detector.size)--;
}

void print_leaks()
{
    struct leak_list* temp = detector.leak_list;
    while(temp != 0) {
        printf(PRINT_LEAK_STR,
               temp->info->filename,
               temp->info->line,
               temp->info->size,
               (unsigned int)temp->info->addr);

        temp = temp->next;
    }
}
