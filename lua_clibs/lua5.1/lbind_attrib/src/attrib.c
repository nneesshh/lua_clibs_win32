#include "attrib.h"

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

#define EXPRESSION_MAX 16
#define REGISTER_MAX 64
#define STACK_DEPTH_MAX 256

#define OPT_REG 0
#define OPT_ADD 1
#define OPT_SUB 2
#define OPT_MUL 3
#define OPT_DIV 4
#define OPT_NEG 5
#define OPT_SQR 6
#define OPT_END 7

static int tbl_priority_level[] = {
	-1,	// REG
	0,	// ADD
	0,	// SUB
	1,	// MUL
	1,	// DIV
	0,	// NEG 
	2,	// SQR
	-1, // END
};

#define MAX_PRIORITY_LEVEL 8

#define R_SHIFT 8 // register index shift
#define OPT_MASK 0xff // mask for operator

union rpn_stack {
	float constant;
	uint32_t r_opt; // register index | operator
};

struct attrib_expression {
	int er; // register index of expression
	int depth;
	union rpn_stack stack[1];
};

struct attrib_e {
	int ecount; // expression count
	struct attrib_expression * exps[EXPRESSION_MAX];
	int rcount; // register count
	int *depend[REGISTER_MAX];
};

struct attrib {
	struct attrib_e * e;
	bool dirty[REGISTER_MAX];
	float reg[REGISTER_MAX];
	bool calc;
};

struct attrib *
attrib_new() {
	struct attrib * a = malloc(sizeof(*a));
	memset(a, 0, sizeof(*a));
	return a;
}

void
attrib_delete(struct attrib * a) {
	free(a);
}

void
attrib_attach(struct attrib * a, struct attrib_e * e) {
	a->e = e;
	{
		int i;
		for (i = 0; i < e->rcount; ++i) {
			a->dirty[i] = true;
			a->reg[i] = 0;
		}
	}
	a->calc = false;
}

float
attrib_write(struct attrib * a, int r, float val) {
	float ret = a->reg[r];
	if (ret != val) {
		a->reg[r] = val;
		if (!a->dirty[r]) {
			a->dirty[r] = true;
			{
				int i;
				int * depend = a->e->depend[r];
				if (depend) {
					for (i = 0; depend[i] >= 0; ++i) {
						a->dirty[depend[i]] = true;
					}
					a->calc = false;
				}
			}
		}
	}
	return ret;
}

static void
_calc_exp(struct attrib_expression * exp, float *reg) {
	float v[STACK_DEPTH_MAX];
	int vp = 0;
	int i;
	for (i = 0; exp->stack[i].r_opt != ~OPT_END; ++i) {
		uint32_t r_opt = exp->stack[i].r_opt;
		if ((int)r_opt < 0) {
			r_opt = ~r_opt;
			int opt = r_opt & OPT_MASK;
			switch (opt) {
			case OPT_REG:
				assert(vp < STACK_DEPTH_MAX);
				v[vp++] = reg[r_opt >> R_SHIFT];
				break;
			case OPT_ADD:
				assert(vp >= 2);
				v[vp - 2] += v[vp - 1];
				--vp;
				break;
			case OPT_SUB:
				assert(vp >= 2);
				v[vp - 2] -= v[vp - 1];
				--vp;
				break;
			case OPT_MUL:
				assert(vp >= 2);
				v[vp - 2] *= v[vp - 1];
				--vp;
				break;
			case OPT_DIV:
				assert(vp >= 2);
				v[vp - 2] /= v[vp - 1];
				--vp;
				break;
			case OPT_NEG:
				assert(vp >= 1);
				v[vp - 1] = -v[vp - 1];
				break;
			case OPT_SQR:
				assert(vp >= 1);
				v[vp - 1] *= v[vp - 1];
				break;
			default:
				assert(0);
			}
		}
		else {
			assert(vp < STACK_DEPTH_MAX);
			v[vp++] = exp->stack[i].constant;
		}
	}
	assert(vp == 1);
	reg[exp->er] = v[0];
}

