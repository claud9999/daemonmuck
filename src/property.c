#include "copyright.h"
#include "config.h"
#include "db.h"
#include "externs.h"
#include "params.h"

#ifdef COMPRESS
#define alloc_compressed(x) dup_string(compress(x))
#else /* !COMPRESS */
#define alloc_compressed(x) dup_string(x)
#endif /* COMPRESS */

char buf2[BUFFER_LEN], buf[BUFFER_LEN];

static propdir *freehead = NULL;

propdir *find_see(propdir *p, char access);

propdir *new_propdir(void) {
	propdir *ret;

	if (freehead) {
		ret = freehead;
		freehead = freehead->next;
	} else {
		ret = (propdir *) malloc(sizeof(propdir));
	}

	return ret;
}

void delete_propdir(propdir *p) {
	p->next = freehead;
	freehead = p;
}

char *unparse_perms(char perms) {
	static char perms_buf[9];
	perms_buf[0] = (perms & PERMS_LOCKED) ? 'l' : '-';
	perms_buf[1] = (perms & PERMS_HIDDEN) ? 'h' : '-';
	perms_buf[2] = (perms & PERMS_COREAD) ? 'r' : '-';
	perms_buf[3] = (perms & PERMS_COWRITE) ? 'w' : '-';
	perms_buf[4] = (perms & PERMS_COSRCH) ? 's' : '-';
	perms_buf[5] = (perms & PERMS_OTREAD) ? 'r' : '-';
	perms_buf[6] = (perms & PERMS_OTWRITE) ? 'w' : '-';
	perms_buf[7] = (perms & PERMS_OTSRCH) ? 's' : '-';
	perms_buf[8] = '\0';

	return perms_buf;
}

char parse_perms(char *perms) {
	char ret = 0;
	if (isdigit(*perms))
		return (char) strtol(perms, NULL, 0);

	ret += (perms[0] == 'l') ? 0x80 : 0;
	ret += (perms[1] == 'h') ? 0x40 : 0;
	ret += (perms[2] == 'r') ? 0x20 : 0;
	ret += (perms[3] == 'w') ? 0x10 : 0;
	ret += (perms[4] == 's') ? 0x08 : 0;
	ret += (perms[5] == 'r') ? 0x04 : 0;
	ret += (perms[6] == 'w') ? 0x02 : 0;
	ret += (perms[7] == 's') ? 0x01 : 0;
	return ret;
}

char default_perms(char *line1) {
	switch (*line1) {
	case '.':
		return (PERMS_COREAD | PERMS_COWRITE | PERMS_COSRCH);
	case '_':
		return (PERMS_COREAD | PERMS_COWRITE | PERMS_COSRCH | PERMS_OTREAD
				| PERMS_OTSRCH);
	case '*':
		return (PERMS_COREAD | PERMS_COWRITE | PERMS_COSRCH | PERMS_HIDDEN);
	}
	return (PERMS_COREAD | PERMS_COWRITE | PERMS_COSRCH | PERMS_OTREAD
			| PERMS_OTWRITE | PERMS_OTSRCH);
}

char access_rights(dbref player, dbref obj, dbref program) {
	if (program != NOTHING) {
		if (FLAGS(program) & STICKY)
			player = OWNER(program);
		if ((FLAGS(program) & WIZARD) && (FLAGS(OWNER(program)) & WIZARD))
			return ACCESS_WI;
	} else if (FLAGS(player) & WIZARD)
		return ACCESS_WI;
	if (controls(player, obj))
		return ACCESS_CO;
	return ACCESS_OT;
}

int check_perms(char perms, char access, char type) {
	if (access & ACCESS_WI)
		return 1;
	if (access & ACCESS_CO) {
		switch (type) {
		case PT_SEE:
			return 1;
		case PT_CHANGE:
			return (!(perms & PERMS_LOCKED));
		case PT_READ:
			return (perms & PERMS_COREAD);
		case PT_WRITE:
			return (perms & PERMS_COWRITE);
		case PT_SRCH:
			return (perms & PERMS_COSRCH);
		}
	} else {
		switch (type) {
		case PT_SEE:
			return (!(perms & PERMS_HIDDEN));
		case PT_CHANGE:
			return 0;
		case PT_READ:
			return (perms & PERMS_OTREAD);
		case PT_WRITE:
			return (perms & PERMS_OTWRITE);
		case PT_SRCH:
			return (perms & PERMS_OTSRCH);
		}
	}
	return 0;
}

#define PERMS_DEFAULT_DIR PERMS_COREAD | PERMS_COWRITE | PERMS_COSRCH | \
  PERMS_OTREAD | PERMS_OTSRCH
