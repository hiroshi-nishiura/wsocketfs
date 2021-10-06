/*
  Smple WebSocket File System
 */
#ifndef _WSOCKET_H_
#define _WSOCKET_H_

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

#ifdef ESP_PLATFORM

#include "lwip/sockets.h"

#else // LINUX

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#endif // PLATFORM

typedef enum
{
  WS_OP_Continuation = 0,
  WS_OP_Text,
  WS_OP_Binary,
  WS_OP_Close = 8,
  WS_OP_Ping,
  WS_OP_Pong,
  WS_OP_MASK = 0xf
} ws_opcodes;

// api
int ws_create(); // return listen
void ws_release(int listen);
int ws_open(int listen); // return : sock
void ws_close(int sock);
int ws_write(int sock, int opcode, const void *data, size_t data_len);
int ws_read(int sock, void *data, size_t data_len); // return : opcode

#define WS_OK 0
#define WS_NG -1

#endif //_WSOCKET_H_
