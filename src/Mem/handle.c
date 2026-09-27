// @identity-TODO: the owning TU is unproven; this unit holds one contiguous
// retail span until link-order evidence names it.

#include <rva.h>

#include <Mem/Alloc.h>
#include <Mem/Handle.h>

#include <string.h>

DATA(0x00076058)
static HandleEntry s_handles[HANDLE_COUNT];

static __inline HandleEntry* GetHandleEntry(i32 handle) {
    return &s_handles[handle];
}

RVA(0x000043d0, 0x1f)
void ClearHandleTable(void) {
    i16 handle;
    for (handle = 0; handle < HANDLE_COUNT; handle++) {
        SetHandleEntry(handle, 0, 0, 0);
    }
}

RVA(0x000043f0, 0x2a)
i32 SetHandleEntry(i32 handle, void* ptr, u16 size, u16 flags) {
    GetHandleEntry(handle)->ptr = ptr;
    GetHandleEntry(handle)->size = size;
    GetHandleEntry(handle)->flags = flags;
    return handle;
}

// @dead-code
// Zero-ref: no rel32 caller, data slot or address-taking (giten sema xref --tree).
RVA(0x00004420, 0xe)
u32 HandleSize(i32 handle) {
    return GetHandleSize(handle);
}

// @early-stop: retail widens the loaded size in place (mov ax; and eax,0xffff)
// where this build zeroes first; return-type and local spellings are flat.
RVA(0x00004430, 0x12)
u32 GetHandleSize(i32 handle) {
    return GetHandleEntry(handle)->size;
}

RVA(0x00004450, 0x1c)
i32 AllocHandle(u32 size) {
    void* block = AllocCleared(1, size);
    return NewHandle(block, size);
}

RVA(0x00004470, 0x3d)
i32 NewHandle(void* ptr, u32 size) {
    i16 handle;
    for (handle = 1; handle < HANDLE_COUNT; handle++) {
        if (!IsHandleUsed(handle)) {
            return SetHandleEntry(handle, ptr, size, HANDLE_IN_USE);
        }
    }
    return 0;
}

RVA(0x000044b0, 0xf)
i16 IsHandleUsed(i32 handle) {
    return GetHandleEntry(handle)->flags & HANDLE_IN_USE;
}

RVA(0x000044c0, 0x22)
i32 AllocArrayHandle(u16 count, u16 size) {
    void* block = AllocCleared(count, size);
    return NewArrayHandle(block, count, size);
}

RVA(0x000044f0, 0x25)
i32 NewArrayHandle(void* ptr, u16 count, u16 size) {
    return NewHandle(ptr, count * size);
}

RVA(0x00004520, 0x13)
i32 CreateArrayHandle(u16 count, u16 size) {
    return AllocArrayHandle(count, size);
}

// A zero-filled block of `size` bytes under a new handle.
RVA(0x00004540, 0x13)
i32 NewBlockHandle(u16 size) {
    return AllocHandle(size);
}

// @dead-code
// Zero-ref: no rel32 caller, data slot or address-taking (giten sema xref --tree).
RVA(0x00004560, 0x50)
i32 NewStringHandle(const char* text) {
    u32 size = strlen(text) + 1;
    char* copy = AllocCleared(1, size);
    strcpy(copy, text);
    return NewHandle(copy, size);
}

RVA(0x000045b0, 0x3e)
i32 ResizeHandle(i32 handle, u32 size) {
    void* block;
    if (handle == 0) {
        return AllocHandle(size);
    }
    block = ReallocBlock(HandlePtr(handle), size);
    return SetHandlePtr(handle, block, size);
}

RVA(0x000045f0, 0xc)
void* HandlePtr(i32 handle) {
    return GetHandleEntry(handle)->ptr;
}

RVA(0x00004600, 0x1a)
i32 SetHandlePtr(i32 handle, void* ptr, u16 size) {
    return SetHandleEntry(handle, ptr, size, HANDLE_IN_USE);
}

RVA(0x00004620, 0x2a)
i32 FreeHandle(i32 handle) {
    if (handle == 0) {
        return 0;
    }
    FreeBlock(HandlePtr(handle));
    return ClearHandle(handle);
}

RVA(0x00004650, 0x16)
i32 ClearHandle(i32 handle) {
    SetHandleEntry(handle, 0, 0, 0);
    return 0;
}

