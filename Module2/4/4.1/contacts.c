#include "contacts.h"
#include <stdlib.h>
#include <string.h>

contacts_status_t string_init(string_t *str) {
    if (!str) return CONTACTS_ERR_INVALID_ARG;
    str->data = NULL;
    str->length = 0;
    str->capacity = 0;
    return CONTACTS_OK;
}

void string_free(string_t *str) {
    if (str) {
        free(str->data);
        str->data = NULL;
        str->length = 0;
        str->capacity = 0;
    }
}

contacts_status_t string_set(string_t *str, const char *val) {
    if (!str) return CONTACTS_ERR_INVALID_ARG;
    if (!val) {
        string_free(str);
        return string_init(str);
    }
    size_t len = strlen(val);
    char *temp = realloc(str->data, len + 1);
    if (!temp) return CONTACTS_ERR_MEM;
    str->data = temp;
    strcpy(str->data, val);
    str->length = len;
    str->capacity = len + 1;
    return CONTACTS_OK;
}

contacts_status_t string_copy(string_t *dest, const string_t *src) {
    if (!dest || !src) return CONTACTS_ERR_INVALID_ARG;
    return string_set(dest, src->data);
}

contacts_status_t string_vector_init(string_vector_t *vec) {
    if (!vec) return CONTACTS_ERR_INVALID_ARG;
    vec->items = NULL;
    vec->size = 0;
    vec->capacity = 0;
    return CONTACTS_OK;
}

void string_vector_free(string_vector_t *vec) {
    if (vec) {
        for (size_t i = 0; i < vec->size; i++) {
            string_free(&vec->items[i]);
        }
        free(vec->items);
        vec->items = NULL;
        vec->size = 0;
        vec->capacity = 0;
    }
}

contacts_status_t string_vector_push(string_vector_t *vec, const char *val) {
    if (!vec) return CONTACTS_ERR_INVALID_ARG;
    if (vec->size >= vec->capacity) {
        size_t new_cap = vec->capacity == 0 ? 4 : vec->capacity * 2;
        string_t *temp = realloc(vec->items, new_cap * sizeof(string_t));
        if (!temp) return CONTACTS_ERR_MEM;
        vec->items = temp;
        vec->capacity = new_cap;
    }
    string_t new_str;
    string_init(&new_str);
    contacts_status_t status = string_set(&new_str, val);
    if (status != CONTACTS_OK) return status;
    vec->items[vec->size] = new_str;
    vec->size++;
    return CONTACTS_OK;
}

contacts_status_t string_vector_copy(string_vector_t *dest, const string_vector_t *src) {
    if (!dest || !src) return CONTACTS_ERR_INVALID_ARG;
    string_vector_free(dest);
    string_vector_init(dest);
    for (size_t i = 0; i < src->size; i++) {
        contacts_status_t status = string_vector_push(dest, src->items[i].data);
        if (status != CONTACTS_OK) {
            string_vector_free(dest);
            return status;
        }
    }
    return CONTACTS_OK;
}

contacts_status_t string_vector_update_at(string_vector_t *vec, size_t index, const char *new_val) {
    if (!vec) return CONTACTS_ERR_INVALID_ARG;
    if (index >= vec->size) return CONTACTS_ERR_OUT_OF_BOUNDS;
    return string_set(&vec->items[index], new_val);
}

contacts_status_t string_vector_delete_at(string_vector_t *vec, size_t index) {
    if (!vec) return CONTACTS_ERR_INVALID_ARG;
    if (index >= vec->size) return CONTACTS_ERR_OUT_OF_BOUNDS;
    string_free(&vec->items[index]);
    for (size_t i = index; i < vec->size - 1; i++) {
        vec->items[i] = vec->items[i + 1];
    }
    vec->size--;
    return CONTACTS_OK;
}

contacts_status_t kv_vector_init(key_value_vector_t *vec) {
    if (!vec) return CONTACTS_ERR_INVALID_ARG;
    vec->items = NULL;
    vec->size = 0;
    vec->capacity = 0;
    return CONTACTS_OK;
}

void kv_vector_free(key_value_vector_t *vec) {
    if (vec) {
        for (size_t i = 0; i < vec->size; i++) {
            string_free(&vec->items[i].key);
            string_free(&vec->items[i].value);
        }
        free(vec->items);
        vec->items = NULL;
        vec->size = 0;
        vec->capacity = 0;
    }
}

