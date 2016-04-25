#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
void getResultsAndPrintTrace(int*, int);
void printTrace(char*);
main(int argc, char *argv[]) {
	char buf[512];
	char host[64];
	int s, p, fp, rc, len, port;
	int max_players, hops;
	struct hostent *hp, *ihp;
	struct sockaddr_in sin, incoming;

	if (argc < 4) {
		fprintf(stderr, "Usage: %s <port-number> <number-of-players> <hops>\n",
				argv[0]);
		exit(1);
	}
	port = atoi(argv[1]);
	max_players = atoi(argv[2]);
	struct sockaddr_in players[max_players];
	int acceptReturns[max_players];
	char* playerLeftPorts[max_players];
	char* playerRightPorts[max_players];
	hops = atoi(argv[3]);

	gethostname(host, sizeof host);

	hp = gethostbyname(host);
	if (hp == NULL) {
		fprintf(stderr, "%s: host not found (%s)\n", argv[0], host);
		exit(1);
	}

	s = socket(AF_INET, SOCK_STREAM, 0);
	/*int boo = 1;
	 if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR|SO_REUSEPORT, &boo, sizeof(int)) == -1) {
	 perror("reuse:");
	 }*/
	if (s < 0) {
		perror("socket:");
		exit(s);
	}

	/* set up the address and port */
	sin.sin_family = AF_INET;
	sin.sin_port = htons(port);
	int boo = 1;
	if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &boo,
			sizeof(int)) < 0) {
		perror("reuse:");
	}
	memcpy(&sin.sin_addr, hp->h_addr_list[0], hp->h_length);
	/* bind socket s to address sin */
	rc = bind(s, (struct sockaddr *) &sin, sizeof(sin));
	if (rc < 0) {
		perror("bind:");
		exit(rc);
	}

	rc = listen(s, max_players);
	if (rc < 0) {
		perror("listen:");
		exit(rc);
	}
	int player_number = 0;

	/* accept connections */

	printf("Potato Master on %s\n", host);
	printf("Players = %d\n", max_players);
	printf("Hops = %d\n", hops);
	fflush(stdout);

	while (player_number < max_players) {
		len = sizeof(sin);
		p = accept(s, (struct sockaddr *) &incoming, &len);
		if (p < 0) {
			perror("bind:");
			exit(rc);
		}

		//receive port numbers
		char *ports = (char *) malloc(sizeof(char) * 64);
		memset(ports, '\0', 64);
		int len = recv(p, ports, 64, 0);
		//printf("%s\n", ports);

		acceptReturns[player_number] = p;
		playerRightPorts[player_number] = strtok(ports, "|");

		//printf("right port of %d : %s\n", player_number,
//				playerRightPorts[player_number]);
		playerLeftPorts[player_number] = strtok(NULL, "|");
		//printf("left port of %d : %s\n", player_number,
		//	playerLeftPorts[player_number]);

		ihp = gethostbyaddr((char *) &incoming.sin_addr, sizeof(struct in_addr),
		AF_INET);
		players[player_number] = incoming;
		printf("player %d is on %s\n", player_number, ihp->h_name);
		fflush(stdout);
		//send the players their numbers and total player number
		char number[81];
		memset(number, '\0', 66);
		char total[33];
		memset(total, '\0', 33);
		sprintf(number, "%d", player_number);
		sprintf(total, "%d", max_players);
		strcat(number, "|");
		strcat(number, total);

		send(acceptReturns[player_number], number, strlen(number), 0);

		if (player_number != 0) {
			char data[64];
			memset(data, '\0', sizeof data);
			strcpy(data, ihp->h_name);
			strcat(data, "|");
			strcat(data, playerLeftPorts[player_number]);

			send(acceptReturns[player_number - 1], data, sizeof(data), 0);

			struct hostent* prev = gethostbyaddr(
					(char *) &players[player_number - 1].sin_addr,
					sizeof(struct in_addr),
					AF_INET);

			memset(data, '\0', sizeof data);
			strcpy(data, prev->h_name);
			strcat(data, "|");
			strcat(data, playerRightPorts[player_number - 1]);
			//printf("sending %s from %d\n", playerRightPorts[player_number - 1],
			//	player_number);
			send(acceptReturns[player_number], data, sizeof(data), 0);

		}
		if (player_number == max_players - 1) {
			sleep(1);

			struct hostent* first = gethostbyaddr((char *) &players[0].sin_addr,
					sizeof(struct in_addr),
					AF_INET);

			char data[64];
			memset(data, '\0', sizeof data);
			strcpy(data, first->h_name);
			strcat(data, "|");
			strcat(data, playerLeftPorts[0]);

			send(acceptReturns[player_number], data, sizeof data, 0);

			struct hostent* last = gethostbyaddr(
					(char *) &players[player_number].sin_addr,
					sizeof(struct in_addr),
					AF_INET);

			memset(data, '\0', sizeof data);
			strcpy(data, last->h_name);
			strcat(data, "|");
			strcat(data, playerRightPorts[player_number]);

			send(acceptReturns[0], data, sizeof data, 0);

		}

		player_number++;
	}
	sleep(1);
	char command[5] = "stop";
	if (hops > 0) {

		char traceStart[100];
		memset(traceStart, '\0', 100);
		char hopstr[33];
		memset(hopstr, '\0', 33);
		sprintf(hopstr, "%d", hops);
		strcat(traceStart, hopstr);
		strcat(traceStart, "|");
		srand(max_players);
		int pick = rand() % max_players;
		printf("All players present, sending potato to player %d\n", pick);
		fflush(stdout);
		send(acceptReturns[pick], traceStart, strlen(traceStart), 0);

		getResultsAndPrintTrace(acceptReturns, max_players);

		int playerindex = 0;
		for (playerindex = 0; playerindex < max_players; playerindex++) {
			int sentAmount = send(acceptReturns[playerindex], command,
					strlen(command), 0);

		}

	} else {
		int playerindex = 0;
		for (playerindex = 0; playerindex < max_players; playerindex++) {
			int sentAmount = send(acceptReturns[playerindex], command,
					strlen(command), 0);

		}
	}
	//sleep(5);
	close(s);
	exit(0);
}

