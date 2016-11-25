#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mpc.h"
#include "util.h"

#include <editline/readline.h>
#include <editline/history.h>

#define LASSERT(args, cond, err) \
    if (!(cond)) { lval_del(args); return lval_err(err); }

/* Forward Declarations */

struct lval;
struct lenv;
typedef struct lval lval;
typedef struct lenv lenv;

void lval_del(lval* v);
lval* lval_copy(lval* v);
lval* lval_err(char* m);
lval* lval_eval(lenv* e, lval* v);


/* Lisp Value */
enum { LVAL_NUM, LVAL_ERR, LVAL_SYM,
       LVAL_FUN, LVAL_SEXPR, LVAL_QEXPR };

typedef lval*(*lbuiltin)(lenv*, lval*);

/* Define Lisp value type */
struct lval {
    int type;
    long num;
    /* Err and Sym types have some String data */
    char* err;
    char* sym;
    lbuiltin fun;
    /* cnt and ptr to list of 'lval' */
    int count;
    lval** cell;
};

struct lenv {
    int count;
    char** syms;
    lval** vals;
};

lenv* lenv_new(void) {
    lenv* e = malloc(sizeof(lenv));
    e->count = 0;
    e->syms = NULL;
    e->vals = NULL;
    return e;
}

void lenv_del(lenv* e) {
    for (int i = 0; i < e->count; i++) {
        free(e->syms[i]);
        lval_del(e->vals[i]);
    }
    free(e->syms);
    free(e->vals);
    free(e);
}

lval* lenv_get(lenv* e, lval* k) {

    /* Iterate over env */
    for (int i = 0; i < e->count; i++) {
        /**
         * Check if the stored string matches the symbols string;
         * If it does, return a copy of the value.
         **/
        if (strcmp(e->syms[i], k->sym) == 0) {
            return lval_copy(e->vals[i]);
        }
    }
    /* If not found return error */
    return lval_err("unbound symbol!");
}

void lenv_put(lenv* e, lval* k, lval* v) {
    /**
     * Iterate over all items in env;
     * This is to see if var already exists
     **/
    for (int i = 0; i < e->count; i++) {

        /**
         * If variable is found delete item at that position;
         * Replace with var supplied by user
         **/
        if (strcmp(e->syms[i], k->sym) == 0) {
            lval_del(e->vals[i]);
            e->vals[i] = lval_copy(v);
            return;
        }
    }

    /* If no existing entry found allocate space for new entry */
    e->count++;

    e->vals = realloc(e->vals, sizeof(lval*) * e->count);
    e->syms = realloc(e->syms, sizeof(char*) * e->count);

    /* Copy contents of lval and symbol string into new location */
    e->vals[e->count-1] = lval_copy(v);
    e->syms[e->count-1] = malloc(strlen(k->sym)+1);
    strcpy(e->syms[e->count-1], k->sym);
}



lval* lval_fun(lbuiltin func) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_FUN;
    v->fun = func;

    return v;
}

/* Create a new number type lval */
lval* lval_num(long x) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_NUM;
    v->num = x;
    return v;
}

/* Error type lval */
lval* lval_err(char* m) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_ERR;
    v->err = malloc(strlen(m) + 1);
    strcpy(v->err, m);
    return v;
}

/* ptr to sym type lval */
lval* lval_sym(char* s) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_SYM;
    v->sym = malloc(strlen(s) + 1);
    strcpy(v->sym, s);
    return v;
}

lval* lval_sexpr(void) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_SEXPR;
    v->count = 0;
    v->cell = NULL;
    return v;
}

lval* lval_qexpr(void) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_QEXPR;
    v->count = 0;
    v->cell = NULL;
    return v;
}

void lval_del(lval* v) {

    switch (v->type) {
        /* Nothing special for number type */
    case LVAL_NUM: break;
    case LVAL_FUN: break;

        /* For Err or Sym free str data */
    case LVAL_ERR: free(v->err); break;
    case LVAL_SYM: free(v->sym); break;

        /* If Sexpr or Qexpr then delete all elements inside */
    case LVAL_QEXPR:
    case LVAL_SEXPR:
        for (int i = 0; i < v->count; i++) {
            lval_del(v->cell[i]);
        }
        /* Also free memory alloc'd to ptrs */
        free(v->cell);
        break;
    }

    /* Free memory allocated to the lval struct itself */
    free(v);
}

