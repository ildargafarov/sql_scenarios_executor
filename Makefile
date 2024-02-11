.EXPORT_ALL_VARIABLES:

POSTGRES_HOST_ADDR=127.0.0.1
POSTGRES_PORT=5432
POSTGRES_USER=postgres
POSTGRES_PASSWORD=postgres
POSTGRES_DB=sql_scenarios
POSTGRES_CONTAINER_NAME=scenarios-postgres


show_lib_version:
	gcc -o lib_version lib_version.c -I/usr/include -lpq
	./lib_version


pg_docker_start:
	docker run --name ${POSTGRES_CONTAINER_NAME} --rm -e POSTGRES_PASSWORD=${POSTGRES_PASSWORD} -e POSTGRES_USER=${POSTGRES_USER} -e POSTGRES_DB=${POSTGRES_DB} -p ${POSTGRES_PORT}:${POSTGRES_PORT} -d postgres


pg_docker_stop:
	docker stop ${POSTGRES_CONTAINER_NAME}


show_server_version:
	gcc -o server_version server_version.c utils.c -I/usr/include -lpq
	./server_version


test_dumb_scenario:
	gcc -o scenarios_adapter scenarios_adapter.c utils.c scenarios.c -I/usr/include -lpq -ljson-c
	./scenarios_adapter

run:
	gcc -g -pthread -o sql_scenarios_executor sql_scenarios_executor.c scenarios.c -I/usr/include -lpq -ljson-c
	./sql_scenarios_executor --scenario "$(scenario)"

debug:
	gcc -g -pthread -o sql_scenarios_executor sql_scenarios_executor.c scenarios.c -I/usr/include -lpq -ljson-c
	gdb --args ./sql_scenarios_executor --scenario "$(scenario)"
