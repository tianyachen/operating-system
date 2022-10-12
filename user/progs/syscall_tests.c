
#include <stdio.h>
#include <syscall.h>

int main(int argc, char *argv[])
{
  set_term_color( FGND_GREEN | BGND_BLUE );

  print( 10, "Hello!!!!!\n");

  printf( "\n" );
  printf( "System Ticks: %d\n", get_ticks() );
  sleep( 100 );
  printf( "System Ticks: %d\n", get_ticks() );

  printf( "Thread ID: %d\n", gettid() );

  vanish();
}