lval* lval_add(lval* v, lval* x) {
    v->count++;
    v->cell = realloc(v->cell, sizeof(lval*) * v->count);
    v->cell[v->count-1] = x;
    return v;
}

lval* lval_copy(lval* v) {
    lval* x = malloc(sizeof(lval));
    x->type = v->type;

    switch (v->type) {
        /* Copy functions and numbers directly */
    case LVAL_FUN: x->fun = v->fun; break;
    case LVAL_NUM: x->num = v->num; break;

        /* Copy strings using malloc and strcpy */
    case LVAL_ERR:
        x->err = malloc(strlen(v->err) + 1);
        strcpy(x->err, v->err); break;

    case LVAL_SYM:
        x->sym = malloc(strlen(v->sym) +1);
        strcpy(x->err, v->err); break;

        /* Copy Lists by copying sub-exps */
    case LVAL_SEXPR:
    case LVAL_QEXPR:
        x->count = v->count;
        x->cell = malloc(sizeof(lval*) * x->count);

        for (int i = 0; i < x->count; i++) {
            x->cell[i] = lval_copy(v->cell[i]);
        }
        break;

    }
    return x;
}

lval* lval_pop(lval* v, int i) {
    /* Find the item at "i" */
    lval* x = v-> cell[i];

    /* Shift memory after the item at "i" over the top */
    memmove(&v->cell[i], &v->cell[i+1],
            sizeof(lval*) * (v->count-i-1));

    /* Decrement count of items in list */
    v->count--;

    /* Reallocate memory used */
    v->cell = realloc(v->cell, sizeof(lval*) * v->count);
    return x;
}

lval* lval_take(lval* v, int i) {
    lval* x = lval_pop(v, i);
    lval_del(v);
    return x;
}


void lval_print(lval* v);

void lval_expr_print(lval* v, char open, char close) {
    putchar(open);
    for (int i = 0; i < v->count; i++) {

        lval_print(v->cell[i]);

        if (i != (v-> count-1)) {
            putchar(' ');
        }
    }
    putchar(close);
}


void lval_print(lval* v) {
    switch (v->type) {

    case LVAL_NUM:   printf("%li", v->num); break;
    case LVAL_FUN:   printf("<function>"); break;
    case LVAL_ERR:   printf("Error: %s", v->err); break;
    case LVAL_SYM:   printf("%s", v->sym); break;
    case LVAL_SEXPR: lval_expr_print(v, '(', ')'); break;
    case LVAL_QEXPR: lval_expr_print(v, '{', '}'); break;
    }
}

/* For prettiness sake... */
void lval_println(lval* v) {
    lval_print(v); putchar('\n');
}

lval* builtin_op(lenv* e, lval* a, char* op) {

    /* Ensure all args anre numbers */
    for (int i = 0; i < a->count; i++) {
        if (a->cell[i]->type != LVAL_NUM) {
            lval_del(a);
            return lval_err("Cannot operate on non-number!");
        }
    }

    /* Pop first element */
    lval* x = lval_pop(a, 0);

    /* If no arguments and sub then perform unary negation */
    if ((strcmp(op, "-") == 0) && a->count == 0) {
        x->num = -x->num;
    }

    /* While there are still elements remaining */
    while (a->count > 0) {

        /* Pop the next element */
        lval* y = lval_pop(a, 0);

        /* Perform operation */
        if (strcmp(op, "+") == 0) { x->num += y->num; }
        if (strcmp(op, "-") == 0) { x->num -= y->num; }
        if (strcmp(op, "*") == 0) { x->num *= y->num; }
        if (strcmp(op, "/") == 0) {
            if (y->num == 0) {
                lval_del(x); lval_del(y);
                x = lval_err("Division by zero");
                break;
            }
            x->num /= y->num;
        }
        if (strcmp(op, "%") == 0) {x->num %= y->num; }
        if (strcmp(op, "^") == 0) {x->num = power(x->num, y->num); }

        /* Delete element now finished with */
        lval_del(y);
    }

    /* Delete input expr and return result */
    lval_del(a);
    return x;
}

