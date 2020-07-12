#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define BEGIN_MOVE  "***Property list start *** - "
#define END_MOVE  "***Property list end ***"

#define SIZE 16384

typedef struct zone_S
{
	long id, start, end;
	struct zone_S *next;
} zone_T;

zone_T *head = NULL;

void add_zone(long id, long start, long end)
{
   zone_T *tmp;

	for (tmp = head; tmp; tmp = tmp->next)
	{
		if (!((tmp->start > end) || (tmp->end < start)))
		{
			fprintf(stderr, "Overlap error:\n");
         fprintf(stderr, " %6ld -> %6ld - %6ld\n", id, start,end);
         fprintf(stderr, " %6ld -> %6ld - %6ld\n",
					  tmp->id, tmp->start, tmp->end);
		}
	}

	tmp = (zone_T *) malloc (sizeof(zone_T));
	tmp->id = id;
	tmp->start = start;
	tmp->end = end;
	tmp->next = head;
	head = tmp;
}

void add_zone2(long id, long start, long end)
{
   zone_T *tmp;

	for (tmp = head; tmp; tmp = tmp->next)
	{
		if (tmp->id == id)
		{
			tmp->start = start;
			tmp->end = end;
		   return;
		}
	}

	tmp = (zone_T *) malloc (sizeof(zone_T));
	tmp->id = id;
	tmp->start = start;
	tmp->end = end;
	tmp->next = head;
	head = tmp;
}

void check_zone2(void)
{
   zone_T *tmp, *ntmp;

	for (tmp = head; tmp; tmp = ntmp)
	{
		ntmp = tmp->next;

		if (tmp->end != -1L && tmp->start != tmp->end)
		{
			fprintf(stderr, "Map error:\n");
			fprintf(stderr, " %6ld -> %6ld : %6ld\n",
					  tmp->id, tmp->start, tmp->end);
		}
		free(tmp);
	}

   head = NULL;
}

void main(int argc, char **argv)
{
   FILE *imap = NULL, *omap = NULL, *idat = NULL, *odat = NULL;
   char buffer[BUFFER_LEN];
   long tmp[2];
	long zid, zstart, zend;
	char *p;

#define CHECKFILE(x) if ((x) == NULL) \
                     { fprintf(stderr, "Trouble opening %s\n", buffer); \
                       exit(1); }

   snprintf(buffer, BUFFER_LEN, "%s.map", argv[1]);
   imap = fopen(buffer, "r+");
   CHECKFILE(imap);

   snprintf(buffer, BUFFER_LEN, "%s.map", argv[2]);
   omap = fopen(buffer, "w+");
   CHECKFILE(omap);

   snprintf(buffer, BUFFER_LEN, "%s.dat", argv[1]);
   idat = fopen(buffer, "r+");
   CHECKFILE(idat);

   snprintf(buffer, BUFFER_LEN, "%s.dat", argv[2]);
   odat = fopen(buffer, "w+");
   CHECKFILE(odat);
   fprintf(odat, "--- Doran dbp file ---\n");

	p = buffer;
	p += strlen(BEGIN_MOVE);

   /* map sanity checker */
	zid = -1L;

   while (fread(tmp, sizeof(long), 2, imap))
	{
		if (tmp[0] >= 0 && tmp[0] != zid)
		{
			fprintf(stderr, "Map index error:\n");
			fprintf(stderr, " %6ld -> %6ld\n", zid, tmp[0]);
		}
		zid ++;
	}

   /* first line sanity checker */
   fseek(idat, 0L, SEEK_SET);

	zstart = 0;

	while (fgets(buffer, SIZE, idat))
	{
		if (!strncmp(buffer, BEGIN_MOVE, strlen(BEGIN_MOVE)))
		{
			zid = atol (p);
			fseek(imap, (zid + 1) * sizeof(long) * 2, SEEK_SET);
         fread(tmp, sizeof(long), 2, imap);

			add_zone2(zid, zstart, tmp[1]);
		}
		zstart = ftell(idat);
	}

	check_zone2();

	fseek(imap, 0L, SEEK_SET);
	fseek(idat, 0L, SEEK_SET);

   while (fread(tmp, sizeof(long), 2, imap) == 2)
   {
      if (tmp[0] != -1L)
      {
         fseek(idat, tmp[1], SEEK_SET);

			zid = tmp[0];
			zstart = tmp[1];

         tmp[1] = ftell(odat);

         fwrite(tmp, sizeof(long), 2, omap);
			fflush(omap);

         /* if (tmp[0] == (ftell(imap) / sizeof(long) / 2 - 2)) */

         fgets(buffer, SIZE, idat);
         fprintf(odat, "%s", buffer);

			if (strncmp(buffer, BEGIN_MOVE, strlen(BEGIN_MOVE)) ||
			    (zid != atol(p)))
			{
				fprintf(stderr, "First line error:\n");
				fprintf(stderr, " %6ld -> %s", zid, buffer);
			}

         do
         {
            fgets(buffer, SIZE, idat);
            fprintf(odat, "%s", buffer);
         } while (strncmp(buffer, END_MOVE, strlen(END_MOVE)));

			zend = ftell(idat) - 1;

			add_zone(zid, zstart, zend);
      }
      else
      {
			tmp[1] = -1L;
         fwrite(tmp, sizeof(long), 2, omap);
			fflush(omap);
      }
   }

   fclose(imap);
   fclose(omap);
   fclose(idat);
   fclose(odat);
}