contacts_status_t kv_vector_push(key_value_vector_t *vec, const char *key, const char *value) {
    if (!vec) return CONTACTS_ERR_INVALID_ARG;
    if (vec->size >= vec->capacity) {
        size_t new_cap = vec->capacity == 0 ? 4 : vec->capacity * 2;
        key_value_t *temp = realloc(vec->items, new_cap * sizeof(key_value_t));
        if (!temp) return CONTACTS_ERR_MEM;
        vec->items = temp;
        vec->capacity = new_cap;
    }
    key_value_t pair;
    string_init(&pair.key);
    string_init(&pair.value);
    contacts_status_t status = string_set(&pair.key, key);
    if (status != CONTACTS_OK) return status;
    status = string_set(&pair.value, value);
    if (status != CONTACTS_OK) {
        string_free(&pair.key);
        return status;
    }
    vec->items[vec->size] = pair;
    vec->size++;
    return CONTACTS_OK;
}

contacts_status_t kv_vector_copy(key_value_vector_t *dest, const key_value_vector_t *src) {
    if (!dest || !src) return CONTACTS_ERR_INVALID_ARG;
    kv_vector_free(dest);
    kv_vector_init(dest);
    for (size_t i = 0; i < src->size; i++) {
        contacts_status_t status = kv_vector_push(dest, src->items[i].key.data, src->items[i].value.data);
        if (status != CONTACTS_OK) {
            kv_vector_free(dest);
            return status;
        }
    }
    return CONTACTS_OK;
}

contacts_status_t kv_vector_update_at(key_value_vector_t *vec, size_t index, const char *new_key, const char *new_val) {
    if (!vec) return CONTACTS_ERR_INVALID_ARG;
    if (index >= vec->size) return CONTACTS_ERR_OUT_OF_BOUNDS;
    contacts_status_t status = string_set(&vec->items[index].key, new_key);
    if (status != CONTACTS_OK) return status;
    return string_set(&vec->items[index].value, new_val);
}

contacts_status_t kv_vector_delete_at(key_value_vector_t *vec, size_t index) {
    if (!vec) return CONTACTS_ERR_INVALID_ARG;
    if (index >= vec->size) return CONTACTS_ERR_OUT_OF_BOUNDS;
    string_free(&vec->items[index].key);
    string_free(&vec->items[index].value);
    for (size_t i = index; i < vec->size - 1; i++) {
        vec->items[i] = vec->items[i + 1];
    }
    vec->size--;
    return CONTACTS_OK;
}

contacts_status_t record_init(record_t *rec) {
    if (!rec) return CONTACTS_ERR_INVALID_ARG;
    string_init(&rec->first_name);
    string_init(&rec->last_name);
    string_init(&rec->place_of_work);
    string_init(&rec->position);
    string_vector_init(&rec->numbers);
    string_vector_init(&rec->emails);
    kv_vector_init(&rec->socials);
    kv_vector_init(&rec->messengers);
    return CONTACTS_OK;
}

void record_free(record_t *rec) {
    if (rec) {
        string_free(&rec->first_name);
        string_free(&rec->last_name);
        string_free(&rec->place_of_work);
        string_free(&rec->position);
        string_vector_free(&rec->numbers);
        string_vector_free(&rec->emails);
        kv_vector_free(&rec->socials);
        kv_vector_free(&rec->messengers);
    }
}

contacts_status_t record_copy(record_t *dest, const record_t *src) {
    if (!dest || !src) return CONTACTS_ERR_INVALID_ARG;
    record_free(dest);
    record_init(dest);
    contacts_status_t status;
    if ((status = string_copy(&dest->first_name, &src->first_name)) != CONTACTS_OK ||
        (status = string_copy(&dest->last_name, &src->last_name)) != CONTACTS_OK ||
        (status = string_copy(&dest->place_of_work, &src->place_of_work)) != CONTACTS_OK ||
        (status = string_copy(&dest->position, &src->position)) != CONTACTS_OK ||
        (status = string_vector_copy(&dest->numbers, &src->numbers)) != CONTACTS_OK ||
        (status = string_vector_copy(&dest->emails, &src->emails)) != CONTACTS_OK ||
        (status = kv_vector_copy(&dest->socials, &src->socials)) != CONTACTS_OK ||
        (status = kv_vector_copy(&dest->messengers, &src->messengers)) != CONTACTS_OK) {
        record_free(dest);
        return status;
    }
    return CONTACTS_OK;
}

contacts_status_t record_set_first_name(record_t *rec, const char *val) {
    if (!rec) return CONTACTS_ERR_INVALID_ARG;
    if (!val || strlen(val) == 0) return CONTACTS_ERR_REQUIRED;
    return string_set(&rec->first_name, val);
}

contacts_status_t record_set_last_name(record_t *rec, const char *val) {
    if (!rec) return CONTACTS_ERR_INVALID_ARG;
    if (!val || strlen(val) == 0) return CONTACTS_ERR_REQUIRED;
    return string_set(&rec->last_name, val);
}

