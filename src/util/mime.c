#include <string.h>
#include <stdio.h>

#define DEFAULT_MIME_TYPE "application/octet-stream"

char *get_mime_type(char *path) {
  char *ext = strchr(path + 1, '.');

  if (!ext)
    return DEFAULT_MIME_TYPE;

  ++ext;
  if (strcmp(ext, "html") == 0)
    return "text/html";

  return DEFAULT_MIME_TYPE;  
}