RVA(0x00004670, 0xe)
void* HandleWritePtr(i32 handle) {
    return HandlePtr(handle);
}

RVA(0x00004680, 0xe)
void* HandleReadPtr(i32 handle) {
    return HandlePtr(handle);
}

// @dead-code
// Zero-ref: no direct call/jmp, relocated reference or data slot reaches it.
// @identity-TODO: this pointer wrapper's distinct role beside read/write is unproven.
RVA(0x00004690, 0xe)
void* HandleAccessPtr(i32 handle) {
    return HandlePtr(handle);
}

RVA(0x000046a0, 0xe)
i32 GetListTail(i32 list) {
    return HandleListTail(list);
}

RVA(0x000046b0, 0x13)
i32 AppendList(i32 list, i32 node) {
    return HandleListAppend(list, node);
}

RVA(0x000046d0, 0x13)
i32 PopListTail(i32 list, i32* node) {
    return HandleListPopTail(list, node);
}

RVA(0x000046f0, 0x31)
i32 HandleListTail(i32 node) {
    ListLink* link;
    if (node == 0) {
        return 0;
    }
    link = HandleListNodeForRead(node);
    while (link->next != 0) {
        node = link->next;
        link = HandleListNodeForRead(node);
    }
    return node;
}

RVA(0x00004730, 0x25)
i32 HandleListAppend(i32 list, i32 node) {
    i32 tail = HandleListTail(list);
    if (tail == 0) {
        return node;
    }
    return HandleListInsertAfter(tail, node);
}

RVA(0x00004760, 0x1d)
i32 HandleListPopTail(i32 list, i32* node) {
    i32 tail = HandleListTail(list);
    *node = tail;
    return HandleListUnlink(tail);
}

RVA(0x00004780, 0xe)
ListLink* HandleListNodeForRead(i32 node) {
    return HandleReadPtr(node);
}

RVA(0x00004790, 0xe)
ListLink* HandleListNodeForWrite(i32 node) {
    return HandleWritePtr(node);
}

RVA(0x000047a0, 0x33)
i32 HandleListHead(i32 node) {
    ListLink* link;
    if (node == 0) {
        return 0;
    }
    link = HandleListNodeForRead(node);
    while (link->prev != 0) {
        node = link->prev;
        link = HandleListNodeForRead(node);
    }
    return node;
}

RVA(0x000047e0, 0x63)
i32 HandleListUnlink(i32 node) {
    ListLink* link;
    i32 prev;
    i32 next;
    if (node == 0) {
        return 0;
    }
    link = HandleListNodeForWrite(node);
    prev = link->prev;
    next = link->next;
    link->next = 0;
    link->prev = 0;
    if (prev != 0) {
        HandleListNodeForWrite(prev)->next = next;
    }
    if (next != 0) {
        HandleListNodeForWrite(next)->prev = prev;
    }
    if (prev == 0) {
        return HandleListHead(next);
    }
    return HandleListHead(prev);
}

RVA(0x00004850, 0x88)
i32 HandleListInsertAfter(i32 pos, i32 node) {
    ListLink* link;
    i32 tail;
    i32 next;
    i32 prev;
    if (pos == 0) {
        return node;
    }
    tail = HandleListTail(node);
    next = HandleListNodeForWrite(pos)->next;
    if (next == 0) {
        HandleListNodeForWrite(node)->prev = pos;
    } else {
        link = HandleListNodeForWrite(next);
        prev = link->prev;
        link->prev = tail;
        HandleListNodeForWrite(node)->prev = prev;
    }
    next = HandleListNodeForRead(pos)->next;
    HandleListNodeForWrite(tail)->next = next;
    HandleListNodeForWrite(pos)->next = node;
    return HandleListHead(pos);
}

// @dead-code
// Zero-ref: no rel32 caller, data slot or address-taking (giten sema xref --tree).
RVA(0x000048e0, 0x15)
i32 HandleListNext(i32 node) {
    if (node == 0) {
        return 0;
    }
    return HandleListNodeForRead(node)->next;
}

// @dead-code
// Zero-ref: no rel32 caller, data slot or address-taking (giten sema xref --tree).
RVA(0x00004900, 0x16)
i32 HandleListPrev(i32 node) {
    if (node == 0) {
        return 0;
    }
    return HandleListNodeForRead(node)->prev;
}
