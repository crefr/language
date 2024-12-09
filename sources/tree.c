#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "frontend.h"
#include "tree.h"

void printTreePrefix(fe_context_t * fe, node_t * node)
{
    assert(fe);

    if (node->type == END){
        printf("$\n");
        return;
    }

    if (node->type == NUM){
        printf("%lg", node->val.number);
        return;
    }

    if (node->type == VAR){
        printf("%s", fe->vars[node->val.var].name);
        return;
    }

    oper_t oper = opers[node->val.op];

    printf("(%s", oper.name);

    printf("(");
    printTreePrefix(fe, node->left);
    printf(")");

    if (oper.binary){
        printf("(");
        printTreePrefix(fe, node->right);
        printf(")");
    }
    printf(")");
}
