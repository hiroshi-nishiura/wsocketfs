/*
  Smple WebSocket File System
 */
#ifndef _WSFS_H_
#define _WSFS_H_

#include "wsocket.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define W_OK 0
#define W_READ 0
#define W_WRITE 1

  // file
  typedef uint32_t w_file;

  int w_init(const char *title);
  int w_release();
  int w_open(w_file *file, const char *path, unsigned mode);
  int w_close(w_file *file);
  int w_read(w_file *file, void *buf, size_t size, size_t *nread);
  int w_write(w_file *file, const void *buf, size_t size, size_t *nwritten);
  int w_seek(w_file *file, uint32_t pos);
  size_t w_size(w_file *file);
  // special api
  int w_printf(const char *fmt, ...); // print to web console

#ifdef __cplusplus
} // extern "C"
#endif

#endif //_WSFS_H_
