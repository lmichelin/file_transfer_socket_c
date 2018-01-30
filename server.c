#include <unistd.h>
#include <stdlib.h>

#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <errno.h>
#include <fcntl.h>

#include "copie.h"

#define BACK_LOG 10            // nombre maximal de demandes de connexions en attente

#define handle_error(msg) \
do { perror(msg); exit(EXIT_FAILURE); } while (0)

int DFLAG;

/* Crée une socket d'écoute
* srv_port: numéro de port d'écoute sous forme de chaîne de caractères
* maxconn: nombre maximum de demandes de connexions en attente
*
* retourne la socket créée
*/
int create_a_listening_socket(char *srv_port, int maxconn){
  struct addrinfo hints;
  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_flags = AI_PASSIVE|AI_ALL|AI_V4MAPPED;
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  struct addrinfo *result, *rp;
  int ret_code;
  int srv_sock;

  /* Récupération des paramètres de création de la socket */
  ret_code = getaddrinfo(NULL, srv_port, &hints, &result);
  if (ret_code != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ret_code));
    return -1;
  }

  for (rp = result; rp != NULL; rp = rp->ai_next) {
    /* Tentative de création de la socket */
    srv_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (srv_sock == -1) {
      fprintf(stderr, "socket\n");
      continue;
    }

    /* Tentative d'appel à bind */
    if(bind (srv_sock, rp->ai_addr, rp->ai_addrlen) == -1) {
      fprintf(stderr, "error : bind\n");
      close(srv_sock);
      continue;
    } else {
      printf("bind successful !\n");
      break;
    }
  }

  if (rp == NULL ) {               /* Aucune socket créée */
    fprintf(stderr, "Could not bind\n");
    return -1;
  }

  freeaddrinfo(result);

  /* Configuration de la socket en écoute passive */
  if (listen(srv_sock, maxconn) == -1) {
    fprintf(stderr, "listen\n");
    return -1;
  }
  return srv_sock;
}

/* Attend l'arrivée de demandes de connexions
* srv_sock: la socket d'écoute
* clt_sockaddr: pointeur sur une structure sockaddr devant recevoir
*               les informations de connexion du client
*
* retourne: le numéro de la socket créée pour le nouveau client
*/
int accept_clt_conn(int srv_sock, struct sockaddr_storage *clt_sockaddr) {
  int clt_sock;
  socklen_t addrlen = sizeof(struct sockaddr_storage);

  /* mise en attente de connexion sur la socket */
  clt_sock = accept(srv_sock, (struct sockaddr*)clt_sockaddr, &addrlen);

  if(clt_sock == -1) {
    perror("error : accept");
  } else {
    printf("Connection accepted !\n");
  }

  return clt_sock;
}

/* Transfert le contenu du fichier par block de BUFF_SIZE octets
* int fd: descripteur du fichier ouvert en lecture
* int sd: descripteur de la socket
*
* retourne: nombre d'octets effectivement transmis
*/
ssize_t transfert_fichier(int sd, int fd) {
  char buff[BUFF_SIZE];

  ssize_t size;                  // nombre d'octets échangés
  ssize_t nb_sent = 0;           // nombre total d'octets envoyés

  /* lecture du fichier par clock de BUFF_SIZE octet
  et envoi du contenu au client */

  while ((size = read(fd, buff, BUFF_SIZE)) && size > 0) {
    send_msg(sd, DATA, size, buff);
    
    nb_sent += size;
    printf("Send %ld bytes\n", nb_sent);
  }
  close(fd);

  /* Envoi du message de fin de fichier */
  send_msg(sd, END_OK, 0, NULL);

  return nb_sent;
}

/*
* Analyse la demande d'un client
*/
int requete_client(int sock)
{
  char *buff;
  int file_fd;
  struct stat file_stat;
  unsigned char code;
  unsigned char size;
  char file_size[BUFF_SIZE];

  // Réception de la requête contenant le nom du fichier à télécharger
  char file_name[256];
  buff = file_name;
  int size_received = recv_msg(sock, &code, &size, &buff);
  if (size_received == -1) {
    PERROR("receive file_name");
  }

  // test si le fichier existe
  if (access(file_name, F_OK) == -1) {
    int size_sent = send_msg(sock, DO_NOT_EXIST, 0, NULL);
    printf("file does not exist !\n");
    if (size_sent == -1) {
      PERROR("send file not exists message");
    }
    return -1;
  }

  // test si le fichier est accessible en lecture
  if (access(file_name, R_OK) == -1) {
    int size_sent = send_msg(sock, ACCESS_DENIED, 0, NULL);
    printf("bad permissions !\n");
    if (size_sent == -1) {
      PERROR("send file not readable message");
    }
    return -1;
  }

  // Ouverture en lecture du fichier demandé
  file_fd = open(file_name, O_RDONLY);

  if (file_fd == -1) {
    // Gestion des cas d'erreur lors de l'ouverture du fichier
    return -1;
  }

  // récupération des informations relatives au fichier
  fstat(file_fd, &file_stat);

  snprintf(file_size, BUFF_SIZE, "%llu", (unsigned long long)file_stat.st_size);

  // envoi de la réponse au client (code + taille du fichier)
  int size_sent = send_msg(sock, ACCESS_OK, strlen(file_size)+1, file_size);
  if (size_sent == -1) {
    PERROR("send file informations");
  }

  return file_fd;

}

int main(void)
{
  int socket_fd;             // socket correspondant au port sur lequel le serveur attend
  int con_fd;                // socket créée lors de l'établissement d'une connexion avec un client

  struct sockaddr_storage sockaddr_client;

  int file_fd;               // descripteur de fichier pour le fichier à transmettre

  // création de la socket
  socket_fd = create_a_listening_socket(SRV_PORT,BACK_LOG);

  for (;;) // boucle infinie: en attende de demande de connexion
  {
  // acceptation d'une demande de connexion
  printf("Waiting for connection...\n");
  con_fd = accept_clt_conn(socket_fd, &sockaddr_client);
  if (con_fd == -1) {
    PERROR("connection not accepted");
    continue;
  }
  // lecture de la requête client
  file_fd = requete_client(con_fd);
  if(file_fd == -1) {
    PERROR("error while opening file");
    continue;
  }

  // transfert du fichier
  int nbre_send = transfert_fichier(con_fd, file_fd);
  printf("Octets envoyés : %d\n", nbre_send);

  // fermeture du fichier
  close(file_fd);

  } // fin for

  return EXIT_SUCCESS;
}
