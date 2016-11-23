#include <stdio.h>
#include <stdlib.h>
#include "mpc.h"

#include <editline/readline.h>
#include <editline/history.h>

long power(long base, long exp) {
    if (exp == 0)
        return 1;
    else if (exp % 2)
        return base * power(base, exp - 1);
    else {
        long temp = power(base, exp / 2);
        return temp * temp;
    }
}

long eval_op(long x, char* op, long y) {
    if (strcmp(op, "+") == 0) { return x + y; }
    if (strcmp(op, "-") == 0) { return x - y; }
    if (strcmp(op, "*") == 0) { return x * y; }
    if (strcmp(op, "/") == 0) { return x / y; }
    if (strcmp(op, "%") == 0) { return x % y; }
    if (strcmp(op, "^") == 0) { return power(x, y); }

    return 0;
}

long eval(mpc_ast_t* t) {
    /* If tagged as numeric return directly */
    if (strstr(t->tag, "number")) {
        return atoi(t->contents);
    }

    /* op is 2nd child */
    char* op = t->children[1]->contents;

    /* Store 3rd ch in `x` */
    long x = eval(t->children[2]);

    /* Iterate remaining children and combine */
    int i = 3;
    while (strstr(t->children[i]->tag, "expr")) {
        x= eval_op(x, op, eval(t->children[i]));
        i++;
    }

    return x;
}


int main(int argc, char** argv) {
    /* Create Some Parsers */
    mpc_parser_t* Number    = mpc_new("number");
    mpc_parser_t* Operator  = mpc_new("operator");
    mpc_parser_t* Expr      = mpc_new("expr");
    mpc_parser_t* Lispy     = mpc_new("lispy");

    /*Define them with the following language */
    mpca_lang(MPCA_LANG_DEFAULT,
    "                                                   \
     number   : /-?[0-9]+/ ;                                      \
     operator : '+' | '-' | '*' | '/' | '%' | '^';                 \
     expr     : <number> | '(' <operator> <expr>+ ')' ; \
     lispy    : /^/ <operator> <expr>+ /$/ ;            \
    ",
              Number, Operator, Expr, Lispy);

    /* Print version and exit info */
    puts("Lispy Version 0.0.0.0.1");
    puts("Press Ctrl+c to Exit\n");

    /* loop */
    while(1) {
        char* input = readline("lispy> ");
        add_history(input);

        /* Attempt to parse */
        mpc_result_t r;
        if (mpc_parse("<stdin>", input, Lispy, &r)) {
            /* On success print and delete the AST */
            long result = eval(r.output);
            printf("%li\n", result);
            mpc_ast_delete(r.output);

        } else {
            /* Otherwise print and delete Error */
            mpc_err_print(r.error);
            mpc_err_print(r.error);
        }

        /* Echo input back yo user */
        //printf("No, your a %s\n", input);

        /* Free retrieved input */
        free(input);
    }

    /* Undef and delete parsers */
    mpc_cleanup(4, Number, Operator, Expr, Lispy);
    return 0;
}
