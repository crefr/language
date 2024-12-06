#ifndef HASHTABLE_INCLUDED
#define HASHTABLE_INCLUDED

typedef struct name {
    char * name;
    struct name * next;

    void * data;
    size_t elem_size;
} name_t;

typedef struct {
    name_t ** names;
    size_t table_size;
} table_t;


table_t tableCtor(size_t table_size);

void tableDtor(table_t * table);

void tableInsert(table_t * table, const char * name, const void * data, size_t size);

name_t * tableLookup(table_t * table, const char * name);

#endif