propdir *setpropdir(propdir **p, char *name, char perms, char access) {
	propdir *tmp;

	for (tmp = *p; tmp && (string_compare(tmp->name, name)); tmp = tmp->next)
		;

	if (tmp) {
		if (!check_perms(tmp->perms, access, PT_SRCH))
			return NULL;
	} else {
		tmp = new_propdir();
		tmp->name = dup_string(name);
		tmp->data = NULL;
		if (access & ACCESS_WI)
			tmp->perms = perms | PERMS_DEFAULT_DIR;
		else
			tmp->perms = (perms | PERMS_DEFAULT_DIR) & ~ PERMS_LOCKED;
		tmp->next = *p;
		tmp->child = NULL;
		*p = tmp;
	}
	return tmp;
}

propdir *setpropelt(propdir **p, char *name, char *data, char perms,
		char access) {
	propdir *tmp;

	for (tmp = *p; tmp && (string_compare(tmp->name, name)); tmp = tmp->next)
		;

	if (tmp) {
		if (!check_perms(tmp->perms, access, PT_WRITE))
			return NULL;
		free(tmp->data);
	} else {
		tmp = new_propdir();
		tmp->name = dup_string(name);
		if (access & ACCESS_WI)
			tmp->perms = perms;
		else
			tmp->perms = perms & ~PERMS_LOCKED;
		tmp->next = *p;
		tmp->child = NULL;
		*p = tmp;
	}

	tmp->data = alloc_compressed(data);
	return tmp;
}

#define split(A,B) \
{ \
  A = B; \
  while (*B && (*B != '/')) B++; \
  if (*B) *B++ = '\0'; \
  while (*B == '/') B++; \
}

#define first(A) while (*A == '/') A++;

propdir *set_propdir(propdir *p, char *name, char *data, char perms,
		char access) {
	propdir *tmp, *loop, *ptmp, *tmp2;
	char *word;

	ptmp = p;

	first(name);
	if (*name) {
		split(word, name);
		if (*name)
			loop = setpropdir(&ptmp, word, perms, access);
		else
			loop = setpropelt(&ptmp, word, data, perms, access);
	} else
		return ptmp;

	while (*name && loop) {
		split(word, name);
		/* can only create new properties if propdir is writeable */
		if (check_perms(loop->perms, access, PT_WRITE)) {
			/* we can write to the propdir so let's do it! */
			if (*name)
				tmp = setpropdir(&(loop->child), word, perms, access);
			else
				tmp = setpropelt(&(loop->child), word, data, perms, access);
		} else {
			for (tmp2 = loop->child; tmp2 && string_compare(tmp2->name, word); tmp2
					= tmp2->next)
				;
			/* if property exists, use the perms on the property not the dir */
			if (tmp2) {
				if (*name)
					tmp = setpropdir(&(loop->child), word, perms, access);
				else
					tmp = setpropelt(&(loop->child), word, data, perms, access);
			} else
				return NULL;
			/* if not then can't create */
		}
		loop = tmp;
	}
	if (*name)
		return NULL;
	if (!loop)
		return NULL;
	return ptmp;
}

int add_property(dbref obj, char *name, char *data, char perms, char access) {
	propdir *p;
	p = set_propdir(DBFETCHPROP(obj), name, data, perms, access);
	if (p) {
		DBSTOREPROP(obj, p);
	}
	return (int) p;
}

void burn_proptree(propdir *p) {
	if (p) {
		free(p->name);
		if (p->data)
			free(p->data);
		burn_proptree(p->child);
		burn_proptree(p->next);
		/* free(p) */
		delete_propdir(p);
	}
}

propdir *burn_prop(propdir *p, char *name, int *status) {
	propdir *tmp, *loop;

	if (!p) {
		*status = 0;
		return NULL;
	} else
		*status = 1;

	if (string_compare(p->name, name)) {
		for (loop = p; loop->next && string_compare(loop->next->name, name); loop
				= loop->next)
			;

		if (loop->next) {
			tmp = loop->next;
			loop->next = tmp->next;
		} else {
			*status = 0;
			return p;
		}
	} else {
		tmp = p;
		p = p->next;
	}

	free(tmp->name);
	free(tmp->data);
	burn_proptree(tmp->child);
	/* free(tmp); */
	delete_propdir(tmp);

	return p;
}

