#include "prims.h"
/* private globals */
extern inst *p_oper1, *p_oper2, *p_oper3, *p_oper4;
static inst temp1, temp2, temp3;
extern int p_result;
static int tmp;
extern int p_nargs;

static int gcd(register int a, register int b) {
	register int t;

	while (b > 0) {
		t = a % b;
		a = b;
		b = t;
	}
	return (a);
}

void prims_pop(__P_PROTO) {
	CHECKOP(1);
	p_oper1 = POP();
	CLEAR(p_oper1);
}

void prims_dup(__P_PROTO) {
	CHECKOP(1);
	if (*top >= STACK_SIZE)
		abort_interp("DUP:Stack overflow.");
	copyinst(&arg[*top - 1], &arg[*top]);
	(*top)++;
}

void prims_swap(__P_PROTO) {
	CHECKOP(2);
	p_oper1 = POP();
	temp2 = *(p_oper2 = POP());

	arg[(*top)++] = *p_oper1;
	arg[(*top)++] = temp2;
}

void prims_over(__P_PROTO) {
	CHECKOP(2);
	if (*top >= STACK_SIZE)
		abort_interp("OVER:Stack overflow.");
	copyinst(&arg[*top - 2], &arg[*top]);
	(*top)++;
}

void prims_pick(__P_PROTO) {
	CHECKOP(1);
	temp1 = *(p_oper1 = POP());
	if (temp1.type != PROG_INTEGER || temp1.data.number <= 0)
		abort_interp("Operand not a positive integer.");
	CHECKOP(temp1.data.number);
	copyinst(&arg[*top - temp1.data.number], &arg[*top]);
	(*top)++;
}

void prims_put(__P_PROTO) {
	CHECKOP(2);
	p_oper1 = POP();
	p_oper2 = POP();
	if (p_oper1->type != PROG_INTEGER || p_oper1->data.number <= 0)
		abort_interp("Operand not a positive integer.");
	tmp = p_oper1->data.number;
	CHECKOP(tmp);
	CLEAR(&arg[*top - tmp]);
	copyinst(p_oper2, &arg[*top - tmp]);
	CLEAR(p_oper1);
	CLEAR(p_oper2);
}

void prims_rot(__P_PROTO) {
	CHECKOP(3);
	p_oper1 = POP();
	p_oper2 = POP();
	temp3 = *(p_oper3 = POP());
	arg[(*top)++] = *p_oper2;
	arg[(*top)++] = *p_oper1;
	arg[(*top)++] = temp3;
}

void prims_rotate(__P_PROTO) {
	CHECKOP(1);
	p_oper1 = POP();
	if (p_oper1->type != PROG_INTEGER)
		abort_interp("Invalid argument type.");
	tmp = p_oper1->data.number; /* Depth on stack */
	CHECKOP(abs(tmp));
	if (tmp > 0) {
		temp2 = arg[*top - tmp];
		for (; tmp > 0; tmp--)
			arg[*top - tmp] = arg[*top - tmp + 1];
		arg[*top - 1] = temp2;
	} else if (tmp < 0) {
		temp2 = arg[*top - 1];
		for (tmp = -1; tmp > p_oper1->data.number; tmp--)
			arg[*top + tmp] = arg[*top + tmp - 1];
		arg[*top + tmp] = temp2;
	}
	CLEAR(p_oper1);
}

void prims_depth(__P_PROTO) {
	p_result = *top;
	if (*top >= STACK_SIZE)
		abort_interp("Stack overflow.");
	push(arg, top, PROG_INTEGER, MIPSCAST &p_result);
}

void prims_pstack(__P_PROTO) {
	int n = 0;
	char buffer[520];
#define BUFEND (&buffer[510]) /* allow a little slop */
	char *bp;
#define ADV (bp+=strlen(bp))
	int sp;

	CHECKOP(1);
	p_oper1 = POP();
	p_oper2 = 0;
	if (p_oper1->type == PROG_STRING) {
		CHECKOP(1);
		p_oper2 = p_oper1;
		p_oper1 = POP();
	}
	if (p_oper1->type != PROG_INTEGER)
		abort_interp("Count value not an integer.");
	n = p_oper1->data.number;
	CLEAR(p_oper1);
	bp = &buffer[0];
	snprintf(bp, 520, "%.*s> ( ", BUFEND - bp,
			p_oper2 ? DoNullInd(p_oper2->data.string) : "Stack");
	ADV;
	sp = *top - n;
	if (sp <= 0) {
		sp = 0;
	} else {
		snprintf(bp, 520, "...[%d]", sp);
		ADV;
	}
	for (; sp < *top; sp++) {
		char *el;
		if (sp) {
			strcpy(bp, ", ");
			ADV;
		}
		el = insttotext(arg + sp);
		if (bp + strlen(el) >= BUFEND) {
			strcpy(bp, "...");
			ADV;
			break;
		}
		strcpy(bp, el);
		ADV;
	}
	strcpy(bp, " )");
	notify(player, player, &buffer[0]);

#undef ADV
#undef BUFEND
	if (p_oper2)
		CLEAR(p_oper2);
}

void prims_roll(__P_PROTO) {
	register int i, jh, jt, n, m, tos;

	CHECKOP(2);
	p_oper2 = POP();
	p_oper1 = POP();
	if (p_oper1->type != PROG_INTEGER)
		abort_interp("Invalid argument type (1)");
	if (p_oper2->type != PROG_INTEGER)
		abort_interp("Invalid argument type (2)");
	n = p_oper1->data.number;
	m = p_oper2->data.number;
	CLEAR(p_oper1);
	CLEAR(p_oper2);
	if (n < 0)
		abort_interp("Negative argument (1)");
	if (n == 0)
		return;
	CHECKOP(n);
	if (n == 1)
		return;
	m %= n;
	if (m < 0)
		m += n;
	if (m == 0)
		return;
	/* to avoid needing m temporaries, we roll the stack by moving one piece
	 at a time.  We need gcd(m,n) pieces to get everything.  For example,
	 15 6 roll (stack is a b c d e f g h i j k l m n o) is done with
	 gcd(15,6)=3 chains of 15/3=5 copies each, with a stride of 6:
	 m->temp; g->m; a->g; j->a; d->j; temp->d
	 n->temp; h->n; b->h; k->b; e->k; temp->e
	 o->temp; i->o; c->i; l->c; f->l; temp->f
	 */
	tos = *top - 1;
	for (i = gcd(m, n) - 1; i >= 0; i--) {
		temp1 = arg[tos - i];
		jh = i;
		while (1) {
			jt = jh;
			jh = (jh + m) % n;
			if (jh == i)
				break;
			arg[tos - jt] = arg[tos - jh];
		}
		arg[tos - jt] = temp1;
	}
}