static void
_calc(struct attrib * a) {
	struct attrib_e * e = a->e;
	int i;
	for (i = 0; i < e->ecount; ++i) {
		struct attrib_expression * exp = e->exps[i];
		int r = exp->er;
		if (!a->dirty[r]) {
			continue;
		}
		_calc_exp(exp, a->reg);
	}
}

float
attrib_read(struct attrib * a, int r) {
	uint32_t idx = r;
	if (!a->calc) {
		_calc(a);
		assert(a->e);
		{
			int i;
			for (i = 0; i < a->e->rcount; ++i) {
				a->dirty[i] = false;
			}
		}
		a->calc = true;
	}
	return (idx < REGISTER_MAX) ? a->reg[idx] : 0.0f;
}

struct attrib_e *
attrib_enew() {
	struct attrib_e * e = malloc(sizeof(struct attrib_e));
	memset(e, 0, sizeof(*e));
	return e;
}

void
attrib_edelete(struct attrib_e * e) {
	int i;
	for (i = 0; i < e->ecount; ++i) {
		free(e->exps[i]);
	}
	for (i = 0; i < e->rcount; ++i) {
		free(e->depend[i]);
	}
	free(e);
}

static int
_read_int(const char **exp) {
	const char * p = *exp;
	int n = 0;
	while (*p >= '0' && *p <= '9') {
		n = n * 10 + (*p - '0');
		++p;
	}
	*exp = p;
	return n;
}

static float
_read_const(const char **exp) {
	const char * p = *exp;
	int n = 0;
	while (*p >= '0' && *p <= '9') {
		n = n * 10 + (*p - '0');
		++p;
	}
	float d = 0;
	float dn = 0.1f;
	if (*p == '.') {
		++p;
		while (*p >= '0' && *p <= '9') {
			d += dn * (*p - '0');
			dn *= 0.1f;
			++p;
		}
	}
	*exp = p;
	return d + n;
}

static int
_compile(const char * expression, union rpn_stack * rs, const char ** err, int *depth) {
	int rsp = 0; // rpn stack pointer
	int n = 0;
	bool neg = true;
	int tmp[STACK_DEPTH_MAX]; // operator tmp
	int tmpp = 0; // operator tmp pointer
	int base = 0;
	for (;;) {
		assert(rsp < STACK_DEPTH_MAX && tmpp < STACK_DEPTH_MAX);
		char c = *expression;
		switch (c) {
		case 'R':
			++expression;
			c = *expression;
			if (c < '0' || c > '9') {
				*err = "Register syntax error";
				return -1;
			}
			rs[rsp++].r_opt = ~(_read_int(&expression) << R_SHIFT | OPT_REG);
			++n;
			if (n > *depth) {
				*depth = n;
			}
			neg = false;
			break;
		case '0': case '1': case '2': case '3': case '4': case '5':
		case '6': case '7':	case '8': case '9': case '.': {
			float val = _read_const(&expression);
			++n;
			if (n > *depth) {
				*depth = n;
			}
			rs[rsp++].constant = val;
			neg = false;
			break;
		}
		case '(':
			base += MAX_PRIORITY_LEVEL;
			neg = true;
			++expression;
			break;
		case ')':
			base -= MAX_PRIORITY_LEVEL;
			if (base < 0) {
				*err = "Open bracket mismatch";
				return -1;
			}
			neg = false;
			++expression;
			break;
		case '\0': case '+': case '-': case '*': case '/': case '^': {
			++expression;
			int opt = 0;
			switch (c) {
			case '+':
				opt = OPT_ADD;
				neg = true;
				break;
			case '-':
				if (neg) {
					opt = OPT_NEG;
					neg = false;
				}
				else {
					opt = OPT_SUB;
					neg = true;
				}
				break;
			case '*':
				opt = OPT_MUL;
				neg = true;
				break;
			case '/':
				opt = OPT_DIV;
				neg = true;
				break;
			case '^':
				opt = OPT_SQR;
				neg = true;
				break;
			case '\0':
				opt = OPT_END;
				if (base != 0) {
					*err = "Close bracket mismatch";
					return -1;
				}
				break;
			default:
				assert(0);
			}
			if (tmpp == 0) {
				tmp[tmpp++] = base + opt;
				if (opt == OPT_END) {
					if (rsp == 0) {
						*err = "Empty expression";
						return -1;
					}
					if (n != 1) {
						*err = "Operator syntax error";
						return -1;
					}
					return rsp;
				}
			}
			else {
				int level = base + tbl_priority_level[opt];
				for (;;) {
					int to = tmp[tmpp - 1] % MAX_PRIORITY_LEVEL;
					int tl = tmp[tmpp - 1] - to + tbl_priority_level[to];
					if (level > tl) {
						tmp[tmpp++] = base + opt;
						break;
					}
					else {
						switch (to) {
						case OPT_ADD:
						case OPT_SUB:
						case OPT_MUL:
						case OPT_DIV:
							n--;
							break;
						case OPT_NEG:
						case OPT_SQR:
							break;
						default:
							assert(0);
						}
						rs[rsp++].r_opt = ~to;
						--tmpp;
						if (tmpp == 0) {
							if (opt == OPT_END) {
								if (n != 1) {
									*err = "Operator syntax error";
									return -1;
								}
								return rsp;
							}
							tmp[tmpp++] = base + opt;
							break;
						}
					}
				}
			}
			break;
		}
		default:
			*err = "Invalid charactor";
			return -1;
		}
	}
	return -2; // unknown error
}

