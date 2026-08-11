#include "runtime_server_init.h"
#include "pc_comm_init.h"



int runtime_server_init()
{
    if (pc_comm_init())
    {
        return 1;
    }
    
  return  0;
}