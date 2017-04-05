#include <iostream>
#include <mctop.h>



int main(int argc, char* argv[])
{
  mctop_t* topo = mctop_load(NULL);
  //  mctop_print(topo);
  if (!topo)   {
    fprintf(stderr, "Couldn't load topology file.\n");
    return EXIT_FAILURE;
  }

  for (int i = 0; i < 96; ++i) {
    printf("%d: %u\n", i,  mctop_hwcid_get_local_node(topo, i));
  }

  return 0;
}