contacts_status_t record_set_place_of_work(record_t *rec, const char *val) {
    if (!rec) return CONTACTS_ERR_INVALID_ARG;
    return string_set(&rec->place_of_work, val);
}

contacts_status_t record_set_position(record_t *rec, const char *val) {
    if (!rec) return CONTACTS_ERR_INVALID_ARG;
    return string_set(&rec->position, val);
}

contacts_status_t record_add_number(record_t *rec, const char *number) {
    if (!rec) return CONTACTS_ERR_INVALID_ARG;
    return string_vector_push(&rec->numbers, number);
}

contacts_status_t record_update_number_at(record_t *rec, size_t index, const char *new_number) {
    if (!rec) return CONTACTS_ERR_INVALID_ARG;
    return string_vector_update_at(&rec->numbers, index, new_number);
}

contacts_status_t record_delete_number_at(record_t *rec, size_t index) {
    if (!rec) return CONTACTS_ERR_INVALID_ARG;
    return string_vector_delete_at(&rec->numbers, index);
}

contacts_status_t record_add_email(record_t *rec, const char *email) {
    if (!rec) return CONTACTS_ERR_INVALID_ARG;
    return string_vector_push(&rec->emails, email);
}

contacts_status_t record_update_email_at(record_t *rec, size_t index, const char *new_email) {
    if (!rec) return CONTACTS_ERR_INVALID_ARG;
    return string_vector_update_at(&rec->emails, index, new_email);
}

contacts_status_t record_delete_email_at(record_t *rec, size_t index) {
    if (!rec) return CONTACTS_ERR_INVALID_ARG;
    return string_vector_delete_at(&rec->emails, index);
}

contacts_status_t record_add_social(record_t *rec, const char *social_name, const char *url) {
    if (!rec) return CONTACTS_ERR_INVALID_ARG;
    return kv_vector_push(&rec->socials, social_name, url);
}

contacts_status_t record_update_social_at(record_t *rec, size_t index, const char *new_social_name, const char *new_url) {
    if (!rec) return CONTACTS_ERR_INVALID_ARG;
    return kv_vector_update_at(&rec->socials, index, new_social_name, new_url);
}

contacts_status_t record_delete_social_at(record_t *rec, size_t index) {
    if (!rec) return CONTACTS_ERR_INVALID_ARG;
    return kv_vector_delete_at(&rec->socials, index);
}

contacts_status_t record_add_messenger(record_t *rec, const char *messenger_name, const char *profile) {
    if (!rec) return CONTACTS_ERR_INVALID_ARG;
    return kv_vector_push(&rec->messengers, messenger_name, profile);
}

contacts_status_t record_update_messenger_at(record_t *rec, size_t index, const char *new_messenger_name, const char *new_profile) {
    if (!rec) return CONTACTS_ERR_INVALID_ARG;
    return kv_vector_update_at(&rec->messengers, index, new_messenger_name, new_profile);
}

contacts_status_t record_delete_messenger_at(record_t *rec, size_t index) {
    if (!rec) return CONTACTS_ERR_INVALID_ARG;
    return kv_vector_delete_at(&rec->messengers, index);
}

static int record_compare(const record_t *a, const record_t *b) {
    int cmp = strcmp(a->last_name.data, b->last_name.data);
    if (cmp != 0) return cmp;
    return strcmp(a->first_name.data, b->first_name.data);
}

static contact_node_t* contacts_get_node_at(const contacts_t *self, size_t index) {
    if (!self || index >= self->size) return NULL;
    
    contact_node_t *curr;
    if (index < self->size / 2) {
        curr = self->head;
        for (size_t i = 0; i < index; i++) {
            curr = curr->next;
        }
    } else {
        curr = self->tail;
        for (size_t i = self->size - 1; i > index; i--) {
            curr = curr->prev;
        }
    }
    return curr;
}

contacts_status_t contacts_init(contacts_t *self) {
    if (!self) return CONTACTS_ERR_INVALID_ARG;
    self->head = NULL;
    self->tail = NULL;
    self->size = 0;
    return CONTACTS_OK;
}

void contacts_free(contacts_t *self) {
    if (self) {
        contact_node_t *curr = self->head;
        while (curr) {
            contact_node_t *next = curr->next;
            record_free(&curr->record);
            free(curr);
            curr = next;
        }
        self->head = NULL;
        self->tail = NULL;
        self->size = 0;
    }
}

static void contacts_insert_node_sorted(contacts_t *self, contact_node_t *node) {
    if (!self->head) {
        self->head = node;
        self->tail = node;
        node->prev = NULL;
        node->next = NULL;
    } else {
        contact_node_t *curr = self->head;
        while (curr && record_compare(&node->record, &curr->record) > 0) {
            curr = curr->next;
        }

        if (!curr) {
            node->prev = self->tail;
            node->next = NULL;
            self->tail->next = node;
            self->tail = node;
        } else if (curr == self->head) {
            node->next = self->head;
            node->prev = NULL;
            self->head->prev = node;
            self->head = node;
        } else {
            node->next = curr;
            node->prev = curr->prev;
            curr->prev->next = node;
            curr->prev = node;
        }
    }
    self->size++;
}

