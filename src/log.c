#include "config.h"
#include "params.h"

#include <stdio.h>
#include <sys/types.h>

#include <time.h>
#include "externs.h"

static void _dmuck_log(char *fname, char *format, va_list args) {
	FILE *fp = NULL;
	time_t lt;

	lt = time(NULL);

	if ((fp = fopen(fname, "a")) == NULL) {
		fprintf(stderr, "Unable to open %s!\n", fname);
		fprintf(stderr, "%.16s: ", ctime(&lt));
		vfprintf(stderr, format, args);
	} else {
		fprintf(fp, "%.16s ", ctime(&lt));
		vfprintf(fp, format, args);
		fclose(fp);
	}
}

void log_status(char *format, ...) {
	va_list args;
	va_start(args, format);
	_dmuck_log(LOG_STATUS, format, args);
}

void log_muf(char *format, ...) {
	va_list args;
	va_start(args, format);
	_dmuck_log(LOG_MUF, format, args);
}

void log_gripe(char *format, ...) {
	va_list args;
	va_start(args, format);
	_dmuck_log(LOG_GRIPE, format, args);
}

void log_command(char *format, ...) {
	va_list args;
	va_start(args, format);
	_dmuck_log(COMMAND_LOG, format, args);
}
