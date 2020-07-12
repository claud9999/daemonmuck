#include  <stdio.h>
#include <sys/stat.h>
#include "copyright.h"
#include "config.h"
#include "help.h"
#include "db.h"
#include "params.h"
#include "interface.h"
#include "externs.h"

void update_index(char *indxfile, char *textfile) {
	long pos = 0l;
	int i = 0, n = 0, lineno = 0, ntopics = 0;
	help_indx entry;
	FILE *rfp = NULL, *wfp = NULL;
	char line1[LINE_SIZE + 1], *s = NULL, *topic = NULL;

	fprintf(stderr, "Updating index file:%s\n", indxfile);

	if ((rfp = fopen(textfile, "r")) == NULL) {
		fprintf(stderr, "can't open %s for reading\n", textfile);
		return;
	}
	if ((wfp = fopen(indxfile, "w")) == NULL) {
		fprintf(stderr, "can't open %s for writing\n", indxfile);
		return;
	}

	pos = 0L;
	lineno = 0;
	ntopics = 0;
	while (fgets(line1, LINE_SIZE, rfp) != NULL) {
		++lineno;

		n = strlen(line1);
		if (line1[n - 1] != '\n')
			fprintf(stderr, "line %d: line too long\n", lineno);

		if (!strncmp(line1, HELP_TOPIC_MARK, strlen(HELP_TOPIC_MARK))) {
			++ntopics;

			if (ntopics > 1) {
				entry.len = (int) (pos - entry.pos);
				if (fwrite(&entry, sizeof(help_indx), 1, wfp) < 1) {
					fprintf(stderr, "error writing %s\n", indxfile);
					return;
				}
			}

			for (topic = &line1[3]; (*topic == ' ' || *topic == '\t') && *topic
					!= '\0'; topic++)
				;
			for (i = -1, s = topic; *s != '\n' && *s != '\0'; s++) {
				if (i >= TOPIC_NAME_LEN - 1)
					break;
				if (*s != ' ' || entry.topic[i] != ' ')
					entry.topic[++i] = *s;
			}
			entry.topic[++i] = '\0';
			entry.pos = pos + (long) n;
		}
		pos += n;
	}
	entry.len = (int) (pos - entry.pos);
	if (fwrite(&entry, sizeof(help_indx), 1, wfp) < 1) {
		fprintf(stderr, "error writing %s\n", indxfile);
		return;
	}

	fclose(rfp);
	fclose(wfp);
}

void spit_file(dbref player, char *filename) {
	FILE *f = NULL;
	char bufferone[BUFFER_LEN];

	if ((f = fopen(filename, "r")) == NULL) {
		notify(player, player, "Sorry, %s is broken.  Management has been notified.", filename);
		fputs("spit_file:", stderr);
		perror(filename);
	} else {
		while (fgets(bufferone, BUFFER_LEN, f)) {
			bufferone[strlen(bufferone) - 1] = '\0';
			notify(player, player, bufferone);
		}
		fclose(f);
	}
}

void output_info(dbref player, char *arg1, char *deftopic, char *indxfile,
		char *textfile) {
	int help_found = 0;
	help_indx entry;
	FILE *fp = NULL;
	char line1[LINE_SIZE + 1];
	int numlines = 0;
	struct stat stats;
	time_t indxtime, texttime;

	stat(indxfile, &stats);
	indxtime = stats.st_mtime;
	stat(textfile, &stats);
	texttime = stats.st_mtime;

	if (indxtime < texttime)
		update_index(indxfile, textfile);

	if (*arg1 == '\0')
		arg1 = deftopic;

	if ((fp = fopen(indxfile, "r")) == NULL) {
		notify(player, player, "Sorry, but %s is temporarily not available.",
				deftopic);
		log_status("Can't open %s for reading\n", indxfile);
		return;
	}

	while ((help_found = fread(&entry, sizeof(help_indx), 1, fp)) == 1)
		if (string_prefix(entry.topic, arg1))
			break;
	fclose(fp);

	if (help_found <= 0) {
		notify(player, player, "No %s for '%s'.", deftopic, arg1);
		return;
	}

	if ((fp = fopen(textfile, "r")) == NULL) {
		notify(player, player, "Sorry, but %s is temporarily not available.",
				textfile);
		log_status("Can't open %s for reading\n", textfile);
		return;
	}

	if (fseek(fp, entry.pos, 0) < 0L) {
		notify(player, player, "Sorry, but %s is temporarily not available.",
				deftopic);
		log_status("seek error in file %s\n", textfile);
		return;
	}

	for (;;) {
		if (fgets(line1, LINE_SIZE, fp) == NULL)
			break;
		if (!strncmp(line1, HELP_TOPIC_MARK, strlen(HELP_TOPIC_MARK))) {
			if (numlines >= 1)
				break;
			else
				continue;
		}
		line1[strlen(line1) - 1] = '\0';
		notify(player, player, line1);
		numlines++;
	}
	fclose(fp);
}

void do_help(__DO_PROTO) {
	output_info(player, arg1, "help", HELP_INDEX, HELP_FILE);
}

void do_news(__DO_PROTO) {
	output_info(player, arg1, "news", NEWS_INDEX, NEWS_FILE);
}

void do_man(__DO_PROTO) {
	output_info(player, arg1, "man", MAN_INDEX, MAN_FILE);
}
