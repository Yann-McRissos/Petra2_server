#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <errno.h>
#include <sys/types.h>
#include <netdb.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <termios.h>
#include <pthread.h>
#include <signal.h>

struct ACTUATEURS
{
	unsigned CP : 2; //Chariot
	unsigned C1 : 1; //Conv1
	unsigned C2 : 1; //Conv2
	unsigned PV : 1; //Ventouse
	unsigned PA : 1; //Truc sur le chariot
	unsigned AA : 1; //Arbre
	unsigned GA : 1; //Grapin sur Arbre
};

struct CAPTEURS
{
	unsigned L1 : 1;
	unsigned L2 : 1;
	unsigned T : 1; /* cable H */
	unsigned S : 1;
	unsigned AP : 1;
	unsigned PP : 1;
	unsigned DE : 1;
	unsigned CS : 1; //anciennement CS
					 /*       unsigned H1 : 1;
		 unsigned H2 : 1;
		 unsigned H3 : 1;
		 unsigned H4 : 1;
		 unsigned H5 : 1;
		 unsigned H6 : 1;
		 unsigned    : 2 ;	
*/
};

union
{
	struct ACTUATEURS act;
	unsigned char byte;
} u_act;

union
{
	struct CAPTEURS capt;
	unsigned char byte;
} u_capt;

union
{
	struct CAPTEURS capt;
	unsigned char byte;
} u_capt_oldvalue;

// Actuateurs
#define CHARIOT u_act.act.CP
#define CONVOYEUR1 u_act.act.C1
#define CONVOYEUR2 u_act.act.C2
#define VENTOUSE u_act.act.PV
#define PLONGEUR u_act.act.PA
#define ARBRE u_act.act.AA
#define GRAPIN u_act.act.GA
// Capteurs
#define L1 u_capt.capt.L1
#define L2 u_capt.capt.L2
#define EPAISSEUR u_capt.capt.T
#define SLOT u_capt.capt.S
#define CHARIOTSTABLE u_capt.capt.CS
#define BRAS u_capt.capt.AP
#define CAPTPLONGEUR u_capt.capt.PP
#define BAC u_capt.capt.DE

#define PORT 40004
#define MAXSTRING 1
#define nbConnexion 1

int initSocket(const char *IP, int port, struct sockaddr_in *adresseSocket);
int createServerSocket(const char *IP, int port, struct sockaddr_in *adresseSocket);
void listenSocket(int hSocket);
int acceptSocket(int hSocket, struct sockaddr *adresseSocket);

void handlerSIGINT(int sig);
void *threadCapteurs(void *);
void ouverturePetra();
void sendMsg(char msg);
int recvMsg();
int menuPetra(int actuateur);

int hSocketService, hSocketEcoute;
int fd_petra_in, fd_petra_out;
struct sigaction act;

int main()
{
	int quitter = 1, msg;
	struct sockaddr_in adresseSocket;
	struct in_addr adresseIP;
	struct hostent *infosHost;
	pthread_t tid;
	char *IP = malloc(17);

	// Informations machine
	if ((infosHost = gethostbyname("192.168.1.29")) == 0)
	{
		printf("Erreur d'acquisition d'infos sur le host %d\n", errno);
		exit(1);
	}
	memcpy(&adresseIP, infosHost->h_addr, infosHost->h_length);
	strcpy(IP, inet_ntoa(adresseIP));
	printf("Server IP = %s\n", IP);
	// Création socket
	hSocketEcoute = createServerSocket(IP, PORT, (struct sockaddr_in *)&adresseSocket);
	printf("Creation socket OK\n");

	// Armement sur le signal SIGINT
	act.sa_handler = handlerSIGINT;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	sigaction(SIGINT, &act, 0);
	// Ouverture de la communication avec le PETRA
	//ouverturePetra();

	while (1)
	{
		listenSocket(hSocketEcoute);
		printf("listen OK\n");
		if ((hSocketService = acceptSocket(hSocketEcoute, (struct sockaddr *)&adresseSocket)) == -1)
		{
			printf("erreur accept : %s\n", strerror(errno));
			exit(1);
		}
		printf("accept OK\n");
		if (pthread_create(&tid, NULL, threadCapteurs, &hSocketService) == -1)
		{
			printf("Erreur de creation de thread : %s\n", strerror(errno));
			exit(1);
		}
		printf("Thread lance\n");

		while (quitter)
		{
			if((msg = recvMsg()) == -1)
				printf("Erreur de reception: %d\n", errno);
			quitter = menuPetra(msg);
		}
		close(hSocketService);
		pthread_kill(tid, SIGUSR1);
	}

	pthread_kill(tid, SIGUSR1);
	close(hSocketEcoute);
	close(hSocketService);
	return 0;
}

