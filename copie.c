#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>

#include "copie.h"

/* send_msg send a message on socket sock
* sock: the socket
* code: message's protocol code
* size: message's size
* msg: message to be sent
*/
int send_msg(int sock, unsigned char code, unsigned char size, char *body)
{
  msg_t msg;
  msg.code = code;
  msg.size = size;

  /* sending message head */
  if (send(sock, &msg, HEADSIZE, 0) == -1) {
    PERROR("send head");
    return -1;
  }
  if (size > 0) {
    /* sending message body if any */
    if (send(sock, body, size, 0) == -1) {
      PERROR("send body");
      return -1;
    }
  }

  return size+HEADSIZE;
}

/* recv_msg recv a message from the socket sock
* sock: the socket
* code: message's protocol code
* size: message's size
* msg: message to be received
*/
int recv_msg(int sock, unsigned char *code, unsigned char *size, char **body)
{
  msg_t msg;

  /* receiving message head */
  if (recv(sock, &msg, HEADSIZE, 0) == -1) {
    PERROR("send head");
    return -1;
  } else {
    *size = msg.size;
    *code = msg.code;
  }


  if (msg.size > 0) {
    /* receiving message body if any */
    if (recv(sock, *body, (size_t)msg.size, 0) == -1) {
      PERROR("receive body");
      return -1;
    }
  }

  return *size+HEADSIZE;
}
