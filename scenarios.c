#include <stdio.h>
#include <stdlib.h>
#include <json-c/json.h>
#include <string.h>
#include "scenarios.h"

operations_t* load_operations_from_json(char *filename) {
    json_object *root = json_object_from_file(filename);
    if (!root) {
        fprintf(stderr, "Failed load scenario: %s\n", json_util_get_last_err());
        exit(1);
    }
    
    operations_t *ops = (operations_t *) malloc(sizeof(operations_t));
    int n = json_object_array_length(root);
    
    json_object *item_obj;
    operation_t *prev_op;
    for (int i = 0; i < n; i++) {
        item_obj = json_object_array_get_idx(root, i);
        json_object *id_obj = json_object_object_get(item_obj, "id");
        json_object *query_obj = json_object_object_get(item_obj, "query");
        
        operation_t *op = (operation_t *) malloc(sizeof(operation_t));
        op->id = json_object_get_int(id_obj);
        op->query = strdup(json_object_get_string(query_obj));
        if (i == 0) {
            ops->first = op;
        } else {
            prev_op->next = op;
        }
        prev_op = op;
    }
    ops->size = n;

    json_object_put(root);
    return ops;
}

void cleanup_operations(operations_t* ops) {
    operation_t *op = ops->first;
    while (!ops->first) {
        op = ops->first;
        ops->first = ops->first->next;
        free(op);
    }
    free(ops);
}