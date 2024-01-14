#include <stdio.h>
#include <stdlib.h>
#include <json-c/json.h>
#include "scenarios.h"

Operations_t* load_operations_from_json(char *filename) {
    json_object *root = json_object_from_file(filename);
    if (!root) {
        fprintf(stderr, "Failed load scenario: %s\n", json_util_get_last_err());
        exit(1);
    }
    
    Operations_t *ops = (Operations_t *) malloc(sizeof(Operations_t));
    int n = json_object_array_length(root);
    
    json_object *item_obj;
    Operation_t *prev_op;
    for (int i = 0; i < n; i++) {
        item_obj = json_object_array_get_idx(root, i);
        json_object *id_obj = json_object_object_get(item_obj, "id");
        json_object *query_obj = json_object_object_get(item_obj, "query");
        
        Operation_t *op = (Operation_t *) malloc(sizeof(Operation_t));
        op->id = json_object_get_string(id_obj);
        op->query = json_object_get_string(query_obj);
        printf("%s\n", prev_op);
        if (prev_op) {
            prev_op->next = op;
        } else {
            ops->first = op;
        }
        prev_op = op;
    }
    ops->size = n;

    json_object_put(root);
    return ops;
}

void cleanup_operations(Operations_t* ops) {
    Operation_t *op = ops->first;
    while (!ops->first) {
        op = ops->first;
        ops->first = ops->first->next;
        free(op);
    }
    free(ops);
}