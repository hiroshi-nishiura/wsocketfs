#include <stdio.h>
#include <stdlib.h>

#include "wsfs.h"

char *image_names[] =
    {
        "images/img01.jpg",
        "images/img02.jpg",
        "images/img03.jpg",
        "images/img04.jpg"};

int main()
{
  int num_images = sizeof(image_names) / sizeof(char *);

  w_init("WebSocket Simple File System Test");

  int rc = 0;
  void *buf = 0;
  do
    for (int i = 0; i < num_images; i++)
    {
      w_file file;
      rc = w_open(&file, image_names[i], W_READ);
      if (rc < 0)
        break;

      size_t nsize = w_size(&file);
      if (!nsize)
        break;

      buf = malloc(nsize);

      rc = w_read(&file, buf, nsize, &nsize);
      if (rc < 0)
        break;

      rc = w_close(&file);
      if (rc < 0)
        break;

      rc = w_open(&file, "result", W_WRITE);
      if (rc < 0)
        break;

      rc = w_write(&file, buf, nsize, &nsize);
      if (rc < 0)
        break;

      rc = w_close(&file);
      if (rc < 0)
        break;

      w_printf("%d: nsize:%d\n", i, nsize);

      free(buf);
      buf = 0;
    }
  while (rc == WS_OK);

  if (buf)
    free(buf);

  w_release();

  return 0;
}
