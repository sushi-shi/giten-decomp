// @identity-TODO: the owning TU is unproven; this unit holds one contiguous
// retail span until link-order evidence names it.

#include <rva.h>

#include <Util/List.h>

#include <stddef.h>

RVA(0x0000bcd0, 0xe)
void* ListNext(void* node) {
    ListNode* entry = node;
    if (entry != NULL) {
        return entry->next;
    }
    return NULL;
}

RVA(0x0000bce0, 0x15)
void* ListLast(void* node) {
    ListNode* entry = node;
    while (entry != NULL) {
        ListNode* next = entry->next;
        if (next == NULL) {
            return entry;
        }
        entry = next;
    }
    return NULL;
}

// Links the chain that starts at `node` in after `pos`.
RVA(0x0000bd00, 0x26)
static void ListInsertAfter(void* pos, void* node) {
    ListNode* at = pos;
    ListNode* first = node;
    ListNode* last = ListLast(first);
    if (at->next != NULL) {
        at->next->prev = last;
    }
    first->prev = at;
    last->next = at->next;
    at->next = first;
}

RVA(0x0000bd30, 0x3a)
void* ListAppend(void* list, void* node) {
    ListNode* head = list;
    ListNode* entry = node;
    ListNode* last = ListLast(head->next);
    if (last != NULL) {
        ListInsertAfter(last, entry);
    } else {
        last = head;
        head->next = entry;
        entry->prev = head;
    }
    return last;
}

RVA(0x0000bd70, 0x29)
void* ListUnlink(void* node) {
    ListNode* entry = node;
    if (entry->prev != NULL) {
        entry->prev->next = entry->next;
    }
    if (entry->next != NULL) {
        entry->next->prev = entry->prev;
    }
    entry->prev = NULL;
    entry->next = NULL;
    return entry;
}

RVA(0x0000bda0, 0x21)
void* ListPopLast(void* node) {
    ListNode* last = ListLast(node);
    if (last != NULL) {
        ListUnlink(last);
    }
    return last;
}