static struct attrib_expression *
attrib_compile(const char * expression, const char ** err) {
	union rpn_stack rs[STACK_DEPTH_MAX];
	int depth = 0;
	int n = _compile(expression, rs, err, &depth);
	if (n > 0) {
		struct attrib_expression * exp = malloc(sizeof(*exp) + sizeof(union rpn_stack) * n);
		exp->er = -1;
		memcpy(exp->stack, rs, sizeof(union rpn_stack) * n);
		exp->stack[n].r_opt = ~OPT_END;
		exp->depth = depth;

		assert(exp->depth < STACK_DEPTH_MAX);
		return exp;
	}
	return NULL;
}

const char *
attrib_epush(struct attrib_e * e, int r, const char * expression) {
	const char * err = NULL;
	int i;
	for (i = 0;i < e->ecount;i++) {
		if (e->exps[i]->er == r) {
			return "Duplicate expression";
		}
	}
	struct attrib_expression * exp = attrib_compile(expression, &err);
	exp->er = r;
	e->exps[e->ecount++] = exp;
	return err;
}

static int
_top_sort(struct attrib_expression ** exp, int n) {
	assert(n < EXPRESSION_MAX);
	struct attrib_expression *tmp[EXPRESSION_MAX];
	int i, j;
	int max = 0;
	for (i = 0; i < n; ++i) {
		if (exp[i]->er > max) {
			max = exp[i]->er;
		}
		for (j = 0; exp[i]->stack[j].r_opt != ~OPT_END; ++j) {
			uint32_t r_opt = exp[i]->stack[j].r_opt;
			if ((int)r_opt < 0) {
				r_opt = ~r_opt;
				int r = r_opt >> R_SHIFT;
				if (r >= max) {
					max = r;
				}
			}
		}
	}
	++max;

	assert(max < REGISTER_MAX);

	bool rflag[REGISTER_MAX];
	memset(rflag, 0, sizeof(rflag));
	for (i = 0; i < n; ++i) {
		rflag[exp[i]->er] = true;
	}
	int count = n;
	int p = 0;
	int last_count = n;
	do {
		for (i = 0; i < n; ++i) {
			if (exp[i]) {
				for (j = 0; exp[i]->stack[j].r_opt != ~OPT_END; ++j) {
					uint32_t r_opt = exp[i]->stack[j].r_opt;
					if ((int)r_opt < 0) {
						r_opt = ~r_opt;
						int opt = r_opt & OPT_MASK;
						int r = r_opt >> R_SHIFT;
						if (opt == OPT_REG) {
							if (rflag[r]) {
								goto _next;
							}
						}
					}
				}
				tmp[p++] = exp[i];
				rflag[exp[i]->er] = false;
				exp[i] = NULL;
				--count;
			}
		_next:
			continue;
		}
		if (count == last_count) {
			return -1;
		}
	} while (count > 0);
	memcpy(exp, tmp, n * sizeof(struct attrib_expression *));
	return max;
}

