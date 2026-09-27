#ifndef GITEN_UTIL_LIST_H
#define GITEN_UTIL_LIST_H

#include <rva.h>

// Doubly linked lists of nodes that start with their `next` and `prev`
// pointers; a list is held as a pointer to its first node.
typedef struct ListNode {
    struct ListNode* next;
    struct ListNode* prev;
} ListNode;

// The node after `node`, or NULL (also for a NULL `node`).
void* ListNext(void* node);

// The last node of the list that `node` starts.
void* ListLast(void* node);

// Links `node` after the last node of the list `*list` (or makes it the
// first); returns that last node (or the list head).
void* ListAppend(void* list, void* node);

// Unlinks `node` from its list (a first node's `prev` is the list head
// itself) and returns it.
void* ListUnlink(void* node);

// Unlinks and returns the last node of the list that `node` starts.
void* ListPopLast(void* node);

#endif // GITEN_UTIL_LIST_H
