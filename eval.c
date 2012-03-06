#include "lisp.h"
#include <string.h>

Atom env_create(Atom parent)
{
	return cons(parent, nil);
}

int env_define(Atom env, Atom symbol, Atom value)
{
	Atom bs = cdr(env);

	while (!nilp(bs)) {
		Atom b = car(bs);
		if (car(b).value.symbol == symbol.value.symbol) {
			cdr(b) = value;
			return Error_OK;
		}
		bs = cdr(bs);
	}

	cdr(env) = cons(cons(symbol, value), cdr(env));

	return Error_OK;
}

int env_get(Atom env, Atom symbol, Atom *result)
{
	Atom parent = car(env);
	Atom bs = cdr(env);

	while (!nilp(bs)) {
		Atom b = car(bs);
		if (car(b).value.symbol == symbol.value.symbol) {
			*result = cdr(b);
			return Error_OK;
		}
		bs = cdr(bs);
	}

	if (nilp(parent))
		return Error_Unbound;

	return env_get(parent, symbol, result);
}

int env_set(Atom env, Atom symbol, Atom value)
{
	Atom parent = car(env);
	Atom bs = cdr(env);

	while (!nilp(bs)) {
		Atom b = car(bs);
		if (car(b).value.symbol == symbol.value.symbol) {
			cdr(b) = value;
			return Error_OK;
		}
		bs = cdr(bs);
	}

	if (nilp(parent))
		return Error_Unbound;

	return env_set(parent, symbol, value);
}

int make_closure(Atom env, Atom args, Atom body, Atom *result)
{
	Atom p;

	if (!listp(body))
		return Error_Syntax;

	/* Check argument names are all symbols */
	p = args;
	while (!nilp(p)) {
		if (p.type == AtomType_Symbol)
			break;
		else if (p.type != AtomType_Pair
				|| car(p).type != AtomType_Symbol)
			return Error_Type;
		p = cdr(p);
	}

	*result = cons(env, cons(args, body));
	result->type = AtomType_Closure;

	return Error_OK;
}

Atom make_frame(Atom parent, Atom env, Atom tail)
{
	return cons(parent,
		cons(env,
		cons(nil, /* op */
		cons(tail,
		cons(nil, /* args */
		cons(nil, /* body */
		nil))))));
}

int eval_do_exec(Atom *stack, Atom *expr, Atom *env)
{
	Atom body;

	*env = list_get(*stack, 1);
	body = list_get(*stack, 5);
	*expr = car(body);
	body = cdr(body);
	if (nilp(body)) {
		/* Finished function; pop the stack */
		*stack = car(*stack);
	} else {
		list_set(*stack, 5, body);
	}

	return Error_OK;
}

int eval_do_bind(Atom *stack, Atom *expr, Atom *env)
{
	Atom op, args, arg_names, body;

	body = list_get(*stack, 5);
	if (!nilp(body))
		return eval_do_exec(stack, expr, env);

	op = list_get(*stack, 2);
	args = list_get(*stack, 4);

	*env = env_create(car(op));
	arg_names = car(cdr(op));
	body = cdr(cdr(op));
	list_set(*stack, 1, *env);
	list_set(*stack, 5, body);

	/* Bind the arguments */
	while (!nilp(arg_names)) {
		if (arg_names.type == AtomType_Symbol) {
			env_define(*env, arg_names, args);
			args = nil;
			break;
		}

		if (nilp(args))
			return Error_Args;
		env_define(*env, car(arg_names), car(args));
		arg_names = cdr(arg_names);
		args = cdr(args);
	}
	if (!nilp(args))
		return Error_Args;

	list_set(*stack, 4, nil);

	return eval_do_exec(stack, expr, env);
}

int eval_do_apply(Atom *stack, Atom *expr, Atom *env, Atom *result)
{
	Atom op, args;

	op = list_get(*stack, 2);
	args = list_get(*stack, 4);

	if (!nilp(args)) {
		list_reverse(&args);
		list_set(*stack, 4, args);
	}

	if (op.type == AtomType_Symbol) {
		if (strcmp(op.value.symbol, "APPLY") == 0) {
			/* Replace the current frame */
			*stack = car(*stack);
			*stack = make_frame(*stack, *env, nil);
			op = car(args);
			args = car(cdr(args));
			if (!listp(args))
				return Error_Syntax;

			list_set(*stack, 2, op);
			list_set(*stack, 4, args);
		}
	}

	if (op.type == AtomType_Builtin) {
		*stack = car(*stack);
		*expr = cons(op, args);
		return Error_OK;
	} else if (op.type != AtomType_Closure) {
		return Error_Type;
	}

	return eval_do_bind(stack, expr, env);
}

