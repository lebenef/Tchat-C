#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/socket.h>
#define BUFFER_SIZE 4096
#define MAX_CLIENTS 5
#define NEW_CLIENT "Bienvenue sur le tchat\n!  Pour quitter le tchat: /quit \nEntrez votre pseudo: "


volatile 
int nb_clients = 0;
int first_free = 0;


pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct _s_client
{
	pthread_t id;
	int sock;
	char *pseudo;
} 
	s_client;
	s_client *clients[MAX_CLIENTS];

int cree_socket_tcp_ip()
{
	int sock;
	struct sockaddr_in adresse;
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		fprintf(stderr, "Erreur socket\n");
		return -1;
	}
	memset(&adresse, 0, sizeof(struct sockaddr_in));
	adresse.sin_family = AF_INET;
	adresse.sin_port = htons(0);
	//adresse.sin_addr.s_addr = htons(INADDR_ANY);
    inet_aton("192.168.1.32", &adresse.sin_addr);
	if (bind(sock, (struct sockaddr*) &adresse,sizeof(struct sockaddr_in)) < 0)
	{
		close(sock);
		fprintf(stderr, "Erreur bind\n");
		return -1;
	}
	return sock;
}


int affiche_adresse_socket(int sock)
{
	struct sockaddr_in adresse;
	socklen_t longueur;
	longueur = sizeof(struct sockaddr_in);
	if (getsockname(sock, (struct sockaddr*)&adresse, &longueur) < 0)
	{
		fprintf(stderr, "Erreur getsockname\n");
		return -1;
	}
	printf("IP = %s, Port = %u\n", inet_ntoa(adresse.sin_addr),
	ntohs(adresse.sin_port));
	return 0;
}

int listen(int sockfd, int backlog);

int accept(int sockfd, struct sockaddr *addr,socklen_t *addrlen);

int getpeername(int sockfd, struct sockaddr *addr,socklen_t *addrlen);


int contactclient(int sock_connectee,char *buffer)
{
	return write(sock_connectee,buffer,BUFFER_SIZE);
}

int diffussion(char *msg, int not_to)
{
	int i;
	
	pthread_mutex_lock(&mutex);	// debut de la section critique
	for(i=0;i<first_free;i++)
	{
		if(clients[i]->sock != not_to)
			contactclient(clients[i]->sock,msg);
	}
	pthread_mutex_unlock(&mutex);	// fin de la section critique
	
	return 0;
}

void deconnec(s_client *me)
{
	int i,j;
	char buf[8192+1];
	snprintf(buf,8192,"Serveur : %s nous quitte...\r\n",me->pseudo);
	buf[8192] = '\0';
	diffussion(buf,me->sock);
	pthread_mutex_lock(&mutex);	// debut de la section critique
	for(i=0;(clients[i]->sock != me->sock);i++);	// recherche de l'index de la structure dans le tableau
	
	close(me->sock);
	free(me->pseudo);
	free(me);
	
	for(j=i+1;j<first_free;j++)	// on reorganise le tableau en decalant les elements situes APRES celui qui est supprime
	{
		clients[j-1] = clients[j];
	}
	nb_clients--;
	first_free--;
	pthread_mutex_unlock(&mutex);	// fin de la section critique
	printf("Un client en moins...%d clients\n",nb_clients);
}


void * servsend() 
{
	char buffer[4096];
	char broad[8192];

	 for (;;) 
	 {


	read(STDOUT_FILENO, buffer, BUFFER_SIZE);


	if(!strncmp(buffer,"/quit",5))
	{

		diffussion(buffer,-1);
 		exit(0);
	}
	else
	{

	snprintf(broad,8192,"Serveur :%s\r\n",buffer);
	broad[8192]='\0';
	diffussion(broad,-1);
    }
}
}

void *traite_connection(void *sock)
{
	int sock_connectee = *((int *) sock);

	int n;
    int i;
    int rec;
		pthread_t rThread;

    s_client *me = NULL;
	char buffer[4096];
	char broad[8192];
	char quit;

	me = (s_client *) malloc(sizeof(s_client));

	if(!me)
	{
		printf("\nErreur d'allocation memoire!\n");
		close(sock_connectee);
		nb_clients--;
		pthread_exit(NULL);
	}

	bzero(me,sizeof(s_client));    
    memset(buffer, 0, BUFFER_SIZE);  
	contactclient(sock_connectee,NEW_CLIENT);
	memset(buffer, 0, BUFFER_SIZE);  
	n = read(sock_connectee,buffer,BUFFER_SIZE);
	printf("%s\n",buffer );

	if(n <= 0)
	{
		printf("\nErreur\n");
		close(sock_connectee);
		free(me);
		me = NULL;
		nb_clients--;
		pthread_exit(NULL);
	}

	buffer[4096] = '\0';	// on limite le pseudo a 255 caracteres
	for(i=0;(buffer[i]!='\0') && (buffer[i]!='\r') && (buffer[i]!='\n') && (buffer[i]!='\t');i++);
	buffer[i] = '\0';	// on isole le pseudo
	
	me->id = pthread_self();
	me->sock = sock_connectee;
	me->pseudo = strdup(buffer);

	pthread_mutex_lock(&mutex);	// debut de la section critique
	clients[first_free] = me;
	first_free++;
	pthread_mutex_unlock(&mutex);	// fin de la section critique

	snprintf(broad,8192,"Serveur : Nouveau client -> %s\r\n",me->pseudo);
	broad[8192]='\0';
	diffussion(broad,-1);

	 while (1)
	    { 

	        memset(buffer, 0, BUFFER_SIZE);

	          

    	  rec = pthread_create(&rThread, NULL, servsend, NULL);
		  if (rec) 
		  {
			  printf("Erreur");
			  exit(1);
		  }

			n = read(sock_connectee,buffer,4096);
			if(n <= 0)
			{
				deconnec(me);
				pthread_exit(NULL);
			}

			if(!strncmp(buffer,"/quit",5))	// sortie "propre" du serveur (avec message)
			{
				int i;
				
				if(buffer[5]=='=')
				{
					for(i=6;(buffer[i]!='\0') && (buffer[i]!='\r') && (buffer[i]!='\n') && (buffer[i]!='\t');i++);
					buffer[i]='\0';
					deconnec(me);
				}
				else 
				{
					deconnec(me);
					pthread_exit(NULL);
                }
            }
			else
			{
	          	snprintf(broad,8192,"%s : %s",me->pseudo,buffer);
				broad[8192] = '\0';
				printf("%s\n",broad);
				diffussion(broad,me->sock);
			}
		}
	                                     

	return NULL;
}


int main(void)
{    
	int sock_contact;
	int sock;
	struct sockaddr_in adresse;
	socklen_t longueur;
	pthread_t th_id;
	sock_contact = cree_socket_tcp_ip();
	if (sock_contact < 0)
	return -1;

	listen(sock_contact, 5);
	printf("Mon adresse (sock contact) ->");
	affiche_adresse_socket(sock_contact);
	printf("Pour fermer le serveur utiliser la commande /quit");
	
		while (1)
		{
			longueur = sizeof(struct sockaddr_in); 
			sock = accept(sock_contact,(struct sockaddr*)&adresse,&longueur);

			if (sock < 0)
			{
				fprintf(stderr, "Erreur accept\n");
				return -1;
			}
			printf("Client connecté\n");

			if(nb_clients < MAX_CLIENTS)
			{   
				pthread_create(&th_id,NULL,traite_connection,(void *)&sock);
				nb_clients++;
				printf("Nouveau client! %d clients \n",nb_clients);

			}
		
			else
				close(sock);

		}

return 0;
}