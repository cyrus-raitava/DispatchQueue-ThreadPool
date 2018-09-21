//
// Created by Cyrus Raitava-Kumar on 5/09/18.
//

#include <stdio.h>
#include <sys/sysinfo.h>
#include <unistd.h>

// Method to get the number of processors on the machine
int getNumberOfProcessors()
{
    return sysconf(_SC_NPROCESSORS_ONLN);
}

int main(int argc, char *argv[])
{
    printf("This machine has %d cores.\n", getNumberOfProcessors());
    return 0;
}


