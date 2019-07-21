#include "error.h"
#include "list.h"
#include <stdlib.h>
#include <stdio.h>

int is_empty_list(const list_t *list)
{
    M_REQUIRE_NON_NULL(list);
    return list->front == NULL && list->back == NULL;
}

void init_list(list_t *list)
{
    M_REQUIRE_NON_NULL(list);
    list->front = NULL;
    list->back = NULL;
}

void clear_list(list_t *list)
{
    if (list == NULL || is_empty_list(list))
        return;

    node_t *iter = NULL;
    node_t *temp = NULL;

    for (iter = list->front; iter != NULL; iter = temp)
    {
        temp = iter->next;
        free(iter);
    }
    list->front = NULL; // correcteur could use init_list directly
    list->back = NULL;
}

node_t *push_back(list_t *list, const list_content_t *content)
{
    M_REQUIRE_NON_NULL(list);
    M_REQUIRE_NON_NULL(content);
    node_t *newNode = calloc(1, sizeof(node_t)); // correcteur alloc code will be duplicated
    // check is new_node is != NULL
    newNode->value = *content;
    newNode->next = NULL;
    if (is_empty_list(list))
    {
        newNode->previous = NULL;
        list->front = newNode;
        list->back = newNode;
    }
    else
    {
        newNode->previous = list->back;
        (list->back)->next = newNode;
        list->back = newNode;
    }
    return newNode;
}

node_t *push_front(list_t *list, const list_content_t *content)
{

    M_REQUIRE_NON_NULL(list);
    M_REQUIRE_NON_NULL(content);
    node_t *newNode = calloc(1, sizeof(node_t));
    M_REQUIRE_NON_NULL(newNode);
    newNode->value = *content;
    newNode->previous = NULL;
    if (is_empty_list(list))
    {
        newNode->next = NULL;
        list->front = newNode;
        list->back = newNode;
    }
    else
    {
        newNode->next = list->front;
        (list->front)->previous = newNode;
        list->front = newNode;
    }
    return newNode;
}

void pop_back(list_t *list)
{
    M_REQUIRE_NON_NULL(list);
    M_REQUIRE(!is_empty_list(list), ERR_SIZE, "Popping empty list", NULL);
    node_t *toPop = list->back;
    list->back = toPop->previous;
    list->back->next = NULL;
    free(toPop);
}

void pop_front(list_t *list)
{
    M_REQUIRE_NON_NULL(list);
    M_REQUIRE(!is_empty_list(list), ERR_SIZE, "Popping empty list", NULL);
    node_t *toPop = list->front;
    list->front = toPop->next;
    list->front->previous = NULL;
    free(toPop);
}

void move_back(list_t *list, node_t *node)
{
    M_REQUIRE_NON_NULL(list);
    M_REQUIRE_NON_NULL(node);
    if (node->next == NULL)
        return; //Node already at the back // correcteur what if the node is not in the list?
    node_t *nextNode = node->next;
    nextNode->previous = node->previous;

    if (node->previous != NULL)
        (node->previous)->next = nextNode;
    else                        // correcteur probably better to explicitely check if node == front
        list->front = nextNode; //Node put at  the back was first node

    (list->back)->next = node;
    node->previous = list->back;
    node->next = NULL;
    list->back = node;
}

int print_list(FILE *stream, const list_t *list)
{
    M_REQUIRE_NON_NULL(stream);
    M_REQUIRE_NON_NULL(list);
    putc('(', stream);
    size_t total = 0;
    for_all_nodes(n, list)
    {
        if (n->previous != NULL)
            fputs(", ", stream);
        print_node(stream, n->value);
        total++;
    }
    putc(')', stream);
    return total;
}

int print_reverse_list(FILE *stream, const list_t *list)
{
    M_REQUIRE_NON_NULL(stream);
    M_REQUIRE_NON_NULL(list);
    putc('(', stream);
    size_t total = 0;
    for_all_nodes_reverse(n, list)
    {
        if (n->next != NULL)
            fputs(", ", stream);
        print_node(stream, n->value);
        total++;
    }
    putc(')', stream);
    return total;
}
