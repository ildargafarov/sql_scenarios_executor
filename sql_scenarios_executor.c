#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "scenarios.h"

int main(int argc, char *argv[]) {
    // const char *SCENARIO_ARG_NAME = "--scenario";
    // char *scenario_filename;
    // if (argc == 1) {
    //     printf("--scenario parameter is required\n");
    //     exit(1);
    // } else if (argc == 2) {
    //     printf("--scenario value is not set\n");
    //     exit(1);
    // } else {
    //     for (int i = 1; i < argc; i++) {
    //         if (strcmp(SCENARIO_ARG_NAME, argv[i]) == 0 && i < argc) {
    //             scenario_filename = argv[i + 1];
    //         }
    //     }
    // }
    // printf("Scenario file: %s\n", scenario_filename);

    Operations_t *ops = load_operations_from_json("scenarios/dumb.json");
    Operation_t *op = ops->first;
    while(op) {
        printf("id: %s, query: %s\n", op->id, op->query);
        op = op->next;
    }
    cleanup_operations(ops); 
    return 0;
}