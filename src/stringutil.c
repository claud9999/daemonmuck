#include "copyright.h"
#include "config.h"

/* String utilities */

#include <string.h> /* amazing how long we've been going without this */
#include <ctype.h>
#include "interface.h"
#include "externs.h"

extern char *uppercase, *lowercase;
#define DOWNCASE(x) (lowercase[(int)x])

static char buf[BUFFER_LEN];
static char tmp_buf[BUFFER_LEN];

int string_compare(char *s1, char *s2) {
	if (!s1)
		s1 = "";
	if (!s2)
		s2 = "";
	while (*s1 && *s2 && DOWNCASE(*s1) == DOWNCASE(*s2))
		s1++, s2++;

	return (DOWNCASE(*s1) - DOWNCASE(*s2));
}

int stringn_compare(char *s1, char *s2, int n) {
	if (!n)
		return 0;
	if (!s1)
		s1 = "";
	if (!s2)
		s2 = "";
	while (--n && *s1 && *s2 && DOWNCASE(*s1) == DOWNCASE(*s2))
		s1++, s2++;

	return (DOWNCASE(*s1) - DOWNCASE(*s2));
}

int string_prefix(char *string, char *prefix) {
	if (!string || !prefix)
		return 0;

	while (*string && *prefix && DOWNCASE(*string) == DOWNCASE(*prefix))
		string++, prefix++;
	return *prefix == '\0';
}

/* accepts only nonempty matches starting at the beginning of a word */
char *string_match(char *src, char *sub) {
	/*  if(*sub != '\0') {  */
	if (*sub != '\0' && *src != '\0') {
		while (*src) {
			if (string_prefix(src, sub))
				return src;
			/* else scan to beginning of next word */
			while (*src && isalnum(*src))
				src++;
			while (*src && !isalnum(*src))
				src++;
		}
	}

	return 0;
}

/*
 * pronoun_substitute()
 *
 * %-type substitutions for pronouns
 *
 * %a/%A for absolute possessive (his/hers/its, His/Hers/Its)
 * %s/%S for subjective pronouns (he/she/it, He/She/It)
 * %o/%O for objective pronouns (him/her/it, Him/Her/It)
 * %p/%P for possessive pronouns (his/her/its, His/Her/Its)
 * %r/%R for reflexive pronouns (himself/herself/itself,
 *                                Himself/Herself/Itself)
 * %n    for the player's name.
 */
