#ifndef GITEN_MEM_HANDLE_H
#define GITEN_MEM_HANDLE_H

#include <EnumDomain.h>
#include <Ints.h>

// Numbered memory blocks: handle 0 is "none"; 1..1023 index the table.
#define HANDLE_COUNT 1024

GZ_ENUM_FLAGS_BEGIN(HandleFlags, u16)
    HANDLE_IN_USE = 1
GZ_ENUM_FLAGS_END(HandleFlags)

typedef struct HandleEntry {
    GZ_ENUM_STORAGE(HandleFlags, u16) flags;
    u16 size;
    void* ptr;
} HandleEntry;

// Doubly linked lists whose nodes are handles; each node block starts with
// its links.
typedef struct ListLink {
    i32 next;
    i32 prev;
} ListLink;

void ClearHandleTable(void);
i32 SetHandleEntry(i32 handle, void* ptr, u16 size, u16 flags);
u32 HandleSize(i32 handle);
u32 GetHandleSize(i32 handle);
i32 NewHandle(void* ptr, u32 size);
i16 IsHandleUsed(i32 handle);
i32 NewArrayHandle(void* ptr, u16 count, u16 size);
i32 AllocHandle(u32 size);
i32 AllocArrayHandle(u16 count, u16 size);
i32 CreateArrayHandle(u16 count, u16 size);
i32 NewBlockHandle(u16 size);
i32 NewStringHandle(const char* text);
i32 ResizeHandle(i32 handle, u32 size);
b32 FreeHandle(i32 handle);
void* HandlePtr(i32 handle);
i32 SetHandlePtr(i32 handle, void* ptr, u16 size);
b32 ClearHandle(i32 handle);
void* HandleWritePtr(i32 handle);
void* HandleReadPtr(i32 handle);
void* HandleAccessPtr(i32 handle);

i32 GetListTail(i32 list);
i32 AppendList(i32 list, i32 node);
i32 PopListTail(i32 list, i32* node);
i32 HandleListTail(i32 node);
i32 HandleListAppend(i32 list, i32 node);
i32 HandleListPopTail(i32 list, i32* node);
ListLink* HandleListNodeForRead(i32 node);
ListLink* HandleListNodeForWrite(i32 node);
i32 HandleListHead(i32 node);
i32 HandleListUnlink(i32 node);
i32 HandleListInsertAfter(i32 pos, i32 node);
i32 HandleListNext(i32 node);
i32 HandleListPrev(i32 node);

#endif // GITEN_MEM_HANDLE_H
