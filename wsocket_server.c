/*
  Smple WebSocket Server
 */
#include "wsocket.h"
#include "base64.h" // https://github.com/joedf/base64.c.git
#include "sha1.h"   // https://www.ipa.go.jp/security/rfc/RFC3174JA.html

#define WS_PORT 3000

static void ws_encode_key(char *key)
{
  printf("key:%s\n", key);
  strcat(key, "258EAFA5-E914-47DA-95CA-C5AB0DC85B11");
  SHA1Context sha;
  uint8_t digest[SHA1HashSize];
  SHA1Reset(&sha);
  SHA1Input(&sha, (const uint8_t *)key, strlen(key));
  SHA1Result(&sha, digest);
  b64_encode(digest, sizeof(digest), (uint8_t *)key);
}

static int ws_find_key(char *buf, char *keyword)
{
  int lnum = 0;
  char *line;
  char *crnl;
  char *colon;
  char *arg;
  char key[32] = {0};

  for (line = (char *)buf; (crnl = strstr(line, "\r\n")) != NULL; line = crnl + 2, lnum++)
  {
    if (lnum == 0)
      continue; // "GET" line
    if (crnl == line)
      break; // empty line
    if (((colon = strchr(line, ':')) == NULL) || (colon > crnl))
    {
      printf("no colon in header line?\n");
      break;
    }
    if (strncasecmp(line, keyword, colon - line) == 0)
    {
      for (arg = colon + 1; isspace(*arg); ++arg)
      {
      }
      strncpy(key, arg, crnl - arg);
      key[crnl - arg] = '\0';
      break;
    }
  }
  if (*key)
  {
    return strlen(strcpy(buf, key));
  }
  printf("keyword %s handshake failed\n", keyword);
  return 0;
}

int ws_create()
{
  struct sockaddr_in dest_addr;
  memset(&dest_addr, 0, sizeof(dest_addr));
  dest_addr.sin_family = AF_INET;
  dest_addr.sin_port = htons(WS_PORT);

  int ws_listen = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);

  int yes = 1;
  if (setsockopt(ws_listen, SOL_SOCKET, SO_REUSEADDR, (const void *)&yes, sizeof(yes)))
  {
    perror("Socket unable to setsockopt");
    ws_release(ws_listen);
    return -1;
  }

  if (bind(ws_listen, (struct sockaddr *)&dest_addr, sizeof(dest_addr)))
  {
    perror("Socket unable to bind");
    ws_release(ws_listen);
    return -1;
  }

  if (listen(ws_listen, 10))
  {
    perror("Error occurred during listen");
    ws_release(ws_listen);
    return -1;
  }
  printf("listening port:%d\n", WS_PORT);

  return ws_listen;
}

void ws_release(int listen)
{
  printf("ws_release %d\n", listen);
  close(listen);
}

int ws_open(int listen)
{
  struct sockaddr_in addr_in;
  int addr_len = sizeof(addr_in);
  int sock = accept(listen, (struct sockaddr *)&addr_in, &addr_len);

  char s_addr[INET_ADDRSTRLEN];
  printf("accept:%d %s\n", sock,
         inet_ntop(AF_INET, &addr_in.sin_addr, s_addr, INET_ADDRSTRLEN));

  char buf[256];
  int len = read(sock, buf, sizeof(buf));
  if (len > 0)
  {
    // websocket opening handshake
    buf[len] = 0;

    if (ws_find_key(buf, "Sec-WebSocket-Key"))
    {
      char key[64];
      ws_encode_key(buf);
      strncpy(key, buf, 64);

      len = sprintf(buf, "HTTP/1.1 101 Switching Protocols \r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: %s\r\n\r\n", key);

      len = write(sock, buf, len);
      if (len > 0)
      {
        printf("websocket connected\n");
        return sock;
      }
    }
  }
  close(sock);
  return 0;
}

void ws_close(int sock)
{
  printf("ws_close %d\n", sock);
  close(sock);
}
