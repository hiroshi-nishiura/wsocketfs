/*
  Smple WebSocket File System
 */
#include "wsocket.h"

/* websocket frame header
0                   1                   2                   3
0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-------+-+-------------+-------------------------------+
|F|R|R|R| opcode|M| Payload len |    Extended payload length    |
|I|S|S|S|  (4)  |A|     (7)     |             (16/64)           |
|N|V|V|V|       |S|             |   (if payload len==126/127)   |
| |1|2|3|       |K|             |                               |
+-+-+-+-+-------+-+-------------+ - - - - - - - - - - - - - - - +
|     Extended payload length continued, if payload len == 127  |
+ - - - - - - - - - - - - - - - +-------------------------------+
|                               |Masking-key, if MASK set to 1  |
+-------------------------------+-------------------------------+
| Masking-key (continued)       |          Payload Data         |
+-------------------------------- - - - - - - - - - - - - - - - +
:                     Payload Data continued ...                :
+ - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - +
|                     Payload Data continued ...                |
+---------------------------------------------------------------+
*/
typedef struct
{
  uint8_t opcode : 4;
  uint8_t reserved : 3;
  uint8_t final : 1; // 1 is final frame
  uint8_t payload_len : 7;
  uint8_t mask : 1;
} ws_frame_header_t;

int ws_read(int sock, void *data, size_t data_len)
{
  uint8_t buf[16];

  if (sock <= 0 || !data || !data_len)
  {
    printf("websocket read invalid arguments\n");
    return -1;
  }

  // read header
  int len = read(sock, buf, 16);
  if (len < sizeof(ws_frame_header_t))
  {
    printf("Connection closed 0:%d\n", len);
    return -1;
  }

  ws_frame_header_t hdr;
  memcpy(&hdr, buf, sizeof(hdr));

  if (hdr.final == 0)
  {
    printf("Unsupported frame streaming\n");
    return -1;
  }
  if (hdr.opcode != WS_OP_Text &&
      hdr.opcode != WS_OP_Binary &&
      hdr.opcode != WS_OP_Close)
  {
    printf("Unsupported opcode %d\n", hdr.opcode);
    return -1;
  }
  if (hdr.opcode == WS_OP_Close || hdr.payload_len == 0)
  {
    printf("received close\n");
    return 0;
  }

  uint8_t *next = (uint8_t *)(buf + sizeof(hdr));
  size_t payload_len = 0;
  if (hdr.payload_len >= 126)
  {
    int bytes = 2; //16bit
    if (hdr.payload_len == 127)
      bytes = 8; //64bit
    for (int i = 0; i < bytes; i++)
    {
      payload_len <<= 8;
      payload_len |= *next++;
    }
  }
  else
    payload_len = hdr.payload_len;
  //printf("payload_len=%ld\n",payload_len);
  if (payload_len > data_len)
  {
    printf("Payload length is larger than data length\n");
    return -1;
  }

  uint8_t masking_key[4];
  if (hdr.mask)
  {
    memcpy(masking_key, next, 4);
    next += 4;
  }

  len -= (uintptr_t)next - (uintptr_t)buf;

  // read payload
  uint8_t *payload = (uint8_t *)data;
  memcpy(payload, next, len);
  for (size_t offs = len; offs < payload_len; offs += len)
  {
    len = read(sock, payload + offs, payload_len - offs);
    if (len <= 0)
    {
      printf("Connection closed 1:%d\n", len);
      return -1;
    }
  }

  // decode
  if (hdr.mask)
  {
    for (size_t i = 0; i < payload_len; i++)
    {
      payload[i] = payload[i] ^ masking_key[i % 4];
    }
  }

  return hdr.opcode;
}

int ws_write(int sock, int opcode, const void *data, size_t data_len)
{
  int err = 0;
  if (sock <= 0 || !opcode || !data || !data_len)
  {
    printf("write websocket invalid arguments\n");
    return -1;
  }

  uint8_t header[16];
  ws_frame_header_t *hdr = (ws_frame_header_t *)header;
  int hdr_len = sizeof(ws_frame_header_t);

  hdr->final = 1;
  hdr->reserved = 0;
  hdr->opcode = opcode & WS_OP_MASK;
  hdr->mask = 0; // disable on server

  if (data_len >> 16) //64bit
  {
    hdr->payload_len = 127;
    header[hdr_len++] = 0;
    header[hdr_len++] = 0;
    header[hdr_len++] = 0;
    header[hdr_len++] = 0;
    header[hdr_len++] = (uint8_t)((data_len >> 24) & 0xff);
    header[hdr_len++] = (uint8_t)((data_len >> 16) & 0xff);
    header[hdr_len++] = (uint8_t)((data_len >> 8) & 0xff);
    header[hdr_len++] = (uint8_t)((data_len >> 0) & 0xff);
  }
  else if (data_len >= 126) //16bit
  {
    hdr->payload_len = 126;
    header[hdr_len++] = (uint8_t)(data_len >> 8);
    header[hdr_len++] = (uint8_t)(data_len & 0xff);
  }
  else //7bit
  {
    hdr->payload_len = data_len;
  }

  // write header
  int len = write(sock, hdr, hdr_len);
  if (len == hdr_len)
  {
    // write payload
    uint8_t *payload = (uint8_t *)data;

    for (size_t offs = 0; offs < data_len; offs += len)
    {
      len = write(sock, payload + offs, data_len - offs);
      if (len <= 0)
      {
        printf("send websocket payload failure\n");
        err = -1;
        break;
      }
    }
  }
  else
  {
    printf("send websocket header failure\n");
    err = -1;
  }

  return err;
}
