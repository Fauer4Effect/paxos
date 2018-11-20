#include <stdlib.h>

#include "data_structures.h"

int list_length(node_t *list)
{
    if (list->data == NULL)
        return 0;
    
    int length = 1;
    while (list->next != NULL)
    {
        length++;
        list = list->next;
    }
    return length;
}

int append_to_list(void *new_data, node_t *list)
{
    node_t *new_node = malloc(sizeof(node_t));
    if (new_node == NULL)
    {
        // malloc fail
        return -1;
    }
    new_node->data = new_data;
    new_node->next = NULL;
    while (list->next != NULL)
    {
        list = list->next;
    }
    list->next = new_node;
    return 0;
}

int clear_list(node_t *list)
{
    int length = list_length(list);
    node_t *next_node;
    while (list->next != NULL)
    {
        next_node = list->next;
        free(list->data);
        free(list);
        list = next_node;
    }
    return 0;
}

void *pop_from_queue(node_t *q)
{
    void *data = q->data;
    if (q->next != NULL)
    {
        q = q->next;
    } else
    {
        q->data = NULL;
    }
    
    return data;
}