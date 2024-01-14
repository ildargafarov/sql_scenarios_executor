#include <stdio.h>
#include <stdlib.h>
#include "utils.h"

void do_exit(PGconn *conn) {
    PQfinish(conn);
    exit(1);
}

PGconn* do_connect(void) {
    const char *PG_HOST_ADDR = getenv("POSTGRES_HOST_ADDR");
    const char *PG_PORT = getenv("POSTGRES_PORT");
    const char *PG_USER = getenv("POSTGRES_USER");
    const char *PG_PASS = getenv("POSTGRES_PASSWORD");
    const char *PG_DB = getenv("POSTGRES_DB");

    char conn_params[256];
    sprintf(
        conn_params, 
        "hostaddr=%s port=%s dbname=%s user=%s password=%s",
        PG_HOST_ADDR, PG_PORT, PG_DB, PG_USER, PG_PASS
    );
    PGconn *conn = PQconnectdb(conn_params);
    if (PQstatus(conn) == CONNECTION_BAD) {
        fprintf(stderr, "Connection to database failed: %s\n", PQerrorMessage(conn));
        do_exit(conn);
    }
    return conn;
}