#include "tsugucc.h"

Function *function();
Node *stmt();
Node *expr();
Node *assign();
Node *equality();
Node *relational();
Node *add();
Node *mul();
Node *unary();
Node *primary();

VarList *locals;

Node *new_node(NodeKind kind)
{
    Node *node = calloc(1, sizeof(Node));
    node->kind = kind;
    return node;
}

Node *new_unary(NodeKind kind, Node *lhs)
{
    Node *node = new_node(kind);
    node->lhs = lhs;
    return node;
}

Node *new_binary(NodeKind kind, Node *lhs, Node *rhs)
{
    Node *node = new_node(kind);
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

Node *new_node_num(int val)
{
    Node *node = new_node(ND_NUM);
    node->val = val;
    return node;
}

Var *find_var(Token *tok)
{
    for (VarList *vl = locals; vl; vl = vl->next)
    {
        Var *var = vl->var;
        if (strlen(var->name) == tok->len && !memcmp(tok->str, var->name, tok->len))
            return var;
    }
    return NULL;
}

Function *program()
{
    Function head;
    head.next = NULL;
    Function *cur = &head;

    while (!at_eof())
    {
        cur->next = function();
        cur = cur->next;
    }
    return head.next;
}

VarList *read_func_params()
{
    if (consume(")"))
        return NULL;

    VarList *head = calloc(1, sizeof(VarList));
    head->var = push_var(expect_ident());
    VarList *cur = head;

    while (!consume(")"))
    {
        expect(",");
        cur->next = calloc(1, sizeof(VarList));
        cur->next->var = push_var(expect_ident());
        cur = cur->next;
    }

    return head;
}

Function *function()
{
    locals = NULL;

    Function *fn = calloc(1, sizeof(Function));
    fn->name = expect_ident();
    expect("(");
    fn->params = read_func_params();
    expect("{");

    Node head;
    head.next = NULL;
    Node *cur = &head;

    while (!consume("}"))
    {
        cur->next = stmt();
        cur = cur->next;
    }

    fn->node = head.next;
    fn->locals = locals;
    return fn;
}

Node *stmt()
{
    Node *node;

    if (consume("return"))
    {
        node = new_node(ND_RETURN);
        node->lhs = expr();
        expect(";");
        return node;
    }

    if (consume("if"))
    {
        node = new_node(ND_IF);
        expect("(");
        node->cond = expr();
        expect(")");
        node->then = stmt();
        if (consume("else"))
            node->els = stmt();
        return node;
    }

    if (consume("while"))
    {
        node = new_node(ND_WHILE);
        expect("(");
        node->cond = expr();
        expect(")");
        node->then = stmt();
        return node;
    }

    if (consume("for"))
    {
        node = new_node(ND_FOR);
        expect("(");
        if (!consume(";"))
        {
            node->init = expr();
            expect(";");
        }
        if (!consume(";"))
        {
            node->cond = expr();
            expect(";");
        }
        if (!consume(")"))
        {
            node->inc = expr();
            expect(")");
        }
        node->then = stmt();
        return node;
    }

    if (consume("{"))
    {
        Node head;
        head.next = NULL;
        Node *cur = &head;

        while (!consume("}"))
        {
            cur->next = stmt();
            cur = cur->next;
        }

        Node *node = new_node(ND_BLOCK);
        node->body = head.next;
        return node;
    }

    node = expr();
    expect(";");
    return node;
}

Node *expr()
{
    return assign();
}

Node *assign()
{
    Node *node = equality();

    if (consume("="))
        node = new_binary(ND_ASSIGN, node, assign());
    return node;
}

Node *equality()
{
    Node *node = relational();

    for (;;)
    {
        if (consume("=="))
            node = new_binary(ND_EQ, node, relational());
        else if (consume("!="))
            node = new_binary(ND_NE, node, relational());
        else
            return node;
    }
}

Node *relational()
{
    Node *node = add();

    for (;;)
    {
        if (consume("<"))
            node = new_binary(ND_LT, node, add());
        else if (consume("<="))
            node = new_binary(ND_LE, node, add());
        else if (consume(">"))
            node = new_binary(ND_LT, add(), node);
        else if (consume(">="))
            node = new_binary(ND_LE, add(), node);
        else
            return node;
    }
}

Node *add()
{
    Node *node = mul();

    for (;;)
    {
        if (consume("+"))
            node = new_binary(ND_ADD, node, mul());
        else if (consume("-"))
            node = new_binary(ND_SUB, node, mul());
        else
            return node;
    }
}

Node *mul()
{
    Node *node = unary();

    for (;;)
    {
        if (consume("*"))
            node = new_binary(ND_MUL, node, unary());
        else if (consume("/"))
            node = new_binary(ND_DIV, node, unary());
        else
            return node;
    }
}

Node *unary()
{
    if (consume("+"))
        return primary();
    if (consume("-"))
        return new_binary(ND_SUB, new_node_num(0), primary());
    if (consume("&"))
        return new_unary(ND_ADDR, unary());
    if (consume("*"))
        return new_unary(ND_DEREF, unary());
    return primary();
}

Node *func_args()
{
    if (consume(")"))
        return NULL;

    Node *head = assign();
    Node *cur = head;
    while (consume(","))
    {
        cur->next = assign();
        cur = cur->next;
    }
    expect(")");
    return head;
}

Node *new_var(Var *var)
{
    Node *node = new_node(ND_VAR);
    node->var = var;
    return node;
}

Var *push_var(char *name)
{
    Var *var = calloc(1, sizeof(Var));
    var->name = name;

    VarList *vl = calloc(1, sizeof(VarList));
    vl->var = var;
    vl->next = locals;
    locals = vl;
    return var;
}

Node *primary()
{
    if (consume("("))
    {
        Node *node = expr();
        expect(")");
        return node;
    }

    Token *tok = consume_ident();
    if (tok)
    {
        Node *node;

        if (consume("("))
        {
            node = new_node(ND_FUNCALL);
            node->funcname = strndup(tok->str, tok->len);
            node->args = func_args();
            return node;
        }

        Var *var = find_var(tok);
        if (!var)
            var = push_var(strndup(tok->str, tok->len));
        return new_var(var);
    }

    return new_node_num(expect_number());
}
