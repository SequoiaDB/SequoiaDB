#include <fcntl.h>    /* fcntl */
#include <stdio.h>    /* FILE */
#include <errno.h>    /* errno */
#include <string.h>   /* strerror */
#include <unistd.h>   /* STDIN_FILENO */

int
ut_lock(FILE *fp)
{
  struct flock lock;
  int    rc, fd;
  
  /* Do not attempt to lock stderr, stdout, stdin */
  fd = fileno(fp);
  if(fd == STDOUT_FILENO || fd == STDIN_FILENO || fd == STDERR_FILENO) {
    return -1;
  }

  lock.l_type   = F_WRLCK;
  lock.l_start 	= 0;
  lock.l_whence = SEEK_SET;
  lock.l_len 	= 0;

  rc = fcntl(fd, F_SETLKW, &lock);  /* get the lock or block process */

  return rc;
}

int
ut_unlock(FILE *fp)
{
  struct flock lock;
  int    rc, fd;
  
  /* Flush FILE buffer before releasing the lock. */
  fflush(fp);
  
  /* Do not attempt to unlock stderr, stdout, stdin */
  fd = fileno(fp);
  if(fd == STDOUT_FILENO || fd == STDIN_FILENO || fd == STDERR_FILENO) {
    return -1;
  }

  lock.l_type 	= F_UNLCK;
  lock.l_start 	= 0;
  lock.l_whence = SEEK_SET;
  lock.l_len 	= 0;

  rc = fcntl(fd, F_SETLK, &lock);

  return rc;
}

int
main(int argc, char *argv[])
{
  int rc;
  FILE *fp;
  const char *filename = "foo";

  printf("...attempting to open file `%s'\n", filename);
  fp = fopen(filename, "w");
  if(fp == NULL) {
    fprintf(stderr, "cannot open file `%s'\n", filename);
    return 1;
  }
  printf("...OK. file `%s' successfully opened\n", filename);

  printf("...attempting to obtain a lock on file `%s'\n", filename);
  rc = ut_lock(fp);
  if(rc == 0) {
    printf("...OK. lock obtained for file `%s'\n", filename);
  } else {
    printf("...OOPS. could not obtain lock for file `%s'\n", filename);
    printf("...reason is: %s\n", strerror(errno));
    return 1;
  }

  printf("...attempting to unlock file `%s'\n", filename);
  rc = ut_unlock(fp);
  if(rc == 0) {
    printf("...OK. file `%s' unlocked\n", filename);
  } else {
    printf("...OOPS. could unlock file `%s'\n", filename);
    printf("...reason is: %s\n", strerror(errno));
    return 1;
  }

  printf("...hmm, doens't look like a locking problem\n");

  return 0;
}
