#include "lisp.h"
#include <stdio.h>

void print_expr(Atom atom)
{
	switch (atom.type) {
	case AtomType_Nil:
		printf("NIL");
		break;
	case AtomType_Pair:
		putchar('(');
		print_expr(car(atom));
		atom = cdr(atom);
		while (!nilp(atom)) {
			if (atom.type == AtomType_Pair) {
				putchar(' ');
				print_expr(car(atom));
				atom = cdr(atom);
			} else {
				printf(" . ");
				print_expr(atom);
				break;
			}
		}
		putchar(')');
		break;
	case AtomType_Symbol:
		printf("%s", atom.value.symbol);
		break;
	case AtomType_Integer:
		printf("%ld", atom.value.integer);
		break;
	case AtomType_Builtin:
		printf("#<BUILTIN:%p>", atom.value.builtin);
		break;
	case AtomType_Closure:
		printf("#<CLOSURE:%p>", atom.value.pair);
		break;
	case AtomType_Macro:
		printf("#<MACRO:%p>", atom.value.pair);
		break;
	}
}