int menuPetra(int actuateur)
{
	fflush(stdin);
	// pas nécessaire
	//read(fd_petra_out, &u_act.byte, 1);
	switch (actuateur)
	{
	case 0: // QUITTER
		return 0;
		break;
	case 1: // Conv1
		printf("Convoyeur1\n");
		if (u_act.act.C1 == 0)
			u_act.act.C1 = 1;
		else
			u_act.act.C1 = 0;
		//write(fd_petra_out, &u_act.byte, 1);
		break;
	case 2: // Conv2
		printf("Convoyeur2\n");
		if (u_act.act.C2 == 0)
			u_act.act.C2 = 1;
		else
			u_act.act.C2 = 0;
		//write(fd_petra_out, &u_act.byte, 1);
		break;
	case 3: // Ventouse
		printf("Ventouse\n");
		if (u_act.act.PV == 0)
			u_act.act.PV = 1;
		else
			u_act.act.PV = 0;
		//write(fd_petra_out, &u_act.byte, 1);
		break;
	case 4: // Plongueur
		printf("Plongeur\n");
		if (u_act.act.PA == 0)
			u_act.act.PA = 1;
		else
			u_act.act.PA = 0;
		//write(fd_petra_out, &u_act.byte, 1);
		break;
	case 5: // Arbre
		printf("Arbre\n");
		if (u_act.act.AA == 0)
			u_act.act.AA = 1;
		else
			u_act.act.AA = 0;
		//write(fd_petra_out, &u_act.byte, 1);
		break;
	case 6: // Grappin
		printf("Grappin\n");
		if (u_act.act.GA == 0)
			u_act.act.GA = 1;
		else
			u_act.act.GA = 0;
		//write(fd_petra_out, &u_act.byte, 1);
		break;
	case 7: // Réservoir
		printf("Chariot: Reservoir\n");
		if ((u_act.act.CP & 0x11) == 0)
			u_act.act.CP |= 0x00;
		else
			u_act.act.CP &= 0x11;
		//write(fd_petra_out, &u_act.byte, 1);
		break;
	case 8: // Tapis 1
		printf("Chariot: Tapis 1\n");
		if ((u_act.act.CP & 0x10) == 0)
			u_act.act.CP |= 0x01;
		else
			u_act.act.CP &= 0x10;
		//write(fd_petra_out, &u_act.byte, 1);
		break;
	case 9: // Bac KO
		printf("Chariot: Bac KO\n");
		if ((u_act.act.CP & 0x01) == 0)
			u_act.act.CP |= 0x10;
		else
			u_act.act.CP &= 0x01;
		//write(fd_petra_out, &u_act.byte, 1);
		break;
	case 10: // Tapis 2
		printf("Chariot: Tapis 2\n");
		if ((u_act.act.CP & 0x00) == 0)
			u_act.act.CP |= 0x11;
		else
			u_act.act.CP &= 0x00;
		//write(fd_petra_out, &u_act.byte, 1);
		break;
	}

	return actuateur;
}

void *threadCapteurs(void *s)
{
	printf("THREAD: Debut...\n");
	/*int *temp, socketClient;
	temp = (int *)s;
	socketClient = *temp;*/

	u_capt_oldvalue.byte = 0x00;
	while (1)
	{
		sleep(4);
		//read(fd_petra_in, &u_capt.byte, 1);
		u_capt.byte = rand() % 8;
		if(u_capt_oldvalue.byte != u_capt.byte)
		{
			switch (u_capt.byte)
			{
			case 0x00:
				printf("L1\n");
				sendMsg((char)0);
				break;
			case 0x01:
				printf("L2\n");
				sendMsg(1);
				break;
			case 0x02:
				printf("EPAISSEUR\n");
				sendMsg(2);
				break;
			case 0x03:
				printf("SLOT\n");
				sendMsg(3);
				break;
			case 0x04:
				printf("CHARIOTSTABLE\n");
				sendMsg(4);
				break;
			case 0x05:
				printf("BRAS\n");
				sendMsg(5);
				break;
			case 0x06:
				printf("CAPTEURPLONGEUR\n");
				sendMsg(6);
				break;
			case 0x07:
				printf("BAC\n");
				sendMsg(7);
				break;
			}
			u_capt_oldvalue.byte = u_capt.byte;
		}
	}
}

