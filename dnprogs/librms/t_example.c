/* Example librms program using text functions. This is the same as the
   example.c program but using the _t functions rather than the RAB/FAB ones.

   This program is in the public domain

*/
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include "rms.h"

int main(int argc, char *argv[])
{
    /* Open the file. Keep the RMSHANDLE returned */
    RMSHANDLE h = rms_t_open("marsha::index.dat", O_RDWR, NULL);
    if (h)
    {
        char b[10240];
	int got;

	/* Look for the record with my name in it. We don't need rac=key
	   here because librms gives us it for free */
	got = rms_t_read(h, b, sizeof(b), "key=PATRICK,kop=kge");
	if (got > 0)
	{
	    b[got] = '\0';
	    printf("Got %d bytes: %s\n", got, b);
	}
	else
	{
	    fprintf(stderr, "Read failed: %s\n", rms_lasterror(h));
	}

        /* Update it */
	memcpy(b+8, "PATWHO?", 7);
	if (rms_t_update(h, b, got, NULL) == -1)
	{
	    fprintf(stderr, "Update failed: %s\n", rms_lasterror(h));
	}
	
	rms_t_close(h);
    }
    else
    {
	fprintf(stderr, "connect: %s\n", rms_openerror());
    }

    return 0;
}