/* list funs */
lval* builtin_head(lenv* e, lval* a) {
    // Check error conditions
    LASSERT(a, a->count ==1,
            "Function 'head' passed too mnay args");
    LASSERT(a, a->cell[0]->type == LVAL_QEXPR,
            "Function 'head passed incorrect type!'");
    LASSERT(a, a->cell[0]->count !=0,
            "Function 'head' passed {}!");

    /* Otherwise take first arg */
    lval* v = lval_take(a, 0);

    /* Delete tail and return */
    while (v->count > 1) { lval_del(lval_pop(v, 1)); }
    return v;
}

lval* builtin_tail(lenv* e, lval* a) {
    // Check error conditions
    LASSERT(a, a->count ==1,
            "Function 'head' passed too mnay args");
    LASSERT(a, a->cell[0]->type == LVAL_QEXPR,
            "Function 'head passed incorrect type!'");
    LASSERT(a, a->cell[0]->count !=0,
            "Function 'head' passed {}!");

    /* Take the first arg */
    lval* v = lval_take(a, 0);

    // Delete first elem & return
    lval_del(lval_pop(v, 0));
    return v;
}

lval* builtin_list(lenv* e, lval* a) {
    a->type = LVAL_QEXPR;
    return a;
}


lval* builtin_eval(lenv* e, lval* a) {
    LASSERT(a, a->count == 1,
            "Function 'eval' passed too many arguments");
    LASSERT(a, a->cell[0]->type == LVAL_QEXPR,
            "Function 'eval' passed incorrect type!'");

    lval* x = lval_take(a, 0);
    x->type = LVAL_SEXPR;
    return lval_eval(e, x);
}

lval* lval_join(lenv* e, lval* x, lval* y) {

    /* For each cell in 'y' add it to 'x' */
    while (y->count) {
        x = lval_add(x, lval_pop(y, 0));
    }

    lval_del(y);
    return x;
}

lval* builtin_join(lenv* e, lval* a) {

    for (int i = 0; i < a->count; i++) {
        LASSERT(a, a->cell[i]->type == LVAL_QEXPR,
            "Function 'join' passed incorrect type.")
    }

    lval* x = lval_pop(a, 0);

    while (a->count) {
        x = lval_join(e, x, lval_pop(a, 0));
    }

    lval_del(a);
    return x;
}

lval* builtin_len(lenv* e, lval* a) {
    LASSERT(a, a->cell[0]->type == LVAL_QEXPR,
            "Function 'len' passed incorrect type");

    lval* x = lval_num(a->cell[0]->count);
    return x;
}

lval* builtin_add(lenv* e, lval* a) {
    return builtin_op(e, a, "+");
}

lval* builtin_sub(lenv* e, lval* a) {
    return builtin_op(e, a, "-");
}

lval* builtin_mul(lenv* e, lval* a) {
    return builtin_op(e, a, "*");
}

lval* builtin_div(lenv* e, lval* a) {
    return builtin_op(e, a, "/");
}

lval* builtin_mod(lenv* e, lval* a) {
    return builtin_op(e, a, "%");
}

lval* builtin_pow(lenv* e, lval* a) {
    return builtin_op(e, a, "^");
}

void lenv_add_builtin(lenv* e, char* name, lbuiltin func) {
    lval* k = lval_sym(name);
    lval* v = lval_fun(func);
    lenv_put(e, k, v);
    lval_del(k); lval_del(v);
}

void lenv_add_builtins(lenv* e) {
    /* List Function */
    lenv_add_builtin(e, "list", builtin_list);
    lenv_add_builtin(e, "head", builtin_head);
    lenv_add_builtin(e, "tail", builtin_tail);
    lenv_add_builtin(e, "eval", builtin_eval);
    lenv_add_builtin(e, "join", builtin_join);
    lenv_add_builtin(e, "len", builtin_len);

    /* Mathematical Functions */
    lenv_add_builtin(e, "+", builtin_add);
    lenv_add_builtin(e, "-", builtin_sub);
    lenv_add_builtin(e, "*", builtin_mul);
    lenv_add_builtin(e, "/", builtin_div);
    lenv_add_builtin(e, "%", builtin_mod);
    lenv_add_builtin(e, "^", builtin_pow);
}