int eval_do_return(Atom *stack, Atom *expr, Atom *env, Atom *result)
{
	Atom op, args, body;

	*env = list_get(*stack, 1);
	op = list_get(*stack, 2);
	body = list_get(*stack, 5);

	if (!nilp(body)) {
		/* Still running a procedure; ignore the result */
		return eval_do_apply(stack, expr, env, result);
	}

	if (nilp(op)) {
		/* Finished evaluating operator */
		op = *result;
		list_set(*stack, 2, op);

		if (op.type == AtomType_Macro) {
			/* Don't evaluate macro arguments */
			args = list_get(*stack, 3);
			*stack = make_frame(*stack, *env, nil);
			op.type = AtomType_Closure;
			list_set(*stack, 2, op);
			list_set(*stack, 4, args);
			return eval_do_bind(stack, expr, env);
		}
	} else if (op.type == AtomType_Symbol) {
		/* Finished working on special form */
		if (strcmp(op.value.symbol, "DEFINE") == 0) {
			Atom sym = list_get(*stack, 4);
			(void) env_define(*env, sym, *result);
			*stack = car(*stack);
			*expr = cons(make_sym("QUOTE"), cons(sym, nil));
			return Error_OK;
		} else if (strcmp(op.value.symbol, "SET!") == 0) {
			Atom sym = list_get(*stack, 4);
			*stack = car(*stack);
			*expr = cons(make_sym("QUOTE"), cons(sym, nil));
			return env_set(*env, sym, *result);
		} else if (strcmp(op.value.symbol, "IF") == 0) {
			args = list_get(*stack, 3);
			*expr = nilp(*result) ? car(cdr(args)) : car(args);
			*stack = car(*stack);
			return Error_OK;
		} else {
			goto store_arg;
		}
	} else if (op.type == AtomType_Macro) {
		/* Finished evaluating macro */
		*expr = *result;
		*stack = car(*stack);
		return Error_OK;
	} else {
	store_arg:
		/* Store evaluated argument */
		args = list_get(*stack, 4);
		list_set(*stack, 4, cons(*result, args));
	}

	args = list_get(*stack, 3);
	if (nilp(args)) {
		/* No more arguments left to evaluate */
		return eval_do_apply(stack, expr, env, result);
	}

	/* Evaluate next argument */
	*expr = car(args);
	list_set(*stack, 3, cdr(args));
	return Error_OK;
}

int eval_expr(Atom expr, Atom env, Atom *result)
{
	static int count = 0;
	Error err = Error_OK;
	Atom stack = nil;

	do {
		if (++count == 100000) {
			gc_mark(expr);
			gc_mark(env);
			gc_mark(stack);
			gc();
			count = 0;
		}

		if (expr.type == AtomType_Symbol) {
			err = env_get(env, expr, result);
		} else if (expr.type != AtomType_Pair) {
			*result = expr;
		} else if (!listp(expr)) {
			return Error_Syntax;
		} else {
			Atom op = car(expr);
			Atom args = cdr(expr);

			if (op.type == AtomType_Symbol) {
				/* Handle special forms */

				if (strcmp(op.value.symbol, "QUOTE") == 0) {
					if (nilp(args) || !nilp(cdr(args)))
						return Error_Args;

					*result = car(args);
				} else if (strcmp(op.value.symbol, "DEFINE") == 0) {
					Atom sym;

					if (nilp(args) || nilp(cdr(args)))
						return Error_Args;

					sym = car(args);
					if (sym.type == AtomType_Pair) {
						err = make_closure(env, cdr(sym), cdr(args), result);
						sym = car(sym);
						if (sym.type != AtomType_Symbol)
							return Error_Type;
						(void) env_define(env, sym, *result);
						*result = sym;
					} else if (sym.type == AtomType_Symbol) {
						if (!nilp(cdr(cdr(args))))
							return Error_Args;
						stack = make_frame(stack, env, nil);
						list_set(stack, 2, op);
						list_set(stack, 4, sym);
						expr = car(cdr(args));
						continue;
					} else {
						return Error_Type;
					}
				} else if (strcmp(op.value.symbol, "LAMBDA") == 0) {
					if (nilp(args) || nilp(cdr(args)))
						return Error_Args;

					err = make_closure(env, car(args), cdr(args), result);
				} else if (strcmp(op.value.symbol, "IF") == 0) {
					if (nilp(args) || nilp(cdr(args)) || nilp(cdr(cdr(args)))
							|| !nilp(cdr(cdr(cdr(args)))))
						return Error_Args;

					stack = make_frame(stack, env, cdr(args));
					list_set(stack, 2, op);
					expr = car(args);
					continue;
				} else if (strcmp(op.value.symbol, "DEFMACRO") == 0) {
					Atom name, macro;

					if (nilp(args) || nilp(cdr(args)))
						return Error_Args;

					if (car(args).type != AtomType_Pair)
						return Error_Syntax;

					name = car(car(args));
					if (name.type != AtomType_Symbol)
						return Error_Type;

					err = make_closure(env, cdr(car(args)),
						cdr(args), &macro);
					if (!err) {
						macro.type = AtomType_Macro;
						*result = name;
						(void) env_define(env, name, macro);
					}
				} else if (strcmp(op.value.symbol, "APPLY") == 0) {
					if (nilp(args) || nilp(cdr(args)) || !nilp(cdr(cdr(args))))
						return Error_Args;

					stack = make_frame(stack, env, cdr(args));
					list_set(stack, 2, op);
					expr = car(args);
					continue;
				} else if (strcmp(op.value.symbol, "SET!") == 0) {
					if (nilp(args) || nilp(cdr(args)) || !nilp(cdr(cdr(args))))
						return Error_Args;
					if (car(args).type != AtomType_Symbol)
						return Error_Type;
					stack = make_frame(stack, env, nil);
					list_set(stack, 2, op);
					list_set(stack, 4, car(args));
					expr = car(cdr(args));
					continue;
				} else {
					goto push;
				}
			} else if (op.type == AtomType_Builtin) {
				err = (*op.value.builtin)(args, result);
			} else {
			push:
				/* Handle function application */
				stack = make_frame(stack, env, args);
				expr = op;
				continue;
			}
		}

		if (nilp(stack))
			break;

		if (!err)
			err = eval_do_return(&stack, &expr, &env, result);
	} while (!err);

	return err;
}

