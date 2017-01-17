#include <limits.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <papi.h>
#include <unistd.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>

int
main( int argc, char *argv[] )
{
  int retval, num_tests = 1, tmp, i,j=1;
  int EventSet1 = PAPI_NULL;
  long long *values;
  char event_name[PAPI_MAX_STR_LEN];
  const PAPI_hw_info_t *hw_info;
  const PAPI_component_info_t *cmpinfo;
  pid_t pid;

  retval = PAPI_library_init( PAPI_VER_CURRENT );
  cmpinfo = PAPI_get_component_info(0);
  hw_info = PAPI_get_hardware_info();
  retval = PAPI_create_eventset(&EventSet1);
  retval = PAPI_assign_eventset_component( EventSet1, 0 );

  PAPI_option_t opt;
  memset( &opt, 0x0, sizeof ( PAPI_option_t ) );
  opt.inherit.inherit = PAPI_INHERIT_ALL;
  opt.inherit.eventset = EventSet1;
  if ( ( retval = PAPI_set_opt( PAPI_INHERIT, &opt ) ) != PAPI_OK ) {
    perror("no!\n");
  }
  
  retval = PAPI_add_event(EventSet1, PAPI_TOT_INS);
  retval = PAPI_add_event(EventSet1, PAPI_TOT_CYC);
  retval = PAPI_add_event(EventSet1, PAPI_L3_TCA);
  if (PAPI_add_named_event(EventSet1, "MEM_LOAD_UOPS_LLC_MISS_RETIRED:REMOTE_DRAM") != PAPI_OK ) {
  }


  /* /\*Find the event code for PAPI_TOT_INS and its info*\/ */
  /* PAPI_event_info_t info; */
  /* if (PAPI_get_event_info(0x10D3, &info) == PAPI_OK) { */
  /*   perror("eat shit"); */
  /* } */

  /* printf("Info: %d\n", info.event_code); */
      
  /* if (PAPI_add_event(EventSet1, 0x10D3) != PAPI_OK) */
  /*   perror("sdfsd\n"); */

  pid = fork();
  if (pid == 0) {

    char* program[] = {"/localhome/kantonia/slate/benchmarks/metis/obj/app/wc", "/localhome/kantonia/slate/benchmarks/metis/data/wc/300MB_1M_Keys.txt", "-p", "12", NULL};
    execv(program[0], program);

  }
  
  values = malloc(sizeof(long) * 10);
  retval = PAPI_start( EventSet1 );

  retval = PAPI_attach( EventSet1, ( unsigned long ) pid );
  int status;


  waitpid(pid, &status, 0);
  
  

  

  retval = PAPI_stop( EventSet1, values );
  retval = PAPI_cleanup_eventset(EventSet1);
  retval = PAPI_destroy_eventset(&EventSet1);
  printf( "Test case: 3rd party attach start, stop.\n" );

  printf
    ( "-------------------------------------------------------------------------\n" );

  printf( "Test type    : \t           1\n" );
  printf( "PAPI_LD_INS :%lld \t", ( values[0] ) );
  printf( "PAPI_SR_CYC : %lld \t", ( values[1] ) );
  printf( "PAPI_L3_TCA : %lld \t", ( values[2] ) );
  printf( "remote_dram : %lld \t", ( values[3] ) );

  printf
    ( "-------------------------------------------------------------------------\n" );


}