void getResultsAndPrintTrace(int* descriptors, int len) {
	fd_set readfds;
	FD_ZERO(&readfds);
	int maxDesc = -1;
	int i = 0;
	for (i = 0; i < len; i++) {
		FD_SET(descriptors[i], &readfds);
		if (maxDesc < descriptors[i]) {
			maxDesc = descriptors[i];
		}
	}
	int rv = select(maxDesc + 1, &readfds, NULL, NULL, NULL);
	if (rv == -1) {
		perror("select");
	} else if (rv == 0) {
		//printf("time out");
	} else {
		char lengthOfTrace[100];
		memset(lengthOfTrace, '\0', 100);
		for (i = 0; i < len; i++) {
			if (FD_ISSET(descriptors[i], &readfds)) {
				int len = recv(descriptors[i], lengthOfTrace, 100, 0);
				int expectedLength = atoi(lengthOfTrace);
				char result[expectedLength + 1];
				memset(result, '\0', expectedLength + 1);
				len = recv(descriptors[i], result, expectedLength, 0);
				printTrace(result);
			}
		}

		for (i = 0; i < len; i++) {
			send(descriptors[i], "stop", 5, 0);
		}
	}

}

void printTrace(char* result) {

	char* token = strtok(result, ">");
	printf("Trace of potato:\n");
	printf("%s", token);
	token = strtok(NULL, ">");
	while (token != NULL) {
		printf(",%s", token);
		token = strtok(NULL, ">");
	}
	printf("\n");
	fflush(stdout);
}

