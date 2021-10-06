/*
  Smple WebSocket File System
 */
#include "wsfs.h"

const char *_w_title = 0;
int _w_listen = 0;
int _w_sock = 0;
int _w_closed = 0;

enum
{
  WSFS_OPEN_R = 1,
  WSFS_OPEN_W,
  WSFS_CLOSE,
  WSFS_READ,
  WSFS_WRITE,
  WSFS_SEEK,
  WSFS_SIZE,
};

static int _wsfs_wait()
{
  if (_w_listen)
  {
    int rc = 0;
    if (_w_sock == 0)
    {
      _w_sock = ws_open(_w_listen);
      if (_w_sock && _w_title)
      {
        rc = ws_write(_w_sock, WS_OP_Text, "INIT", 4);
        rc = ws_write(_w_sock, WS_OP_Text, _w_title, strlen(_w_title));
      }
    }
    if (_w_sock)
      return rc;
  }
  return -1;
}

int w_init(const char *title)
{
  if (_w_listen)
    return 0;

  _w_title = title;
  _w_listen = ws_create();

  if (_w_listen)
    return 0;
  return -1;
}

int w_release()
{
  if (_w_sock)
  {
    ws_close(_w_sock);
  }
  if (_w_listen)
  {
    ws_release(_w_listen);
  }
  _w_sock = 0;
  _w_listen = 0;
  return 0;
}

int w_open(w_file *file, const char *path, unsigned mode)
{
  if (!file)
    return -1;

  if (_w_closed)
  {
    if (mode == W_READ)
    {
      _w_closed = 0;
      ws_close(_w_sock);
      _w_sock = 0;
    }
    else
      return 0;
  }
  if (_wsfs_wait())
    return -1;

  uint32_t cmd = (mode == W_READ ? WSFS_OPEN_R : WSFS_OPEN_W) << 16;
  int rc = ws_write(_w_sock, WS_OP_Binary, &cmd, 4);
  if (rc == W_OK)
    rc = ws_write(_w_sock, WS_OP_Text, path, strlen(path));
  if (rc == W_OK)
  { // get file id
    rc = ws_read(_w_sock, file, sizeof(file));
    if (rc == W_OK)
      _w_closed = 1;
  }
  return rc;
}

int w_close(w_file *file)
{
  if (_w_closed)
    return 0;
  if (!file || _wsfs_wait())
    return -1;

  uint32_t cmd = (WSFS_CLOSE << 16) | *file;
  int rc = ws_write(_w_sock, WS_OP_Binary, &cmd, 4);
  *file = 0;
  return rc;
}

int w_read(w_file *file, void *buf, size_t size, size_t *nread)
{
  if (_w_closed)
    return 0;
  if (!file || _wsfs_wait())
    return -1;

  uint32_t cmd = (WSFS_READ << 16) | *file;
  int rc = ws_write(_w_sock, WS_OP_Binary, &cmd, 4);
  if (rc == W_OK)
  {
    // request size
    uint32_t n32 = size;
    rc = ws_write(_w_sock, WS_OP_Binary, &n32, 4);
  }
  rc = ws_read(_w_sock, buf, size);
  if (rc == W_OK)
    _w_closed = 1;
  else if (rc > W_OK && nread)
    *nread = size;
  return rc;
}

int w_write(w_file *file, const void *buf, size_t size, size_t *nwritten)
{
  if (_w_closed)
    return 0;
  if (!file || _wsfs_wait())
    return -1;

  uint32_t cmd = (WSFS_WRITE << 16) | *file;
  int rc = ws_write(_w_sock, WS_OP_Binary, &cmd, 4);
  if (rc == W_OK)
  {
    rc = ws_write(_w_sock, WS_OP_Binary, buf, size);
    if (rc == W_OK && nwritten)
      *nwritten = size;
  }
  return rc;
}

int w_seek(w_file *file, uint32_t pos)
{
  if (_w_closed)
    return 0;
  if (!file || _wsfs_wait())
    return -1;

  uint32_t cmd = (WSFS_SEEK << 16) | *file;
  int rc = ws_write(_w_sock, WS_OP_Binary, &cmd, 4);
  if (rc == W_OK)
    rc = ws_write(_w_sock, WS_OP_Binary, &pos, sizeof(pos));
  return rc;
}

size_t w_size(w_file *file)
{
  if (!file)
    return 0;

  uint32_t cmd = (WSFS_SIZE << 16) | *file;
  int rc = ws_write(_w_sock, WS_OP_Binary, &cmd, 4);
  if (rc == W_OK)
  {
    uint32_t size;
    rc = ws_read(_w_sock, &size, sizeof(size));
    if (rc > W_OK)
      return size;
  }
  return 0;
}

#include <stdarg.h>
int w_printf(const char *fmt, ...)
{
  char str[256];
  va_list args;

  if (_w_closed || _wsfs_wait())
    return 0;

  va_start(args, fmt);
  vsnprintf(str, 256, fmt, args);
  va_end(args);

  int size = strlen(str);
  int rc = ws_write(_w_sock, WS_OP_Text, "PRINT", 5);
  if (rc == W_OK)
    rc = ws_write(_w_sock, WS_OP_Text, str, size);

  return size;
}
