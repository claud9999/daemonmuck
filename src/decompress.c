#include "copyright.h"
#include "config.h"

#include <stdio.h>

void main()
{
    char buf[16384];

    while(gets(buf))
    {
        puts(uncompress(buf));
    }
    exit(0);
}
