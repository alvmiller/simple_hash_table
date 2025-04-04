#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>

// https://www.digitalocean.com/community/tutorials/hash-table-in-c-plus-plus

#define CAPACITY 100

unsigned long hash_function(const char *str)
{
    unsigned long i = 0;

    for (int j = 0; str[j]; ++j)
        i += str[j];

    return i % CAPACITY;
}

typedef struct Ht_item
{
    char *key;
    char *value;
} Ht_item;

typedef struct LinkedList
{
    Ht_item *item;
    LinkedList *next;
} LinkedList;

typedef struct HashTable
{
    Ht_item **items;
    LinkedList **overflow_buckets;
    int size;
    int count;
} HashTable;

LinkedList *allocate_list()
{
    LinkedList *list = (LinkedList *)malloc(sizeof(LinkedList));
    return list;
}

LinkedList *linkedlist_insert(LinkedList *list, Ht_item *item)
{
    if (!list) {
        LinkedList *head = allocate_list();
        head->item = item;
        head->next = NULL;
        list = head;
        return list;
    } else if (list->next == NULL) {
        LinkedList *node = allocate_list();
        node->item = item;
        node->next = NULL;
        list->next = node;
        return list;
    }

    LinkedList *temp = list;

    while (temp->next->next) {
        temp = temp->next;
    }

    LinkedList *node = allocate_list();
    node->item = item;
    node->next = NULL;
    temp->next = node;
    return list;
}

Ht_item *linkedlist_remove(LinkedList *list)
{
    if (!list)
        return NULL;

    if (!list->next)
        return NULL;

    LinkedList *node = list->next;
    LinkedList *temp = list;
    temp->next = NULL;
    list = node;
    Ht_item *it = NULL;
    memcpy(temp->item, it, sizeof(Ht_item));
    free(temp->item->key);
    free(temp->item->value);
    free(temp->item);
    free(temp);
    return it;
}

void free_linkedlist(LinkedList *list)
{
    LinkedList *temp = list;

    while (list) {
        temp = list;
        list = list->next;
        free(temp->item->key);
        free(temp->item->value);
        free(temp->item);
        free(temp);
    }
}

LinkedList **create_overflow_buckets(HashTable *table)
{
    LinkedList **buckets = (LinkedList **)calloc(table->size, sizeof(LinkedList *));

    for (int i = 0; i < table->size; ++i)
        buckets[i] = NULL;

    return buckets;
}

void free_overflow_buckets(HashTable *table)
{
    LinkedList **buckets = table->overflow_buckets;

    for (int i = 0; i < table->size; i++)
        free_linkedlist(buckets[i]);

    free(buckets);
}

Ht_item *create_item(const char *key, const char *value)
{
    // Creates a pointer to a new HashTable item.
    Ht_item *item = (Ht_item *)malloc(sizeof(Ht_item));
    item->key = (char *)malloc(strlen(key) + 1);
    item->value = (char *)malloc(strlen(value) + 1);
    strcpy(item->key, key);
    strcpy(item->value, value);
    return item;
}

HashTable *create_table(int size)
{
    HashTable *table = (HashTable *)malloc(sizeof(HashTable));
    table->size = size;
    table->count = 0;
    table->items = (Ht_item **)calloc(table->size, sizeof(Ht_item *));

    for (int i = 0; i < table->size; ++i)
        table->items[i] = NULL;

    table->overflow_buckets = create_overflow_buckets(table);

    return table;
}

void free_item(Ht_item *item)
{
    free(item->key);
    free(item->value);
    free(item);
}

void free_table(HashTable *table)
{
    for (int i = 0; i < table->size; i++)
    {
        Ht_item *item = table->items[i];

        if (item != NULL)
            free_item(item);
    }

    free_overflow_buckets(table);
    free(table->items);
    free(table);
}

void handle_collision(HashTable *table, unsigned long index, Ht_item *item)
{
    LinkedList *head = table->overflow_buckets[index];

    if (head == NULL) {
        head = allocate_list();
        head->item = item;
        table->overflow_buckets[index] = head;
        return;
    } else {
        table->overflow_buckets[index] = linkedlist_insert(head, item);
        return;
    }
}

void ht_insert(HashTable *table, const char *key, const char *value)
{
    Ht_item *item = create_item(key, value);
    int index = hash_function(key);

    Ht_item *current_item = table->items[index];
    if (current_item == NULL) {
        if (table->count == table->size) {
            printf("Insert Error: Hash Table is full\n");
            free_item(item);
            return;
        }

        table->items[index] = item;
        table->count++;
    } else {
        if (strcmp(current_item->key, key) == 0) {
            strcpy(table->items[index]->value, value);
            return;
        } else {
            handle_collision(table, index, item);
            return;
        }
    }
}

char *ht_search(HashTable *table, const char *key)
{
    int index = hash_function(key);
    Ht_item *item = table->items[index];
    LinkedList *head = table->overflow_buckets[index];

    if (item != NULL)
    {
        if (strcmp(item->key, key) == 0)
            return item->value;

        if (head == NULL)
            return NULL;

        item = head->item;
        head = head->next;
    }

    return NULL;
}

void ht_delete(HashTable *table, const char *key)
{
    int index = hash_function(key);
    Ht_item *item = table->items[index];
    LinkedList *head = table->overflow_buckets[index];

    if (item == NULL) {
        return;
    } else {
        if (head == NULL && strcmp(item->key, key) == 0) {
            // Collision chain does not exist.
            // Remove the item.
            // Set table index to NULL.
            table->items[index] = NULL;
            free_item(item);
            table->count--;
            return;
        } else if (head != NULL) {
            if (strcmp(item->key, key) == 0) {
                free_item(item);
                LinkedList *node = head;
                head = head->next;
                node->next = NULL;
                table->items[index] = create_item(node->item->key, node->item->value);
                free_linkedlist(node);
                table->overflow_buckets[index] = head;
                return;
            }

            LinkedList *curr = head;
            LinkedList *prev = NULL;

            while (curr) {
                if (strcmp(curr->item->key, key) == 0) {
                    if (prev == NULL) {
                        free_linkedlist(head);
                        table->overflow_buckets[index] = NULL;
                        return;
                    } else {
                        prev->next = curr->next;
                        curr->next = NULL;
                        free_linkedlist(curr);
                        table->overflow_buckets[index] = head;
                        return;
                    }
                }

                curr = curr->next;
                prev = curr;
            }
        }
    }
}

void print_search(HashTable *table, const char *key)
{
    char *val;

    if ((val = ht_search(table, key)) == NULL) {
        printf("Key:%s does not exist\n", key);
        return;
    } else {
        printf("Key:%s, Value:%s\n", key, val);
    }
}

void print_table(HashTable *table)
{
    printf("\nHash Table\n-------------------\n");

    for (int i = 0; i < table -> size; i++) {
        if (table -> items[i]) {
            printf("Index:%d, Key:%s, Value:%s\n", i, table -> items[i] -> key, table -> items[i] -> value);
        }
    }

    printf("-------------------\n\n");
}

int main()
{
    HashTable *ht = create_table(CAPACITY);
    ht_insert(ht, "1", "First address");
    ht_insert(ht, "2", "Second address");
    ht_insert(ht, "Hel", "Third address");
    ht_insert(ht, "Cau", "Fourth address");
    print_search(ht, "1");
    print_search(ht, "2");
    print_search(ht, "3");
    print_search(ht, "Hel");
    print_search(ht, "Cau"); // Collision!
    print_table(ht);
    ht_delete(ht, "1");
    ht_delete(ht, "Cau");
    print_table(ht);
    free_table(ht);
    return 0;
}
