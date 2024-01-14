#include <libpq-fe.h>

void do_exit(PGconn *conn);

PGconn* do_connect(void);