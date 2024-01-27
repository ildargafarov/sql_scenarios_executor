struct operation {
    int id;
    const char *query;
    struct operation *next;
};
typedef struct operation operation_t;

struct operations {
    operation_t *first;
    int size;
};
typedef struct operations operations_t;

operations_t* load_operations_from_json(char *filename);

void cleanup_operations(operations_t* ops);