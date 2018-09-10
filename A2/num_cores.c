//
// Created by Cyrus Raitava-Kumar on 5/09/18.
//

#include <stdio.h>
//#include <sys/sysinfo.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
        printf("This machine has %ld cores.\n", sysconf(_SC_NPROCESSORS_ONLN));
    return 0;
}

