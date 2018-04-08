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

sem_t sem;


int connect(int sockfd, const struct sockaddr *addr,socklen_t addrlen);

int cree_socket_tcp_client(int argc,char** argv)
{
	struct sockaddr_in adresse;
	int sock;
	if (argc != 3)
	{
		fprintf(stderr, "Usage : %s adresse : %s port : %s \n",argv[0],argv[1],argv[2]);
		exit(0);
	}
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		fprintf(stderr, "Erreur socket\n");
		return -1;
	}
	memset(&adresse, 0, sizeof(struct sockaddr_in));
	adresse.sin_family = AF_INET;
	adresse.sin_port = htons(atoi(argv[2]));
	inet_aton(argv[1], &adresse.sin_addr);

	if (connect(sock, (struct sockaddr*) &adresse,sizeof(struct sockaddr_in)) < 0)
	{
		close(sock);
		fprintf(stderr, "Erreur connect\n");
		return -1;
	}
	printf("Connecté au serveur \n");
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



void * recevoir(void * sock_contact) 
{

	 int sockfd, ret;
	 sockfd = *((int *) sock_contact);
	 char buffer[BUFFER_SIZE]; 
	 memset(buffer, 0, BUFFER_SIZE);  

	 for (;;) 
	 {
	  	ret = read(sockfd, buffer, BUFFER_SIZE);  
		if (ret < 0) 
		{  
			printf("Erreur message non reçu!\n");    
		}
		if(!strncmp(buffer,"/quit",5))
		{
			printf("Serveur : Le Serveur est fermée désolé\n");
 				exit(0);
		}
		else 
		{
   			printf("%s\n",buffer);
	  	}  
	 }
}


	int main(int argc,char** argv)
	{ 
		int n=0;
		int rec;
		char buffer[4096];
		pthread_t rThread;
		sem_init(&sem, 0, 1);


		int sock_contact;
	    sock_contact = cree_socket_tcp_client( argc, argv);

		printf("Mon adresse (sock contact) ->");
		affiche_adresse_socket(sock_contact);

	   	read(sock_contact, buffer, BUFFER_SIZE);
    	printf("%s \n", buffer);


		n = read(STDOUT_FILENO, buffer, BUFFER_SIZE);
		write(sock_contact, buffer, n);




       while (1)
   	   {

    	  rec = pthread_create(&rThread, NULL, recevoir, (void *) &sock_contact);
		  if (rec) 
		  {
			  printf("Erreur");
			  exit(1);
		  }


		  if(!strncmp(buffer,"/quit",5))	// sortie "propre" du serveur (avec message)
		  {
 				exit(0);
		  }
		sem_wait(&sem);
    	memset(buffer,0,BUFFER_SIZE);
      	n = read(STDOUT_FILENO, buffer, BUFFER_SIZE);
      	write(sock_contact, buffer, n);
      	sem_post(&sem);

    }   

     close(sock_contact);
     pthread_exit(NULL);


		return 0;
	}