/* returns 0 if successful, 1 if property not found, 2 if permission denied */
int remove_property(dbref obj, char *name, char access) {
	propdir *p, *tmp;
	char *word;
	int s;

	p = DBFETCHPROP(obj); /* forgot one Claudius! */

	if (!name || !*name) {
		burn_proptree(p);
		DBSTOREPROP(obj, NULL);
		return 0;
	}

	first(name);
	split(word, name);

	if (*name) {
		while (p && string_compare(p->name, word))
			p = p->next;
		if (!p)
			return 1;
	} else {
		tmp = burn_prop(p, word, &s);
		if (s) {
			DBSTOREPROP(obj, tmp);
			return 0;
		} else
			return 1;
	}

	split(word, name);

	while (*name && p->child) {
		if (check_perms(p->perms, access, PT_SRCH)) {
			for (p = p->child; p && string_compare(p->name, word); p = p->next)
				;
			if (!p)
				return 1;
			split(word, name);
		} else
			return 2;
	}

	if (p->child) {
		if (check_perms(p->perms, access, PT_WRITE)) {
			tmp = burn_prop(p->child, word, &s);
			if (s)
				p->child = tmp;
			else
				return 1;
		} else
			return 2;
	}
	return 0;
}

/*
 checks if object has property, returning 1 if it or any of it's
 contents has the property stated
 */
int has_property(dbref obj, char *name, char *data, char access) {
	if (validate_property(obj, name, data, access))
		return 1;

	for (obj = DBFETCH(obj)->contents; obj != NOTHING; obj = DBFETCH(obj)->next)
		if (validate_property(obj, name, data, access))
			return 1;

	return 0;
}

propdir *find_property(dbref obj, char *name, char access) {
	propdir *tmp;
	char *word;

	tmp = DBFETCHPROP(obj);
	first(name);

	if (!*name)
		return NULL;

	while (*name) {
		split(word, name);
		while (tmp && string_compare(tmp->name, word))
			tmp = tmp->next;
		if (*name) {
			if (tmp && check_perms(tmp->perms, access, PT_SRCH))
				tmp = tmp->child;
			else
				return NULL;
		}
	}
	return tmp;
}

int validate_property(dbref obj, char *name, char *data, char access) {
	propdir *p;
	if ((p = find_property(obj, name, access)) && p->data && check_perms(
			p->perms, access, PT_READ)) {
		return (!string_compare(uncompress(p->data), data));
	}
	return 0;
}

/* return class of property */
char *get_property_data(dbref obj, char *name, char access) {
	propdir *p;

	if ((p = find_property(obj, name, access)) && (check_perms(p->perms,
			access, PT_READ)))
		return uncompress(p->data);
	return NULL;
}

propdir *dup_propdir(propdir *src, char access) {
	propdir *new = NULL;
	if (src && check_perms(src->perms, access, PT_READ)) {
		/* new = (propdir *)malloc(sizeof(propdir)); */
		new = new_propdir();
		new->name = dup_string(src->name);
		new->data = dup_string(src->data);
		if (check_perms(src->perms, access, PT_SRCH))
			new->child = dup_propdir(src->child, access);
		new->next = dup_propdir(src->next, access);
	}
	return new;
}

/* copies properties */
void copy_prop(dbref obj, dbref dest, char access) {
	DBSTOREPROP(dest, dup_propdir(DBFETCHPROP(obj), access));
}

/* return old gender values for pronoun substitution code */
int genderof(dbref player, char access) {
	if (validate_property(player, "sex", "male", access))
		return GENDER_MALE;
	if (validate_property(player, "sex", "female", access))
		return GENDER_FEMALE;
	if (validate_property(player, "sex", "neuter", access))
		return GENDER_NEUTER;
	return GENDER_UNASSIGNED;
}

void notify_pdrecurse(dbref player, propdir *p, char *match, char access,
		char longform) {
	char *end;
	char *junk;
	char *tmp;

	for (; p; p = p->next) {
		for (junk = match; (*junk != '/') && *junk; junk++)
			;
		if (!stringn_compare(p->name, match, (int) (junk - match))) {
			end = buf2 + strlen(buf2);
			if ((p->child) && check_perms(p->perms, access, PT_SRCH)) {
				if (*junk) {
					strcat(buf2, p->name);
					strcat(buf2, "/");
					while (*junk == '/')
						junk++;
					notify_pdrecurse(player, p->child, junk, access, longform);
					*end = '\0';
				}
			}
			if ((!(p->perms & PERMS_HIDDEN) || (access == ACCESS_WI))) {
				if (longform) {
					notify(player, player, "%s:%s%s%s%s%s", unparse_perms(
							p->perms), buf2, p->name, p->child ? "/" : "",
							p->data ? ":" : "", check_perms(p->perms, access,
									PT_READ) ? p->data
									: "<<PERMISSION DENIED>>");
				} else {
					tmp = p->data;
					if (!check_perms(p->perms, access, PT_READ))
						notify(player, player, "<<PERMISSION DENIED>>");
					else if (p->data && *tmp)
						notify(player, player, p->data);
					else
						notify(player, player, "%s%s%s%s", buf2, p->name,
								p->child ? "/" : "", p->data ? ":" : "");
				}
			}
			*end = '\0';
		}
	}
}

