#ifndef _FILELS_H_ // This was just _FILE_H_, but that interfered with Cygwin
#define _FILELS_H_

/*
 * This file provided from https://github.com/LambdaSchool/C-Web-Server
*/

struct file_data {
    int size;
    void *data;
};

// used in the hash table.
struct file_ent {
  char *filename;
  struct file_data *data;
};

extern struct file_ent *file_ent_create(char *filename);
extern void file_ent_free(struct file_ent *file);

extern struct file_data *file_load(char *filename);
extern void file_free(struct file_data *filedata);

#endif
