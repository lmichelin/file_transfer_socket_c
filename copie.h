#ifndef COPIE_H
#define COPIE_H

#include <stdio.h>

/* Protocol codes */
#define GET_FILE      11
#define PUT_FILE      12
#define FILE_NAME     13
#define END_OK        14

#define ACCESS_OK     21
#define DATA          22

#define BUSY          31

#define END_ERROR     41
#define ACCESS_DENIED 43
#define DO_NOT_EXIST  44



#define BUFF_SIZE 128

#define SRV_PORT "4445"

#define eprintf(...) fprintf(stderr, __VA_ARGS__)

#define DEBUG(...) {if (DFLAG!=0) {                 \
      fprintf(stderr, "%s@%s,line %d: ", __func__, __FILE__, __LINE__); \
      fprintf(stderr, __VA_ARGS__);                 \
      fprintf(stderr, "\n");                        \
      fflush(stderr);                           \
    }}

#define PERROR(str) {                           \
    fprintf(stderr, "%s@%s,line %d: ", __func__, __FILE__, __LINE__);   \
    perror((str));                          \
  }

/* 0 means "off", any other integer means "on" */
extern int DFLAG;

/* structure des messages échangés
   code + size forment l'entête du message,
   data, le contenu du message
 */
typedef struct {
  unsigned char code;
  unsigned char size;
} msg_t;

#define HEADSIZE sizeof(msg_t)

int send_msg(int sock, unsigned char code, unsigned char size, char *body) ;
int recv_msg(int sock, unsigned char *code, unsigned char *size, char **body);

#endif // COPIE_H