void notify_propdir(dbref player, dbref obj, char *match, char access,
		char longform) {
	if (DBFETCHPROP(obj))
		notify_pdrecurse(player, DBFETCHPROP(obj), match, access, longform);
}

void change_perms(dbref obj, char *name, char perms, char access) {
	propdir *p;

	strcpy(buf, name);
	p = find_property(obj, buf, access);
	if (p && check_perms(p->perms, access, PT_CHANGE)) {
		if (access == ACCESS_WI)
			p->perms = perms;
		else
			p->perms = perms &= ~PERMS_LOCKED;
	}
}

propdir *find_see(propdir *p, char access) {
	while (p && !check_perms(p->perms, access, PT_SEE))
		p = p->next;
	return p;
}

int nextprop(char *buffer, propdir *p, char *name, char sub, char access) {
	char *word;
	propdir *tmp;

	if (!p)
		return 0;

	split(word, name);
	for (tmp = p; tmp && string_compare(uncompress(tmp->name), word); tmp
			= tmp->next)
		;
	if (tmp && name && *name) {
		if (nextprop(buffer, tmp->child, name, sub, access)) {
			strcpy(buf, uncompress(tmp->name));
			strcat(buf, "/");
			strcat(buf, buffer);
			strcpy(buffer, buf);
		} else {
			if ((tmp = find_see(tmp->next, access)))
				strcpy(buffer, uncompress(tmp->name));
			else
				return 0;
		}
	} else {
		if (!tmp)
			strcpy(buffer, uncompress(p->name));
		else {
			if (sub)
				p = find_see(tmp->child, access);
			else
				p = NULL;

			if (p) {
				strcpy(buffer, uncompress(tmp->name));
				strcat(buffer, "/");
				tmp = p;
			} else {
				strcpy(buffer, "");
				tmp = find_see(tmp->next, access);
			}

			if (tmp)
				strcat(buffer, uncompress(tmp->name));
			else
				return 0;
		}
	}
	return 1;
}

void next_property(char *buffer, dbref obj, char *name, char access) {
	if (name != NULL)
		first(name);
	if (name != NULL && (DBFETCHPROP(obj)))
		nextprop(buffer, DBFETCHPROP(obj), name, name[strlen(name) - 1] == '/',
				access);
	else if (DBFETCHPROP(obj))
		strcpy(buffer, DBFETCHPROP(obj)->name);
	else
		strcpy(buffer, "");
}

int has_next_property(dbref obj, char *name, char access, int child) {
	propdir *p;
	if (!(p = find_property(obj, name, access)))
		return 0;
	return ((int) find_see(child ? p->child : p->next, access));
}

void putproperties_recurse(FILE *f, propdir *p) {
	char *end;

	for (; p; p = p->next) {
		if (p->child) {
			end = buf2 + strlen(buf2);
			strcat(buf2, p->name);
			strcat(buf2, "/");
			putproperties_recurse(f, p->child);
			*end = '\0';
		}
		if (p->data) {
			fprintf(f, "%s%s:%d:%s\n", buf2, p->name, p->perms, p->data);
		}
	}
}

void putproperties(FILE *f, dbref obj) {
	strcpy(buf2, "");
	putproperties_recurse(f, DBFETCHPROP(obj));
}

void getproperties(FILE *f, dbref obj, char permflag) {
	char *data, *perms, getbuf[BUFFER_LEN];
	propdir *p = NULL;

	/* get rid of first line */
	fgets(getbuf, sizeof(getbuf), f);

	/* initialize first line stuff */
	fgets(getbuf, sizeof(getbuf), f);
	while (strcmp(getbuf, "***Property list end ***\n")) {
		data = (char *) strchr(getbuf, PROP_DELIMITER);
		if (data) {
			*data++ = '\0';
			if (permflag) /* permissions now stored in db... */
			{
				perms = data;
				data = (char *) strchr(data, PROP_DELIMITER);
				if (!data)
					abort(); /* PARSE ERROR */
				*data++ = '\0';
				data[strlen(data) - 1] = '\0';
				p = set_propdir(p, getbuf, data, atoi(perms), ACCESS_WI);
			} else {
				data[strlen(data) - 1] = '\0';
				p = set_propdir(p, getbuf, data, default_perms(getbuf),
						ACCESS_WI);
			}
		}
		fgets(getbuf, sizeof(getbuf), f);
	}
	DBSTOREPROP(obj, p);
}

int is_propdir(dbref player, dbref object1, char *string, dbref program) {
	propdir *p = NULL;

	p = find_property(object1, string, access_rights(player, object1, program));
	if (!p)
		return 0;
	if (!p->child)
		return 0;

	return 1;
}
