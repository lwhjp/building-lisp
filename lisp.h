typedef enum {
	Error_OK = 0,
	Error_Syntax,
	Error_Unbound,
	Error_Args,
	Error_Type
} Error;

struct Atom;

typedef int (*Builtin)(struct Atom args, struct Atom *result);

struct Atom {
	enum {
		AtomType_Nil,
		AtomType_Pair,
		AtomType_Symbol,
		AtomType_Integer,
		AtomType_Builtin,
		AtomType_Closure,
		AtomType_Macro
	} type;

	union {
		struct Pair *pair;
		const char *symbol;
		long integer;
		Builtin builtin;
	} value;
};

struct Pair {
	struct Atom atom[2];
};

typedef struct Atom Atom;

#define car(p) ((p).value.pair->atom[0])
#define cdr(p) ((p).value.pair->atom[1])
#define nilp(atom) ((atom).type == AtomType_Nil)

static const Atom nil = { AtomType_Nil };

/* READER */

int read_expr(const char *input, const char **end, Atom *result);

/* PRINTER */

void print_expr(Atom atom);

/* EVALUATOR */

Atom env_create(Atom parent);
int env_define(Atom env, Atom symbol, Atom value);
int env_get(Atom env, Atom symbol, Atom *result);
int env_set(Atom env, Atom symbol, Atom value);
int eval_expr(Atom expr, Atom env, Atom *result);

/* DATA */

Atom cons(Atom car_val, Atom cdr_val);
Atom make_int(long x);
Atom make_sym(const char *s);
Atom make_builtin(Builtin fn);
int listp(Atom expr);
Atom copy_list(Atom list);
Atom list_create(int n, ...);
Atom list_get(Atom list, int k);
void list_set(Atom list, int k, Atom value);
void list_reverse(Atom *list);
void gc_mark(Atom root);
void gc();

/* BUILTINS */

int builtin_car(Atom args, Atom *result);
int builtin_cdr(Atom args, Atom *result);
int builtin_cons(Atom args, Atom *result);
int builtin_eq(Atom args, Atom *result);
int builtin_pairp(Atom args, Atom *result);
int builtin_procp(Atom args, Atom *result);
int builtin_add(Atom args, Atom *result);
int builtin_subtract(Atom args, Atom *result);
int builtin_multiply(Atom args, Atom *result);
int builtin_divide(Atom args, Atom *result);
int builtin_numeq(Atom args, Atom *result);
int builtin_less(Atom args, Atom *result);

