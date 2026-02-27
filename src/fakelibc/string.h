#include <stddef.h>

size_t strlen(const char *s);
int strcmp(const char *l, const char *r);
int strncmp(const char *_l, const char *_r, size_t n);

void *memcpy(void *dest, const void * src, unsigned long n);

int memcmp(const void *str1, const void *str2, unsigned long n);
void * memset ( void * ptr, int value, unsigned long num );

void free(void *ptr);
void* malloc (unsigned long size);
void *calloc(size_t nitems, size_t size);
void* realloc (void* ptr, unsigned long size);
void *memchr(const void *src, int c, size_t n);
char * strchr ( const char * str, int character );
char * strrchr ( const char * str, int character );
int rand(void);
int remove(const char *filename);
char *strcpy(char *restrict dest, const char *restrict src);
char *tmpnam(char *str);

#define FILENAME_MAX 4096