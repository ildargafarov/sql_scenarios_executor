#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <libpq-fe.h>
#include "scenarios.h"

char* pg_conn_params() {
    const char *PG_HOST_ADDR = getenv("POSTGRES_HOST_ADDR");
    const char *PG_PORT = getenv("POSTGRES_PORT");
    const char *PG_USER = getenv("POSTGRES_USER");
    const char *PG_PASS = getenv("POSTGRES_PASSWORD");
    const char *PG_DB = getenv("POSTGRES_DB");

    char* conn_params = malloc(sizeof(char) * 256);
    sprintf(
        conn_params, 
        "hostaddr=%s port=%s dbname=%s user=%s password=%s",
        PG_HOST_ADDR, PG_PORT, PG_DB, PG_USER, PG_PASS
    );
    return conn_params;
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
    
    char* conn_params = pg_conn_params();
    PGconn* conns[ops->size];
    for (int i = 0; i < ops->size; i++) {
        PGconn *conn = PQconnectdb(conn_params);
        if (PQstatus(conn) == CONNECTION_BAD) {
            fprintf(stderr, "Connection to database failed: %s\n", PQerrorMessage(conn));
            PQfinish(conn);
            // TODO free well
            exit(1);
        }
        conns[i] = conn;
    }
    free(conn_params);
    
    operation_t *op = ops->first;
    while(op) {
        printf("========= [%d] ==========\n=> %s\n", op->id, op->query);
        PGresult* res = PQexec(conns[op->id], op->query);
        if (PQresultStatus(res) == PGRES_FATAL_ERROR) {
            fprintf(stderr, "Query failed: %s\n", PQresultErrorMessage(res));
            PQclear(res);
            // TODO free well
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

    for (int i = 0; i < ops->size; i++) {
        PQfinish(conns[i]);
    }
    cleanup_operations(ops); 
    // TODO Segmentation fault? 
    return 0;
}