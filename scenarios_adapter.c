#include <stdlib.h>
#include <stdio.h>
#include "scenarios.h"

int main() {
    Operations_t *ops = load_operations_from_json("scenarios/dumb.json");
    Operation_t *op = ops->first;
    while(op) {
        printf("id: %s, query: %s\n", op->id, op->query);
        op = op->next;
    }
    cleanup_operations(ops);
    return 0;
}