#ifndef CONTACTS_H
#define CONTACTS_H

#include <stddef.h>

typedef enum {
    CONTACTS_OK = 0,
    CONTACTS_ERR_MEM,          
    CONTACTS_ERR_INVALID_ARG,
    CONTACTS_ERR_REQUIRED,
    CONTACTS_ERR_OUT_OF_BOUNDS 
} contacts_status_t;

typedef struct {
    char *data;
    size_t length;
    size_t capacity;
} string_t;

typedef struct {
    string_t *items;
    size_t size;
    size_t capacity;
} string_vector_t;

typedef struct {
    string_t key;
    string_t value;
} key_value_t;

typedef struct {
    key_value_t *items;
    size_t size;
    size_t capacity;
} key_value_vector_t;

typedef struct {
    string_t first_name;        
    string_t last_name;         
    string_t place_of_work;     
    string_t position;          
    string_vector_t numbers;    
    string_vector_t emails;     
    key_value_vector_t socials;   
    key_value_vector_t messengers;
} record_t;

typedef struct {
    record_t *records;
    size_t size;
    size_t capacity;
} contacts_t;

contacts_status_t string_init(string_t *str);
void string_free(string_t *str);
contacts_status_t string_set(string_t *str, const char *val);
contacts_status_t string_copy(string_t *dest, const string_t *src);

contacts_status_t string_vector_init(string_vector_t *vec);
void string_vector_free(string_vector_t *vec);
contacts_status_t string_vector_push(string_vector_t *vec, const char *val);
contacts_status_t string_vector_copy(string_vector_t *dest, const string_vector_t *src);
contacts_status_t string_vector_update_at(string_vector_t *vec, size_t index, const char *new_val);
contacts_status_t string_vector_delete_at(string_vector_t *vec, size_t index);

contacts_status_t kv_vector_init(key_value_vector_t *vec);
void kv_vector_free(key_value_vector_t *vec);
contacts_status_t kv_vector_push(key_value_vector_t *vec, const char *key, const char *value);
contacts_status_t kv_vector_copy(key_value_vector_t *dest, const key_value_vector_t *src);
contacts_status_t kv_vector_update_at(key_value_vector_t *vec, size_t index, const char *new_key, const char *new_val);
contacts_status_t kv_vector_delete_at(key_value_vector_t *vec, size_t index);

contacts_status_t record_init(record_t *rec);
void record_free(record_t *rec);
contacts_status_t record_copy(record_t *dest, const record_t *src);

contacts_status_t record_set_first_name(record_t *rec, const char *val); 
contacts_status_t record_set_last_name(record_t *rec, const char *val);  
contacts_status_t record_set_place_of_work(record_t *rec, const char *val);
contacts_status_t record_set_position(record_t *rec, const char *val);

contacts_status_t record_add_number(record_t *rec, const char *number);
contacts_status_t record_update_number_at(record_t *rec, size_t index, const char *new_number);
contacts_status_t record_delete_number_at(record_t *rec, size_t index);

contacts_status_t record_add_email(record_t *rec, const char *email);
contacts_status_t record_update_email_at(record_t *rec, size_t index, const char *new_email);
contacts_status_t record_delete_email_at(record_t *rec, size_t index);

contacts_status_t record_add_social(record_t *rec, const char *social_name, const char *url);
contacts_status_t record_update_social_at(record_t *rec, size_t index, const char *new_social_name, const char *new_url);
contacts_status_t record_delete_social_at(record_t *rec, size_t index);

contacts_status_t record_add_messenger(record_t *rec, const char *messenger_name, const char *profile);
contacts_status_t record_update_messenger_at(record_t *rec, size_t index, const char *new_messenger_name, const char *new_profile);
contacts_status_t record_delete_messenger_at(record_t *rec, size_t index);

contacts_status_t contacts_init(contacts_t *self);
void contacts_free(contacts_t *self);

contacts_status_t contacts_add(contacts_t *self, const record_t *rec);

const record_t* contacts_get_at(const contacts_t *self, size_t index);
size_t contacts_get_count(const contacts_t *self);

contacts_status_t contacts_update_at(contacts_t *self, size_t index, const record_t *new_rec);

contacts_status_t contacts_delete_at(contacts_t *self, size_t index);

record_t* contacts_get_mut_at(contacts_t *self, size_t index);

#endif // CONTACTS_H