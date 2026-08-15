#include "runtime_server_init.h"
#include "pc_comm_init.h"
#include "satellite_server_init.h"


int runtime_server_init()
{
    if (pc_comm_init())
    {
        return 1;
    }

     if (satellite_server_init())
    {
        return 1;
    }
    
  return  0;
}