static void
_insert_depend(struct attrib_e * e, int r1, int r2) {
	int * d = e->depend[r1];
	if (d == NULL) {
		d = malloc(sizeof(int) * (e->rcount + 1));
		memset(d, -1, sizeof(int) * (e->rcount + 1));
		e->depend[r1] = d;
	}
	d[r2] = 0;
}

static void
_mark_depend(int ** depend, int * root, bool *mark) {
	int i;
	for (i = 0; root[i] >= 0; ++i) {
		int r = root[i];
		if (mark[r])
			continue;
		mark[r] = true;
		int *branch = depend[r];
		if (branch) {
			_mark_depend(depend, branch, mark);
		}
	}
}

static void
_gen_depend(struct attrib_e * e) {
	int i, j;
	for (i = 0; i < e->ecount; ++i) {
		int er = e->exps[i]->er;
		for (j = 0; e->exps[i]->stack[j].r_opt != ~OPT_END; ++j) {
			uint32_t r_opt = e->exps[i]->stack[j].r_opt;
			if ((int)r_opt < 0) {
				r_opt = ~r_opt;
				int opt = r_opt & OPT_MASK;
				int r = r_opt >> R_SHIFT;
				if (opt == OPT_REG) {
					_insert_depend(e, r, er);
				}
			}
		}
	}
	for (i = 0; i < e->rcount; ++i) {
		int * d = e->depend[i];
		if (d) {
			int p = 0;
			for (j = 0; j < e->rcount; ++j) {
				if (d[j] == 0) {
					d[p++] = j;
				}
			}
			d[p] = -1;
		}
	}
	for (i = 0; i < e->rcount; ++i) {
		assert(e->rcount < REGISTER_MAX);
		bool mark[REGISTER_MAX];
		memset(mark, 0, sizeof(bool) * e->rcount);
		int * root = e->depend[i];
		if (root) {
			_mark_depend(e->depend, root, mark);
			int p = 0;
			for (j = 0; j < e->rcount; ++j) {
				if (mark[j]) {
					root[p++] = j;
				}
			}
			root[p] = -1;
		}
	}
}

const char *
attrib_einit(struct attrib_e * e) {
	int rcount = _top_sort(e->exps, e->ecount);
	if (rcount < 0) {
		return "Detected circular reference in topo sort";
	}
	assert(e->rcount == 0);
	e->rcount = rcount;
	memset(e->depend, 0, rcount * sizeof(int *));
	_gen_depend(e);

	return NULL;
}

#include <stdio.h>

static void
_dumpe(struct attrib_expression * exp) {
	int i = 0;
	while (exp->stack[i].r_opt != ~OPT_END) {
		uint32_t r_opt = exp->stack[i].r_opt;
		if ((int)r_opt < 0) {
			r_opt = ~r_opt;
			int opt = r_opt & OPT_MASK;
			int r = r_opt >> R_SHIFT;
			switch (opt) {
			case OPT_REG:
				printf("R%d ", r);
				break;
			case OPT_ADD:
				printf("+ ");
				break;
			case OPT_SUB:
				printf("- ");
				break;
			case OPT_MUL:
				printf("* ");
				break;
			case OPT_DIV:
				printf("/ ");
				break;
			case OPT_NEG:
				printf("_ ");
				break;
			case OPT_SQR:
				printf("^ ");
			}
		}
		else {
			printf("%g ", exp->stack[i].constant);
		}
		++i;
	}
	printf("\n");
}

void
attrib_edump(struct attrib_e * e) {
	int i;
	for (i = 0; i < e->ecount; ++i) {
		printf("R%d = ", e->exps[i]->er);
		_dumpe(e->exps[i]);
	}
	int j;
	for (i = 0; i < e->rcount; ++i) {
		int * d = e->depend[i];
		if (d) {
			printf("R%d : ", i);
			for (j = 0; d[j] >= 0; ++j) {
				printf("R%d ", d[j]);
			}
			printf("\n");
		}
	}
}