char *pronoun_substitute(dbref player, char *str) {
	char c;
	char prn[3];
	char *tmp;
	char *result;
	char *self_sub; /* self substitution code */
	int lngth_sofar = 0;

	/*  lngth_sofar is a ROUGH account, so we don't overflow our buf
	 *   if you've got a gripe about it, fix it yourself.  Too many
	 *   people sat on their asses and told me 'this needs to be fixed'
	 *   without offering any code for so simple a problem (saying that,
	 *   I'm sure you will find 20 things wrong with my code).  Before
	 *   you fly off the handle at me, remember how much WORSE the original
	 *   was.  --Doran (can you tell I'm a little annoyed?)
	 */

	static char *subjective[4] = { "", "it", "she", "he" };
	static char *possessive[4] = { "", "its", "her", "his" };
	static char *objective[4] = { "", "it", "her", "him" };
	static char *reflexive[4] = { "", "itself", "herself", "himself" };
	static char *absolute[4] = { "", "its", "hers", "his" };

	prn[0] = '%';
	prn[2] = '\0';

#ifdef COMPRESS
	str = uncompress(str);
#endif /* COMPRESS */

	tmp = tmp_buf;

	strcpy(tmp, str);

	result = buf;
	while (*tmp && lngth_sofar < (BUFFER_LEN - 1)) {
		lngth_sofar++;
		if (*tmp == '%' && tmp[1] != '\0') {
			*result = '\0';
			prn[1] = c = *(++tmp);
			self_sub = get_property_data(player, prn, access_rights(player,
					player, NOTHING));
			if (self_sub) {
				lngth_sofar += strlen(self_sub);
				if (lngth_sofar < BUFFER_LEN)
					strcat(result, self_sub);
				if (isupper(prn[1]))
					*result = toupper(*result);
				result += strlen(result);
				tmp++;
			} else if (genderof(player, access_rights(player, player, NOTHING))
					== GENDER_UNASSIGNED) {
				switch (c) {
				case 'n':
				case 'N':
				case 'o':
				case 'O':
				case 's':
				case 'S':
				case 'r':
				case 'R':
					lngth_sofar += strlen(unparse_name(player));
					if (lngth_sofar < BUFFER_LEN)
						strcat(result, unparse_name(player));
					break;
				case 'a':
				case 'A':
				case 'p':
				case 'P':
					lngth_sofar += strlen(unparse_name(player)) + 2;
					if (lngth_sofar < BUFFER_LEN) {
						strcat(result, unparse_name(player));
						strcat(result, "'s");
					}
					break;
				default:
					result[0] = *tmp;
					result[1] = 0;
					break;
				}
				tmp++;
				result += strlen(result);
			} else {
				switch (c) {
				case 'a':
				case 'A':
					lngth_sofar += strlen(absolute[genderof(player,
							access_rights(player, player, NOTHING))]);
					if (lngth_sofar < BUFFER_LEN)
						strcat(result, absolute[genderof(player, access_rights(
								player, player, NOTHING))]);
					break;
				case 's':
				case 'S':
					lngth_sofar += strlen(subjective[genderof(player,
							access_rights(player, player, NOTHING))]);
					if (lngth_sofar < BUFFER_LEN)
						strcat(result, subjective[genderof(player,
								access_rights(player, player, NOTHING))]);
					break;
				case 'p':
				case 'P':
					lngth_sofar += strlen(possessive[genderof(player,
							access_rights(player, player, NOTHING))]);
					if (lngth_sofar < BUFFER_LEN)
						strcat(result, possessive[genderof(player,
								access_rights(player, player, NOTHING))]);
					break;
				case 'o':
				case 'O':
					lngth_sofar += strlen(objective[genderof(player,
							access_rights(player, player, NOTHING))]);
					if (lngth_sofar < BUFFER_LEN)
						strcat(result, objective[genderof(player,
								access_rights(player, player, NOTHING))]);
					break;
				case 'r':
				case 'R':
					lngth_sofar += strlen(reflexive[genderof(player,
							access_rights(player, player, NOTHING))]);
					if (lngth_sofar < BUFFER_LEN)
						strcat(result, reflexive[genderof(player,
								access_rights(player, player, NOTHING))]);
					break;
				case 'n':
				case 'N':
					lngth_sofar += strlen(unparse_name(player));
					if (lngth_sofar < BUFFER_LEN)
						strcat(result, unparse_name(player));
					break;
				default:
					*result = *tmp;
					result[1] = '\0';
					break;
				}
				if (isupper(c) && islower(*result)) {
					*result = toupper(*result);
				}

				result += strlen(result);
				tmp++;
			}
		} else {
			*result++ = *tmp++;
		}
	}
	*result = '\0';
	return buf;
}

char *dup_string(char *string) {
	char *s;
	int i = 0;

	/* NULL, "" -> NULL */
	if (!string || !*string)
		return NULL;
	i = strlen(string) + 1;

	/*  if((s = (char *)calloc(strlen(string)+1, sizeof(char))) == 0) abort(); */
	if (!(s = (char *) calloc(i, sizeof(char))))
		abort();
	strcpy(s, string);
	return s;
}

char *dup_it(char *string) {
	char *s;

	/* NOTE!  This is NOT the same as dup_string! */

	/* NULL -> malloc'd "" */
	if (string == NULL)
		string = "";
	if ((s = (char *) calloc(strlen(string) + 1, sizeof(char))) == 0)
		abort();
	strcpy(s, string);
	return s;
}

void lowerstring(char *string) {
	while (*string) {
		*string = (isupper(*string)) ? tolower(*string) : *string;
		string++;
	}
}

void upperstring(char *string) {
	while (*string) {
		*string = (islower(*string)) ? toupper(*string) : *string;
		string++;
	}
}

int safe_chr(char c, char *buf, char **bufp) {
	char *tp; /* adds a character to a string, careful not to overflow buffer */
	int val;

	tp = *bufp;
	val = 0;

	if ((tp - buf) < BUFFER_LEN)
		*tp++ = c;
	else
		val = 1;

	*bufp = tp;
	return val;
}

int safe_copy_str(char *c, char *buff, char **bp, int maxlen) {
	/* copies a string into a buffer, making sure there's no overflow. */

	char *tp;

	tp = *bp;
	if (c == NULL)
		return 0;
	while (*c && ((tp - buff) < maxlen))
		*tp++ = *c++;
	*bp = tp;
	return strlen(c);
}
