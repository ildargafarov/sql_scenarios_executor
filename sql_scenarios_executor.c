#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <libpq-fe.h>
#include <pthread.h>
#include "scenarios.h"

#define PG_CONN_PARAMS_MAX_LEN 256

#define WORKER_READY 0
#define WORKER_BUSY 1

static pthread_cond_t worker_started = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t worker_started_mutex = PTHREAD_MUTEX_INITIALIZER;

static int finished = 0;
static operation_t *current_op = NULL; 

static pthread_cond_t worker_busy = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t worker_busy_mutex = PTHREAD_MUTEX_INITIALIZER;

static int * busy_workers;

static pthread_mutex_t print_mutex = PTHREAD_MUTEX_INITIALIZER;

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
    for(int i = 0; i < ops->size; i++) {
        if (ops->operations[i]->id > max_id) {
            max_id = ops->operations[i]->id;
        }
    }
    return max_id + 1;
}

struct Worker {
    int id;
    pthread_t tid;
    PGconn *conn;
};

static void* start_worker(void *arg) {
    struct Worker *worker = (struct Worker *) arg;
    operation_t *op = NULL;
    for(;;) {
        pthread_mutex_lock(&worker_busy_mutex);
        busy_workers[worker->id] = WORKER_READY;
        pthread_mutex_unlock(&worker_busy_mutex);
        pthread_cond_broadcast(&worker_busy);

        pthread_mutex_lock(&worker_started_mutex);
        while(current_op == NULL || current_op->id != worker->id || op == current_op) {
            if (finished == 1) {
                pthread_mutex_unlock(&worker_started_mutex);
                return NULL;
            }
            pthread_cond_wait(&worker_started, &worker_started_mutex); 
        }
        op = current_op;
        pthread_mutex_unlock(&worker_started_mutex);

        pthread_mutex_lock(&worker_busy_mutex);
        busy_workers[worker->id] = WORKER_BUSY;
        pthread_mutex_unlock(&worker_busy_mutex);
        pthread_cond_broadcast(&worker_busy);

        pthread_mutex_lock(&print_mutex);
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
        pthread_mutex_unlock(&print_mutex);

        PGresult* res = PQexec(worker->conn, op->query);
        if (PQresultStatus(res) == PGRES_FATAL_ERROR) {
            pthread_mutex_lock(&print_mutex);
            fprintf(stderr, "Query failed: %s\n", PQresultErrorMessage(res));
            fprintf(stderr, "Conn status: %s\n",PQerrorMessage(worker->conn));
            pthread_mutex_unlock(&print_mutex);
            PQclear(res);
            return NULL;
        }
        int rows = PQntuples(res);
        int cols = PQnfields(res);
        pthread_mutex_lock(&print_mutex);
        for (int row = 0; row < rows; row++) {
            printf("   ------ [%d] ------\n", row);
            for (int col = 0; col < cols; col++) {
                printf("   %s: %s\n", PQfname(res, col), PQgetvalue(res, row, col));
            }
        }
        printf("\n");
        pthread_mutex_unlock(&print_mutex);
        PQclear(res);

        pthread_mutex_lock(&worker_busy_mutex);
        busy_workers[worker->id] = WORKER_READY;
        pthread_mutex_unlock(&worker_busy_mutex);
        pthread_cond_broadcast(&worker_busy);
    }

    return NULL;
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
    struct Worker **workers = malloc(sizeof(struct Worker *) * ids_number);
    busy_workers = malloc(sizeof(int) * ids_number);
    char* conn_params = pg_conn_params();

    for (int i = 0; i < ids_number; i++) {
        workers[i] = malloc(sizeof(struct Worker));
        workers[i]->id = i;
        workers[i]->conn = PQconnectdb(conn_params);
        if (PQstatus(workers[i]->conn) == CONNECTION_BAD) {
            fprintf(stderr, "Connection to database failed: %s\n", PQerrorMessage(workers[i]->conn));
            PQfinish(workers[i]->conn);
            break;
        }
        if (pthread_create(&workers[i]->tid, NULL, start_worker, (void *) workers[i]) != 0) {
            fprintf(stderr, "Thread creation failure\n");
            break;
        }
    }
    free(conn_params);

    for(int i = 0; i < ops->size; i++) {
        if (i != 0) {
            pthread_mutex_lock(&worker_busy_mutex);
            while(busy_workers[ops->operations[i]->id] != WORKER_READY) {
                pthread_cond_wait(&worker_busy, &worker_busy_mutex); 
            }
            pthread_mutex_unlock(&worker_busy_mutex);
        }

        pthread_mutex_lock(&worker_started_mutex);
        current_op = ops->operations[i];
        pthread_mutex_unlock(&worker_started_mutex);
        pthread_cond_broadcast(&worker_started);

        pthread_mutex_lock(&worker_busy_mutex);
        while(busy_workers[ops->operations[i]->id] != WORKER_BUSY) {
            pthread_cond_wait(&worker_busy, &worker_busy_mutex); 
        }
        pthread_mutex_unlock(&worker_busy_mutex);

        if (ops->operations[i]->wait) {
            pthread_mutex_lock(&worker_busy_mutex);
            while(busy_workers[ops->operations[i]->id] == WORKER_BUSY) {
                pthread_cond_wait(&worker_busy, &worker_busy_mutex); 
            }
            pthread_mutex_unlock(&worker_busy_mutex);
        }
    }
    pthread_mutex_lock(&worker_started_mutex);
    current_op = NULL;
    finished = 1;
    pthread_mutex_unlock(&worker_started_mutex);
    pthread_cond_broadcast(&worker_started);
    cleanup_operations(ops);
    free(workers);
    free(busy_workers);
    return 0;
}