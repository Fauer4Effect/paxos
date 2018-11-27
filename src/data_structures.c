#include <stdlib.h>
#include <string.h>

#include "data_structures.h"

int list_length(node_t *list)
{
    logger(0, LOG_LEVEL, MY_SERVER_ID, "Getting list length\n");
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
    logger(0, LOG_LEVEL, MY_SERVER_ID, "Appending to list\n");
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
    logger(0, LOG_LEVEL, MY_SERVER_ID, "Clearing list\n");
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
    logger(0, LOG_LEVEL, MY_SERVER_ID, "Popping from queue\n");
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
    logger(0, LOG_LEVEL, MY_SERVER_ID, "Getting index %d\n", index);
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
    logger(0, LOG_LEVEL, MY_SERVER_ID, "Increasing array size\n");
    uint32_t *new_array = malloc(sizeof(uint32_t) * 2 * MAX_CLIENT_ID);
    memset(new_array, 0, 2 * MAX_CLIENT_ID);
    int i;
    for (i = 0; i < MAX_CLIENT_ID; i++)
    {
        new_array[i] = old_array[i];
    }
    MAX_CLIENT_ID *= 2;
    return new_array;
}
