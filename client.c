#include <unistd.h>
#include <stdlib.h>

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>

#include <netinet/in.h>
#include <netdb.h>

#include <fcntl.h>

#include <string.h> // pour memcpy

#include "copie.h"

int DFLAG;

char *srv_name = "localhost";

/* Établit une session TCP vers srv_name sur le port srv_port
 * char *srv_name: nom du serveur (peut-être une adresse IP)
 * char *srv_port: port sur lequel la connexion doit être effectuée
 *
 * retourne: descripteur vers la socket
 */
int connect_to_server(char *srv_name, char *srv_port){
  struct addrinfo hints;
  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  struct addrinfo *result, *rp;
  int ret_code;
  int clt_sock;

  /* Récupération des paramètres de création de la socket */
  ret_code = getaddrinfo(srv_name, srv_port, &hints, &result);
  if (ret_code != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ret_code));
    return -1;
  }

  for (rp = result; rp != NULL; rp = rp->ai_next) {
    /* Tentative de création de la socket */
    clt_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (clt_sock == -1) {
      fprintf(stderr, "socket\n");
      continue;
    }

    /* Tentative de connexion au serveur */
    if(connect (clt_sock, rp->ai_addr, rp->ai_addrlen) == -1) {
      /* Si erreur, essayer l'entrée suivante */
      // fprintf(stderr, "error : connect\n");
      close(clt_sock);
      continue;
    } else {
      printf("connect successful !\n");
      break;
    }
  }

  if (rp == NULL ) {               /* Aucune socket créée */
    fprintf(stderr, "Could not connect\n");
    return -1;
  }

  freeaddrinfo(result);

  return clt_sock;
}

/* Lit le contenu du fichier par block de taille BUFF_SIZE
 * int fd: descripteur du fichier ouvert en écriture
 * int sd: descripteur de la socket
 * int fz: taille du fichier à recevoir en octets
 *
 * retourne: nombre d'octets effectivement reçus ou -1 en cas d'erreur
 */
int reception_fichier(int fd, int sd, int fz)
{
  int nb_recv = 0;                    // nombre total d'octet reçus

  unsigned char code, size;

  char buffer[BUFF_SIZE];
  char* data = buffer;

  // si fichier vide, retourner 0
  if (fz == 0) return 0;

  for (;;) {
    size = recv_msg(sd, &code, &size, &data);
    if (code == END_OK) {
      break;
    }
    write(fd, data, size-HEADSIZE);
    nb_recv += size-HEADSIZE;
    printf("Received %ld bytes and we hope :- %d bytes\n", size-HEADSIZE, fz-nb_recv);
  }
  close(fd);

  return nb_recv;
}

int main(int argc, char *argv[])
{
  int clt_sock;                       // descripteur de la socket
  char *srv_port = SRV_PORT;

  int file_fd;                        // descripteur du fichier créé
  ssize_t size_sent;                  // nombre d'octets échangés

  int file_size;                      // taille du fichier à télécharger
  int nb_recv = 0;                    // nombre total d'octet reçus

  char file_name[BUFF_SIZE];


  unsigned char code, size;           // code et taille des données reçues
  char *data;                         // données reçues


  DFLAG = 1;

  if (argc < 2)
    {
      printf("%s prends comme argument le nom d'un fichier\n", argv[0]);
      exit(-1);
    }

  strncpy(file_name, argv[1], BUFF_SIZE);

  // Création de la socket et connexion au serveur
  clt_sock = connect_to_server(srv_name, srv_port);

  // Envoi de la requête contenant le nom du fichier à télécharger

  size_sent = send_msg(clt_sock, GET_FILE, strlen(file_name)+1, file_name); // +1 for \0
  if (size_sent == -1) {
    PERROR("send file_name");
  }

  // Réception de la réponse du serveur
  char buffer[BUFF_SIZE];
  data = buffer;
  int size_received = recv_msg(clt_sock, &code, &size, &data);
  if (size_received == -1) {
    PERROR("receive file informations");
  }

  // Test du code d'erreur retourné par le serveur
  if (code == DO_NOT_EXIST) {
    printf("Le fichier n'existe pas\n");
    return -1;
  } else if (code == ACCESS_DENIED) {
    printf("Fichier inaccessible : mauvaises permissions\n");
    return -1;
  }

  file_size = atoi(buffer);
  printf("Le fichier %s fait %d octets\n", file_name, file_size);

  // Création du fichier destination
  file_fd = open(file_name, O_CREAT|O_WRONLY|O_TRUNC, 00644);

  if ( file_fd == -1 ) {
    perror("client: erreur lors de la création du fichier");
    send_msg(clt_sock, END_ERROR, 0, NULL);
    exit(-1);
  }

  // Récupération du contenu du fichier
  nb_recv = reception_fichier(file_fd, clt_sock, file_size);
  printf("%d octets reçus\n", nb_recv);

  close(clt_sock);

  return EXIT_SUCCESS;
}
