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

struct	ACTUATEURS 
{
		 unsigned CP : 2; //Chariot
		 unsigned C1 : 1; //Conv1
		 unsigned C2 : 1; //Conv2
		 unsigned PV : 1; //Ventouse
		 unsigned PA : 1; //Truc sur le chariot
		 unsigned AA : 1; //Arbre
		 unsigned GA : 1; //Grapin sur Arbre
} ;

struct 	CAPTEURS
{
		 unsigned L1 : 1;
		 unsigned L2 : 1;
		 unsigned T  : 1;	// cable H
		 unsigned S  : 1;
		 unsigned AP : 1;
		 unsigned PP : 1;
		 unsigned DE : 1;
		 unsigned CS : 1;	// anciennement CS
/*       unsigned H1 : 1;
		 unsigned H2 : 1;
		 unsigned H3 : 1;
		 unsigned H4 : 1;
		 unsigned H5 : 1;
		 unsigned H6 : 1;
		 unsigned    : 2 ;	
*/
} ;

union
{
	struct ACTUATEURS act ;
	unsigned char byte ;
} u_act ;

union
{
	struct CAPTEURS capt ;
	unsigned char byte ;
} u_capt ;


#define PORT 40000
#define MAXSTRING 1
#define nbConnexion 1

int initSocket(const char *IP, int port, struct sockaddr_in *adresseSocket);
/* Creates and returns a server socket for the machine identified by hostname on the Network Port port */
int createServerSocket(const char *IP, int port, struct sockaddr_in *adresseSocket);
void listenSocket(int hSocket);
int acceptSocket(int hSocket, struct sockaddr *adresseSocket);

void* threadCapteur(void *);
void ouverturePetra();
void snd(int msg);
int rcv();
int menuPetra( int choix);

int hSocketService, hSocketEcoute;
int captors_in, actuators_out ;

int main()
{
	int quitter = 0;
	struct sockaddr_in adresseSocket;
	struct in_addr adresseIP;
	struct hostent *infosHost;
	pthread_t tid;
	char *IP = malloc(17); char msg;
	
	// Informations machine
	if ((infosHost = gethostbyname("ubuntu")) == 0)
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
	//ouverturePetra();

	while (1)
	{
		listenSocket(hSocketEcoute);
		printf("listen OK\n");
		if ((hSocketService = acceptSocket(hSocketEcoute, (struct sockaddr *)&adresseSocket))== -1)
		{
			printf("erreur accept : %s\n", strerror(errno));
			exit(1);
		}
		printf("accept OK\n");
		pthread_create(&tid,NULL,threadCapteur,&hSocketService);
		
		while(quitter != 8)
		{
			msg=rcv();
			//quitter = menuPetra(msg);
		}
		
		pthread_kill(tid, SIGUSR1);
	}
	
	return 0;
}

int menuPetra( int choix)
{
	read ( actuators_out, &u_act.byte , 1 );
							
	
	printf("on a recu : %dG\n",choix);
	fflush(stdin);
	
	
	if(choix==1)
	{
		printf("IF 1");
        if (u_act.act.C1 == 0)
            u_act.act.C1 = 1;
        else
            u_act.act.C1 = 0;
        write(actuators_out, &u_act.byte, 1);
	}
	else if(choix==2)
	{
		printf("IF 2");
		if (u_act.act.C2 == 0)
            u_act.act.C2 = 1;
        else
            u_act.act.C2 = 0;
        write(actuators_out, &u_act.byte, 1);
	}
	else if(choix==3)
	{
		printf("IF 3");
		if (u_act.act.PV == 0)
            u_act.act.PV = 1;
        else
            u_act.act.PV = 0;
        write(actuators_out, &u_act.byte, 1);
	}
	else if(choix==4)
	{
		printf("IF 4");
		if (u_act.act.PA == 0)
            u_act.act.PA = 1;
        else
            u_act.act.PA = 0;
        write(actuators_out, &u_act.byte, 1);
	}
	else if(choix==5)
	{
		printf("IF 5");
		if (u_act.act.AA == 0)
            u_act.act.AA = 1;
        else
            u_act.act.AA = 0;
        write(actuators_out, &u_act.byte, 1);
	}
	else if(choix==6)
	{
		printf("IF 6");
		if (u_act.act.GA == 0)
            u_act.act.GA = 1;
        else
            u_act.act.GA = 0;
        write(actuators_out, &u_act.byte, 1);
	}
	else if(choix==7)
	{
		printf("\33[2J]");
        printf("--------Position du bras--------");
        printf("\n1. Réservoir");
        printf("\n2. Tapis 1");
        printf("\n3. Entre les 2 tapis");
        printf("\n4. Tapis 2");
        printf("\n--------------------------------\n");

        fflush(stdin);
        choix=rcv();
        switch (choix)
        {
		case 1:
			if (u_act.act.CP != 0)
				u_act.act.CP = 0; // 10
			break;
		case 2:    
			if (u_act.act.CP != 1)
				u_act.act.CP = 1; // 01
			break;
		case 3:
			if (u_act.act.CP != 2)
				u_act.act.CP = 2; // 10
			break;
		case 4:
			if (u_act.act.CP != 3)
				u_act.act.CP = 3; // 11
			
        }
        write(actuators_out, &u_act.byte, 1); 
	}
	return choix;
}


void* threadCapteur(void *s)
{
	int* temp,hSocketService,i=0;
	
	temp=(int*)s;
	hSocketService=*temp;
	
	while(1)
	{
    
		read ( captors_in , &u_capt.byte , 1 );

		snd(u_capt.capt.DE);
		snd(u_capt.capt.CS);
		snd(u_capt.capt.PP);
		snd(u_capt.capt.S);
		snd(u_capt.capt.L1);
		snd(u_capt.capt.L2);
		snd(u_capt.capt.AP);	
	}
}
void snd(int msg)
{	
	char t[1];
	
	sprintf(t,"%d",msg);
	printf("on envoit : %s\n",t);
	printf("int : %d\n",t[0]);
	
	if(send(hSocketService,t,MAXSTRING,0) == -1 )
	{
		printf("erreur send : %s\n",strerror(errno));
		exit(1);
	}	
}
void ouverturePetra()
{
	actuators_out = open ( "/dev/actuateursPETRA", O_WRONLY );
	if ( actuators_out == -1 ) 
	{
		perror ( "MAIN : Erreur ouverture PETRA_OUT" );
		exit(1) ;
	}
	else
		printf ("MAIN: PETRA_OUT opened\n");


	captors_in = open ( "/dev/capteursPETRA", O_RDONLY );
	if ( captors_in == -1 ) 
	{
		perror ( "MAIN : Erreur ouverture PETRA_IN" );
		exit(1) ;
	}
	else
		printf ("MAIN: PETRA_IN opened\n");
}

int rcv()
{
	int m;
	
	if(recv(hSocketService,&m,MAXSTRING,0) == -1 )
	{
		printf("erreur recv : %s\n",strerror(errno));
		exit(1);
	}
	printf("recu via fct : %d\n",m);
	
	return m;	
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
		switch(errno)
		{
			case EADDRINUSE: printf("Error: Socket is in use\n");
				break;
			case EINVAL: printf("Error: Socket already bound to an address\n");
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