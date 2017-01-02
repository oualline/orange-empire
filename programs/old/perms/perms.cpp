#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

int main()
{
    setuid( 0 );
    system("chmod a+rw /dev/serial/by-id/*");
    system("chmod a+rw /dev/input/by-id/*");

    return 0;
}