contacts_status_t contacts_add(contacts_t *self, const record_t *rec) {
    if (!self || !rec) return CONTACTS_ERR_INVALID_ARG;
    if (!rec->first_name.data || rec->first_name.length == 0 ||
        !rec->last_name.data || rec->last_name.length == 0) {
        return CONTACTS_ERR_REQUIRED;
    }

    contact_node_t *new_node = malloc(sizeof(contact_node_t));
    if (!new_node) return CONTACTS_ERR_MEM;

    record_init(&new_node->record);
    contacts_status_t status = record_copy(&new_node->record, rec);
    if (status != CONTACTS_OK) {
        free(new_node);
        return status;
    }

    contacts_insert_node_sorted(self, new_node);
    return CONTACTS_OK;
}

const record_t* contacts_get_at(const contacts_t *self, size_t index) {
    contact_node_t *node = contacts_get_node_at(self, index);
    return node ? &node->record : NULL;
}

record_t* contacts_get_mut_at(contacts_t *self, size_t index) {
    contact_node_t *node = contacts_get_node_at(self, index);
    return node ? &node->record : NULL;
}

size_t contacts_get_count(const contacts_t *self) {
    return self ? self->size : 0;
}

contacts_status_t contacts_update_at(contacts_t *self, size_t index, const record_t *new_rec) {
    if (!self || !new_rec) return CONTACTS_ERR_INVALID_ARG;
    contact_node_t *node = contacts_get_node_at(self, index);
    if (!node) return CONTACTS_ERR_OUT_OF_BOUNDS;

    if (!new_rec->first_name.data || new_rec->first_name.length == 0 ||
        !new_rec->last_name.data || new_rec->last_name.length == 0) {
        return CONTACTS_ERR_REQUIRED;
    }

    record_t temp;
    record_init(&temp);
    contacts_status_t status = record_copy(&temp, new_rec);
    if (status != CONTACTS_OK) return status;

    if (node == self->head) {
        self->head = node->next;
        if (self->head) self->head->prev = NULL;
        else self->tail = NULL;
    } else if (node == self->tail) {
        self->tail = node->prev;
        if (self->tail) self->tail->next = NULL;
        else self->head = NULL;
    } else {
        node->prev->next = node->next;
        node->next->prev = node->prev;
    }
    self->size--;

    record_free(&node->record);
    node->record = temp;

    contacts_insert_node_sorted(self, node);
    return CONTACTS_OK;
}

contacts_status_t contacts_delete_at(contacts_t *self, size_t index) {
    if (!self) return CONTACTS_ERR_INVALID_ARG;
    contact_node_t *node = contacts_get_node_at(self, index);
    if (!node) return CONTACTS_ERR_OUT_OF_BOUNDS;

    if (node == self->head) {
        self->head = node->next;
        if (self->head) self->head->prev = NULL;
        else self->tail = NULL;
    } else if (node == self->tail) {
        self->tail = node->prev;
        if (self->tail) self->tail->next = NULL;
        else self->head = NULL;
    } else {
        node->prev->next = node->next;
        node->next->prev = node->prev;
    }

    record_free(&node->record);
    free(node);
    self->size--;
    return CONTACTS_OK;
}

contacts_status_t contacts_resort_record(contacts_t *self, const record_t *rec) {
    if (!self || !rec) return CONTACTS_ERR_INVALID_ARG;

    contact_node_t *node = self->head;
    while (node && &node->record != rec) {
        node = node->next;
    }

    if (!node) return CONTACTS_ERR_OUT_OF_BOUNDS;

    int needs_resort = 0;
    if (node->prev && record_compare(&node->prev->record, &node->record) > 0) {
        needs_resort = 1;
    }
    if (node->next && record_compare(&node->record, &node->next->record) > 0) {
        needs_resort = 1;
    }

    if (needs_resort) {
        // Временно извлекаем узел из двусвязного списка
        if (node == self->head) {
            self->head = node->next;
            if (self->head) self->head->prev = NULL;
            else self->tail = NULL;
        } else if (node == self->tail) {
            self->tail = node->prev;
            if (self->tail) self->tail->next = NULL;
            else self->head = NULL;
        } else {
            node->prev->next = node->next;
            node->next->prev = node->prev;
        }
        self->size--;

        // Вставляем узел заново в правильную (отсортированную) позицию
        contacts_insert_node_sorted(self, node);
    }

    return CONTACTS_OK;
}