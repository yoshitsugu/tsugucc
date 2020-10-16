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

Node *new_add(Node *lhs, Node *rhs)
{
    add_type(lhs);
    add_type(rhs);

    // int + int
    if (is_integer(lhs->ty) && is_integer(rhs->ty))
        return new_binary(ND_ADD, lhs, rhs);

    if (lhs->ty->base && rhs->ty->base)
        error("invalid operands");

    // Canonicalize `num + ptr` to `ptr + num`.
    if (!lhs->ty->base && rhs->ty->base)
    {
        Node *tmp = lhs;
        lhs = rhs;
        rhs = tmp;
    }

    // ptr + num
    rhs = new_binary(ND_MUL, rhs, new_node_num(4));
    return new_binary(ND_ADD, lhs, rhs);
}

static Node *new_sub(Node *lhs, Node *rhs)
{
    add_type(lhs);
    add_type(rhs);

    // num - num
    if (is_integer(lhs->ty) && is_integer(rhs->ty))
        return new_binary(ND_SUB, lhs, rhs);

    // ptr - num
    if (lhs->ty->base && is_integer(rhs->ty))
    {
        rhs = new_binary(ND_MUL, rhs, new_node_num(4));
        add_type(rhs);
        Node *node = new_binary(ND_SUB, lhs, rhs);
        node->ty = lhs->ty;
        return node;
    }

    // ptr - ptr, which returns how many elements are between the two.
    if (lhs->ty->base && rhs->ty->base)
    {
        Node *node = new_binary(ND_SUB, lhs, rhs);
        node->ty = ty_int;
        return new_binary(ND_DIV, node, new_node_num(8));
    }

    error("invalid operands");
}

Node *new_sizeof(Node *node)
{
    add_type(node);
    switch (node->ty->kind)
    {
    case TY_INT:
        return new_node_num(4);
    case TY_PTR:
        return new_node_num(8);
    }
    error("unknown type");
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
    expect_ident_str("int");
    head->var = push_var(expect_ident(), ty_int);
    VarList *cur = head;

    while (!consume(")"))
    {
        expect(",");
        cur->next = calloc(1, sizeof(VarList));
        expect_ident_str("int");
        cur->next->var = push_var(expect_ident(), ty_int);
        cur = cur->next;
    }

    return head;
}

Node *new_var(Var *var)
{
    Node *node = new_node(ND_VAR);
    node->var = var;
    return node;
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

    if (consume("int"))
    {
        Type *type = ty_int;

        while (consume("*"))
            type = pointer_to(type);
        char *ident = expect_ident();
        Var *var = push_var(ident, type);
        node = new_var(var);
        node->ty = type;
        expect(";");
        return node;
    }

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
            node = new_add(node, mul());
        else if (consume("-"))
            node = new_sub(node, mul());
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
    if (consume("sizeof"))
        return new_sizeof(unary());
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

Var *push_var(char *name, Type *type)
{
    Var *var = calloc(1, sizeof(Var));
    var->name = name;
    var->ty = type;

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
        if (var)
            return new_var(var);
    }
    return new_node_num(expect_number());
}
