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

int append_to_list(void *new_data, node_t *list, int data_type)
{
    if (list->data == NULL)
    {
        list->data = new_data;
        list->next = NULL;
        list->data_type = data_type;
        return 0;
    }
    node_t *new_node = malloc(sizeof(node_t));
    if (new_node == NULL)
    {
        // malloc fail
        return -1;
    }
    new_node->data = new_data;
    new_node->next = NULL;
    new_node->data_type = data_type;
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
    }
    else
    {
        q->data = NULL;
    }

    return data;
}

node_t *get_index(node_t *list, int index)
{
    node_t *tmp_list = list;
    while (tmp_list->next != NULL)
    {
        if (index == 0)
        {
            return tmp_list;
        }
        tmp_list = tmp_list->next;
        index--;
    }
    return NULL;
}

// Amortize resizing of array by increasing size of array by 2
uint32_t *increase_array_size(uint32_t *old_array)
{
    uint32_t *new_array = malloc(sizeof(uint32_t) * 2 * MAX_CLIENT_ID);
    memset(new_arry, 0, 2 * MAX_CLIENT_ID);
    int i;
    for (i = 0; i < MAX_CLIENT_ID; i++)
    {
        new_array[i] = old_array[i];
    }
    MAX_CLIENT_ID *= 2;
    return new_array;
}