lval* lval_eval_sexpr(lenv* e, lval* v) {

    /* Evaluate Children */
    for (int i = 0; i < v->count; i++) {
        v->cell[i] = lval_eval(e, v->cell[i]);
    }

    /* Error Checking */
    for (int i = 0; i < v->count; i++) {
        if (v->cell[i]->type == LVAL_ERR) { return lval_take(v, i); }
    }

    /* Empty Expr */
    if (v->count == 0) { return v; }

    /*Single Expr */
    if (v->count == 1) { return lval_take(v, 0); }

    /* Ensure first element is function after eval */
    lval* f = lval_pop(v, 0);
    if (f->type != LVAL_FUN) {
        lval_del(v); lval_del(f);
        return lval_err("First element is not a function");
    }

    /* Call builtin with operator */
    lval* result = f->fun(e, v);
    lval_del(f);
    return result;
}

lval* lval_eval(lenv* e, lval* v) {
    if (v->type == LVAL_SYM) {
        lval* x = lenv_get(e, v);
        lval_del(v);
        return x;
    }
    /* Evaluate Sexpressions */
    if (v->type == LVAL_SEXPR) { return lval_eval_sexpr(e, v); }
    /* All other types remain the same */
    return v;
}

lval* lval_read_num(mpc_ast_t* t) {
    errno = 0;
    long x = strtol(t->contents, NULL, 10);
    return errno != ERANGE ?
        lval_num(x) : lval_err("invalid number");
}

lval* lval_read(mpc_ast_t* t) {
    /* If Symbol or Number return conversion to that type */
    if (strstr(t->tag, "number")) { return lval_read_num(t); }
    if (strstr(t->tag, "symbol")) {return lval_sym(t->contents); }

    /* If root (>) or sexpr then create empty list */
    lval* x = NULL;
    if (strcmp(t->tag, ">") == 0) { x = lval_sexpr(); }
    if (strstr(t->tag, "sexpr"))  { x = lval_sexpr(); }
    if (strstr(t->tag, "qexpr"))  { x = lval_qexpr(); }

    /* Fill this list with any valid expression contained within */
    for (int i = 0; i < t->children_num; i++) {
        if (strcmp(t->children[i]->contents, "(") == 0) { continue; }
        if (strcmp(t->children[i]->contents, ")") == 0) { continue; }
        if (strcmp(t->children[i]->contents, "}") == 0) { continue; }
        if (strcmp(t->children[i]->contents, "{") == 0) { continue; }
        if (strcmp(t->children[i]->tag,  "regex") == 0) { continue; }
        x = lval_add(x, lval_read(t->children[i]));
    }

    return x;
}


int main(int argc, char** argv) {
    /* Create Some Parsers */
    mpc_parser_t* Number    = mpc_new("number");
    mpc_parser_t* Symbol    = mpc_new("symbol");
    mpc_parser_t* Sexpr     = mpc_new("sexpr");
    mpc_parser_t* Qexpr     = mpc_new("qexpr");
    mpc_parser_t* Expr      = mpc_new("expr");
    mpc_parser_t* Lispy     = mpc_new("lispy");

    /*Define them with the following language */
    mpca_lang(MPCA_LANG_DEFAULT,
    "                                                     \
     number   : /-?[0-9]+/ ;                              \
     symbol   : /[a-zA-Z0-9_+\\-*\\/\\\\=<>!&%\\^]+/ ;        \
     sexpr    : '(' <expr>* ')' ;                         \
     qexpr    : '{' <expr>* '}' ;                         \
     expr     : <number> | <symbol> | <sexpr> | <qexpr> ; \
     lispy    : /^/ <expr>* /$/ ;                         \
    ",
              Number, Symbol, Sexpr, Qexpr, Expr, Lispy);

    /* Print version and exit info */
    puts("Lispy Version 0.0.5");
    puts("Press Ctrl+c to Exit\n");

    lenv* e = lenv_new();
    lenv_add_builtins(e);

    /* loop */
    while(1) {
        char* input = readline("lispy> ");
        add_history(input);

        /* Attempt to parse */
        mpc_result_t r;
        if (mpc_parse("<stdin>", input, Lispy, &r)) {
            /* On success print and delete the AST */
            lval* x = lval_eval(e, lval_read(r.output));
            lval_println(x);
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

    lenv_del(e);

    /* Undef and delete parsers */
    mpc_cleanup(6, Number, Symbol, Sexpr, Qexpr, Expr, Lispy);
    return 0;
}
