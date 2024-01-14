typedef struct Operation {
    const char *id;
    const char *query;
    struct Operation *next;
} Operation_t;

typedef struct Operations {
    Operation_t *first;
    int size;
} Operations_t;

Operations_t* load_operations_from_json(char *filename);

void cleanup_operations(Operations_t* ops);