void sendMsg(char msg)
{
	char t[1];

	sprintf(t, "%d", msg);
	printf("on envoit : %s\n", t);
	printf("int : %d\n", t[0]);

	if (send(hSocketService, &msg, MAXSTRING, 0) == -1)
	{
		printf("erreur send : %s\n", strerror(errno));
		exit(1);
	}
}

int recvMsg()
{
	char message;

	if (recv(hSocketService, &message, MAXSTRING, 0) == -1)
	{
		printf("erreur recv : %s\n", strerror(errno));
		exit(1);
	}
	printf("Message recu: %d\n", message);

	return message;
}

void ouverturePetra()
{
	fd_petra_out = open("/dev/actuateursPETRA", O_WRONLY);
	if (fd_petra_out == -1)
	{
		perror("MAIN : Erreur ouverture PETRA_OUT");
		exit(1);
	}
	else
		printf("MAIN: PETRA_OUT opened\n");

	fd_petra_in = open("/dev/capteursPETRA", O_RDONLY);
	if (fd_petra_in == -1)
	{
		perror("MAIN : Erreur ouverture PETRA_IN");
		exit(1);
	}
	else
		printf("MAIN: PETRA_IN opened\n");
}

int initSocket(const char *IP, int port, struct sockaddr_in *adresseSocket)
{
	int tempSocket;

	/* Creation de la socket */
	tempSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (tempSocket == -1)
	{
		printf("Erreur de creation de la socket %d\n", errno);
		exit(1);
	}
	//printf("socket() ok\n");

	/* Acquisition des informations sur l'ordinateur local ou distant */
	/* Préparation de la structure sockaddr_in */
	memset(adresseSocket, 0, sizeof(struct sockaddr_in));
	(*adresseSocket).sin_family = AF_INET;
	(*adresseSocket).sin_port = htons(port);
	(*adresseSocket).sin_addr.s_addr = htons(INADDR_ANY); //inet_addr(IP);
	//printf("Infohost-IP: %s\n", inet_ntoa((*adresseSocket).sin_addr));

	return tempSocket;
}

int createServerSocket(const char *IP, int port, struct sockaddr_in *adresseSocket)
{
	int hSocket;

	hSocket = initSocket(IP, port, (struct sockaddr_in *)adresseSocket);
	/* Le système prend connaissance de l'adresse et du port de la socket */
	if (bind(hSocket, (struct sockaddr *)adresseSocket, sizeof(struct sockaddr_in)) == -1)
	{
		switch (errno)
		{
		case EADDRINUSE:
			printf("Error: Socket is in use\n");
			break;
		case EINVAL:
			printf("Error: Socket already bound to an address\n");
			break;
		}
		exit(1);
	}
	//printf("Bind adresse et port socket OK\n");

	return hSocket;
}

void listenSocket(int hSocket)
{
	if (listen(hSocket, SOMAXCONN) == -1)
	{
		printf("Erreur sur lel isten de la socket %d\n", errno);
		close(hSocket); /* Fermeture de la socket */
		exit(1);
	}
	//printf("Listen socket OK\n");
}

int acceptSocket(int hSocket, struct sockaddr *adresseSocket)
{
	int hSocketService;
	unsigned tailleSockaddr_in;
	tailleSockaddr_in = sizeof(struct sockaddr_in);
	if ((hSocketService = accept(hSocket, (struct sockaddr *)&adresseSocket, &tailleSockaddr_in)) == -1)
	{
		printf("Erreur sur l'accept de la socket %d\n", errno);
		close(hSocket);
		exit(1);
	}
	//printf("Accept socket OK\n");

	return hSocketService;
}

void handlerSIGINT(int sig)
{
	printf("\nHANDLER: reçu %d\n", sig);
	close(hSocketEcoute);
	close(hSocketService);
	printf("Sockets fermees\n");
	puts("Fin du serveur...\n");
	exit(1);
}