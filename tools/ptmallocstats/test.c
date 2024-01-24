#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

//gcc test.c -g -lpthread -o test -L /opt/glibc-debug/glibc-2.23/lib -I /opt/glibc-debug/glibc-2.23/include -Wl,--rpath="/opt/glibc-debug/glibc-2.23/lib"
//gcc test.c -g -lpthread -o test

int main () {
  int alloc_time = 4000;
  char *a[alloc_time];
  char *b[alloc_time];
  int i, j;

  for(i=0; i<alloc_time; i++)
  {
      a[i] = (char *)malloc(i);
      memset(a[i], 1, i);
  }
  printf("malloc finished\n");
  for(i=alloc_time-1; i>=0; i--)
  {
      free(a[i]);
  }
  printf("free finished\n");

  malloc_stats();
  malloc_info(0, stdout);
  while(1) {
    sleep(5);
  }
  malloc_trim (0);
}
