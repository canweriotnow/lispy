#include <stdio.h>
#include <stdlib.h>
#include "mpc.h"

#include <editline/readline.h>
#include <editline/history.h>

/* Define Lisp value type */
typedef struct {
    int type;
    long num;
    int err;
} lval;

/* Create enum of poss. lval types */
enum { LVAL_NUM, LVAL_ERR };

/* Create enum of poss. error types */
enum { LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM };

/* Create a new number type lval */
lval lval_num(long x) {
    lval v;
    v.type = LVAL_NUM;
    v.num = x;
    return v;
}

/* Error type lval */
lval lval_err(int x) {
    lval v;
    v.type = LVAL_ERR;
    v.err = x;
    return v;
}

/* Print an lval */
void lval_print(lval v) {
    switch (v.type) {
    /* Print if it's a number */
    case LVAL_NUM: printf("%li", v.num); break;

    /* And for errors... */
    case LVAL_ERR:
        /* Check error type */
        if (v.err == LERR_DIV_ZERO) {
            printf("Error: Division By Zero!");
        }
        if (v.err == LERR_BAD_OP) {
            printf("Error: Invalid Operator!");
        }
        if (v.err == LERR_BAD_NUM) {
            printf("Error: Invalid Number!");
        }
        break;
    }
}

/* For prettiness sake... */
void lval_println(lval v) {
    lval_print(v); putchar('\n');
}


/*
   Write our own integer exponentiation function - the power
   functions in math.h only handle doubles and floats.
*/
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

lval eval_op(lval x, char* op, lval y) {

    /* If either value is an error return it */
    if (x.type == LVAL_ERR) { return x; }
    if (y.type == LVAL_ERR) { return y; }

    /* Else do maths on number values */
    if (strcmp(op, "+") == 0) { return lval_num(x.num + y.num); }
    if (strcmp(op, "-") == 0) { return lval_num(x.num - y.num); }
    if (strcmp(op, "*") == 0) { return lval_num(x.num * y.num); }
    if (strcmp(op, "/") == 0) {
        /* If 2nd arg is 0 then error */
        return y.num == 0
            ? lval_err(LERR_DIV_ZERO)
            : lval_num(x.num / y.num);
    }

    if (strcmp(op, "%") == 0) { return lval_num(x.num % y.num); }
    if (strcmp(op, "^") == 0) { return lval_num(power(x.num, y.num)); }

    return lval_err(LERR_BAD_OP);
}

lval eval(mpc_ast_t* t) {
    /* If tagged as numeric return directly */
    if (strstr(t->tag, "number")) {
        /* Check conversion error */
        errno = 0;
        long x = strtol(t->contents, NULL, 10);
        return errno != ERANGE ? lval_num(x) : lval_err(LERR_BAD_NUM);
    }

    /* op is 2nd child */
    char* op = t->children[1]->contents;
    lval x = eval(t->children[2]);

    /* Iterate remaining children and combine */
    int i = 3;
    while (strstr(t->children[i]->tag, "expr")) {
        x = eval_op(x, op, eval(t->children[i]));
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
    puts("Lispy Version 0.0.2");
    puts("Press Ctrl+c to Exit\n");

    /* loop */
    while(1) {
        char* input = readline("lispy> ");
        add_history(input);

        /* Attempt to parse */
        mpc_result_t r;
        if (mpc_parse("<stdin>", input, Lispy, &r)) {
            /* On success print and delete the AST */
            lval result = eval(r.output);
            lval_println(result);
            mpc_ast_delete(r.output);

        } else {
            /* Otherwise print and delete Error */
            mpc_err_print(r.error);
            mpc_err_delete(r.error);
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
