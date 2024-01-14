#include <stdio.h>
#include <stdlib.h>
#include <libpq-fe.h>
#include "utils.h"

int main() {
    PGconn *conn = do_connect();

    int ver = PQserverVersion(conn);
    printf("Server version: %d\n", ver);
    PQfinish(conn);

    return 0;
}