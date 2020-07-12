#include "db.h"
#include "externs.h"

void dbreflist_dump(FILE *f, dbref_list *drl) {
	while (drl) {
		putref(f, drl->object);
		drl = drl->next;
	}
}

dbref_list *dbreflist_add(dbref_list *drl, dbref obj) {
	dbref_list *tmp = NULL;

	if (dbreflist_find(drl, obj))
		return drl;

	tmp = (dbref_list *) malloc(sizeof(dbref_list));
	tmp->object = obj;
	tmp->next = drl;

	return tmp;
}

dbref_list *dbreflist_remove(dbref_list *drl, dbref obj) {
	dbref_list *tmp = NULL, *loop = NULL;

	while (drl && (drl->object == obj)) {
		tmp = drl;
		drl = drl->next;
		free(tmp);
	}
	loop = drl;
	while (loop && loop->next) {
		while (loop && loop->next && (loop->next->object == obj)) {
			tmp = loop->next;
			loop->next = tmp->next;
			free(tmp);
		}
		if (loop)
			loop = loop->next;
	}
	return drl;
}

void dbreflist_burn(dbref_list *drl) {
	dbref_list *tmp = NULL;

	while (drl) {
		tmp = drl;
		drl = drl->next;
		free(tmp);
	}
}

dbref_list *dbreflist_read(FILE *f) {
	char buf[BUFFER_LEN];
	dbref_list *new = NULL, *list = NULL;

	fgets(buf, BUFFER_LEN, f);
	while (*buf != '*') {
		new = (dbref_list *) malloc(sizeof(dbref_list));
		new->object = (dbref) atol(buf);
		new->next = list;
		list = new;
		fgets(buf, BUFFER_LEN, f);
	}
	return list;
}

int dbreflist_find(dbref_list *drl, dbref obj) {
	while (drl && (drl->object != obj))
		drl = drl->next;
	return ((int) drl);
}
