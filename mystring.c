#include "slash.h"

string* string_new(size_t capacity) {
    if(capacity == 0) {
        perror("Capacité trop petite creation string");
        return NULL;
    }
    string* str = malloc(sizeof(string));
    if(str == NULL) {
        perror("Erreur allocation string");
        return NULL;
    }
    str->capacity = capacity;
    str->length = 0;
    char* ptr = malloc(capacity);
    if(ptr == NULL) {
        perror("Erreur allocation tab");
        return NULL;
    }
    *ptr = '\0';
    str->data = ptr;

    return str;
}

void string_delete(string* str) {
    if(str == NULL)
        return;
    if(str->data != NULL)
        free(str->data);
    free(str);
}

int string_append (string* dest, char * src) {
    if(dest == NULL || src == NULL || dest->data == NULL)
        return 0;
    size_t size = strlen(src);
    // Le dernier char du tableau est pour le '\0'. Ce qui explique le >= au lieu de >
    if(dest->length + size >= dest->capacity) {
        char* ptr = realloc(dest->data, dest->capacity * 2);
        if(ptr == NULL) {
            perror("Erreur realloc string_append");
            return 0;
        }
        dest->data = ptr;
        dest->capacity *= 2;
    }
    memmove(dest->data + dest->length, src, size);
    dest->length += size;
    dest->data[dest->length] = '\0';
    return 1;
}

void string_truncate (string* str, size_t nchars) {
    if(str == NULL || str->data == NULL)
        return;
    str->length = nchars >= str->length ? 0 : str->length - nchars;
    str->data[str->length] = '\0';
}