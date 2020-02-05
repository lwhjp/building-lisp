#include "lisp.h"
#include <stdio.h>
#include <stdlib.h>
#include <readline/readline.h>

char *slurp(const char *path)
{
	FILE *file;
	char *buf;
	long len;

	file = fopen(path, "r");
	if (!file)
		return NULL;
	fseek(file, 0, SEEK_END);
	len = ftell(file);
	fseek(file, 0, SEEK_SET);

	buf = malloc(len + 1);
	if (!buf)
		return NULL;

	fread(buf, 1, len, file);
	buf[len] = 0;
	fclose(file);

	return buf;
}

void load_file(Atom env, const char *path)
{
	char *text;

	printf("Reading %s...\n", path);
	text = slurp(path);
	if (text) {
		const char *p = text;
		Atom expr;
		while (read_expr(p, &p, &expr) == Error_OK) {
			Atom result;
			Error err = eval_expr(expr, env, &result);
			if (err) {
				printf("Error in expression:\n\t");
				print_expr(expr);
				putchar('\n');
			} else {
				print_expr(result);
				putchar('\n');
			}
		}
		free(text);
	}
}

int main(int argc, char **argv)
{
	Atom env;
	char *input;

	env = env_create(nil);

	/* Set up the initial environment */
	env_define(env, make_sym("CAR"), make_builtin(builtin_car));
	env_define(env, make_sym("CDR"), make_builtin(builtin_cdr));
	env_define(env, make_sym("CONS"), make_builtin(builtin_cons));
	env_define(env, make_sym("+"), make_builtin(builtin_add));
	env_define(env, make_sym("-"), make_builtin(builtin_subtract));
	env_define(env, make_sym("*"), make_builtin(builtin_multiply));
	env_define(env, make_sym("/"), make_builtin(builtin_divide));
	env_define(env, make_sym("T"), make_sym("T"));
	env_define(env, make_sym("="), make_builtin(builtin_numeq));
	env_define(env, make_sym("<"), make_builtin(builtin_less));
	env_define(env, make_sym("EQ?"), make_builtin(builtin_eq));
	env_define(env, make_sym("PAIR?"), make_builtin(builtin_pairp));
	env_define(env, make_sym("PROCEDURE?"), make_builtin(builtin_procp));

	load_file(env, "library.lisp");

	/* Main loop */
	while ((input = readline("> ")) != NULL) {
		const char *p = input;
		Error err;
		Atom expr, result;

		err = read_expr(p, &p, &expr);		

		if (!err)
			err = eval_expr(expr, env, &result);

		switch (err) {
		case Error_OK:
			print_expr(result);
			putchar('\n');
			break;
		case Error_Syntax:
			puts("Syntax error");
			break;
		case Error_Unbound:
			puts("Symbol not bound");
			break;
		case Error_Args:
			puts("Wrong number of arguments");
			break;
		case Error_Type:
			puts("Wrong type");
			break;
		}

		free(input);
	}

	return 0;
}

