#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  int i;

  // 0 - stdin
  // 1 - stdout

  // first arg is name of program, so skip it
  for(i = 1; i < argc; ++i)
  {
    // write to default stdout, from buffer pointed by argv[i]
    write(1, argv[i], strlen(argv[i]));

    if(i + 1 < argc)
    {
      // if not last put space
      write(1, " ", 1);
    } 
    else 
    {
      // if last arg, then print \n
      write(1, "\n", 1);
    }
  }

  exit(0);
}
