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
    operation_t **operations = malloc(sizeof(operation_t *) * n);
    
    json_object *item_obj;
    for (int i = 0; i < n; i++) {
        item_obj = json_object_array_get_idx(root, i);
        json_object *id_obj = json_object_object_get(item_obj, "id");
        json_object *query_obj = json_object_object_get(item_obj, "query");
        json_object *comment_obj = json_object_object_get(item_obj, "comment");
        json_object *wait_obj = json_object_object_get(item_obj, "wait");
        
        operations[i] = malloc(sizeof(operation_t));
        operations[i]->id = json_object_get_int(id_obj);
        operations[i]->query = strdup(json_object_get_string(query_obj));
        if (comment_obj != NULL) {
            operations[i]->comment = strdup(json_object_get_string(comment_obj));
        } else {
            operations[i]->comment = NULL;
        }
        if (wait_obj != NULL) {
            operations[i]->wait = json_object_get_boolean(wait_obj); 
        } else {
            operations[i]->wait = 1;
        }
    }
    ops->operations = operations;
    ops->size = n;

    json_object_put(root);
    return ops;
}

void cleanup_operations(operations_t* ops) {
    for (int i = 0; i < ops->size; i++) {
        free(ops->operations[i]);
    }
    free(ops);
}