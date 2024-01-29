#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <libpq-fe.h>
#include "scenarios.h"

#define PG_CONN_PARAMS_MAX_LEN 256

char* pg_conn_params() {
    const char *PG_HOST_ADDR = getenv("POSTGRES_HOST_ADDR");
    const char *PG_PORT = getenv("POSTGRES_PORT");
    const char *PG_USER = getenv("POSTGRES_USER");
    const char *PG_PASS = getenv("POSTGRES_PASSWORD");
    const char *PG_DB = getenv("POSTGRES_DB");

    char* conn_params = malloc(sizeof(char) * PG_CONN_PARAMS_MAX_LEN);
    sprintf(
        conn_params, 
        "hostaddr=%s port=%s dbname=%s user=%s password=%s",
        PG_HOST_ADDR, PG_PORT, PG_DB, PG_USER, PG_PASS
    );
    return conn_params;
}

int get_ids_number(operations_t *ops) {
    int max_id = -1;
    operation_t *op = ops->first;
    for(int i = 0; i < ops->size; i++) {
        if (op->id > max_id) {
            max_id = op->id;
        }
        op = op->next;
    }
    return max_id + 1;
}

void clear(operations_t *ops, PGconn* conns[]) {
    for (int i = 0; i < get_ids_number(ops); i++) {
        PQfinish(conns[i]);
    }
    cleanup_operations(ops); 
}

int init_conns(int conns_count, PGconn* conns[]) {
    char* conn_params = pg_conn_params();
    for (int i = 0; i < conns_count; i++) {
        PGconn *conn = PQconnectdb(conn_params);
        if (PQstatus(conn) == CONNECTION_BAD) {
            fprintf(stderr, "Connection to database failed: %s\n", PQerrorMessage(conn));
            PQfinish(conn);
            for(int j = 0; j < i; j++) {
                PQfinish(conns[j]);
            }
            free(conn_params);
            return 1;
        }
        conns[i] = conn;
    }
    free(conn_params);
    return 0;
}

void execute_scenario(operations_t *ops, PGconn* conns[]) {
    operation_t *op = ops->first;
    for(int i = 0; i < ops->size; i++) {
        printf("========= [%d] ==========\n", op->id);

        if(op->comment != NULL) {
            printf("#  %s\n", op->comment);
        }

        char query_copy[strlen(op->query) + 1];
        strcpy(query_copy, op->query);
        char *query_item = strtok(query_copy, ";");
        while(query_item != NULL) {
            printf("=> %s\n", query_item);
            query_item = strtok(NULL, ";");
        }
        
        PGresult* res = PQexec(conns[op->id], op->query);
        if (PQresultStatus(res) == PGRES_FATAL_ERROR) {
            fprintf(stderr, "Query failed: %s\n", PQresultErrorMessage(res));
            PQclear(res);
            clear(ops, conns);
            exit(1);
        }
        int rows = PQntuples(res);
        int cols = PQnfields(res);
        for (int row = 0; row < rows; row++) {
            printf("   ------ [%d] ------\n", row);
            for (int col = 0; col < cols; col++) {
                printf("   %s: %s\n", PQfname(res, col), PQgetvalue(res, row, col));
            }
        }
        printf("\n");
        PQclear(res);
        op = op->next;
    }
}

int main(int argc, char *argv[]) {
    const char *SCENARIO_ARG_NAME = "--scenario";
    char *scenario_filename;
    if (argc == 1) {
        printf("--scenario parameter is required\n");
        exit(1);
    } else if (argc == 2) {
        printf("--scenario value is not set\n");
        exit(1);
    } else {
        for (int i = 1; i < argc; i++) {
            if (strcmp(SCENARIO_ARG_NAME, argv[i]) == 0 && i < argc) {
                scenario_filename = argv[i + 1];
            }
        }
    }
    printf("Scenario file: %s\n", scenario_filename);
    operations_t *ops = load_operations_from_json(scenario_filename);
    
    int ids_number = get_ids_number(ops);
    PGconn* conns[ids_number];
    if(init_conns(ids_number, conns) == 1) {
        cleanup_operations(ops); 
    }
    
    execute_scenario(ops, conns);

    clear(ops, conns);
    return 0;
}