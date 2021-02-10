/*
 * This file provided from https://github.com/LambdaSchool/C-Web-Server
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "file.h"


struct file_ent *file_ent_create(char *filename) {
  struct file_ent *res = calloc(1, sizeof(struct file_ent));
  if (!res) return NULL;

  res->data = file_load(filename);
  res->filename = malloc(strlen(filename) * sizeof(char));
  strcpy(res->filename, filename);

  return res;
}


void file_ent_free(struct file_ent *file) {
  file_free(file->data);
  free(file->filename);
  free(file);
}

/**
 * Loads a file into memory and returns a pointer to the data.
 * 
 * Buffer is not NUL-terminated.
 */
struct file_data *file_load(char *filename)
{
    char *buffer, *p;
    struct stat buf;
    int bytes_read, bytes_remaining, total_bytes = 0;

    // Get the file size
    if (stat(filename, &buf) == -1) {
        return NULL;
    }

    // Make sure it's a regular file
    if (!(buf.st_mode & S_IFREG)) {
        return NULL;
    }

    // Open the file for reading
    FILE *fp = fopen(filename, "rb");

    if (fp == NULL) {
        return NULL;
    }

    // Allocate that many bytes
    bytes_remaining = (int)buf.st_size;
    p = buffer = malloc(bytes_remaining);

    if (buffer == NULL) {
        return NULL;
    }

    // Read in the entire file
    while ((void)(bytes_read = (int)fread(p, 1, bytes_remaining, fp)), bytes_read != 0 && bytes_remaining > 0) {
        if (bytes_read == -1) {
            free(buffer);
            return NULL;
        }

        bytes_remaining -= bytes_read;
        p += bytes_read;
        total_bytes += bytes_read;
    }

    // Allocate the file data struct
    struct file_data *filedata = malloc(sizeof *filedata);

    if (filedata == NULL) {
        free(buffer);
        return NULL;
    }

    filedata->data = buffer;
    filedata->size = total_bytes;

    return filedata;
}

/**
 * Frees memory allocated by file_load().
 */
void file_free(struct file_data *filedata)
{
    free(filedata->data);
    free(filedata);
}
