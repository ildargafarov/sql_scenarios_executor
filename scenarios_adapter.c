#include <stdlib.h>
#include <stdio.h>
#include "scenarios.h"

int main() {
    operations_t *ops = load_operations_from_json("scenarios/dumb.json");
    operation_t *op = ops->first;
    while(op) {
        printf("id: %s, query: %s\n", op->id, op->query);
        op = op->next;
    }
    cleanup_operations(ops);
    return 0;
}