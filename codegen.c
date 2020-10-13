#include "tsugucc.h"

int jmp_label_count = 0;
char *argreg[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};
char *funcname;

// Pushes the given node's address to the stack.
void gen_addr(Node *node)
{
    if (node->kind == ND_VAR)
    {
        printf("  lea rax, [rbp-%d]\n", node->var->offset);
        printf("  push rax\n");
        return;
    }

    error("not an lvalue");
}

void gen(Node *node)
{
    int seq;
    switch (node->kind)
    {
    case ND_RETURN:
        gen(node->lhs);
        printf("  pop rax\n");
        printf("  jmp .Lreturn.%s\n", funcname);
        return;
    case ND_BLOCK:
        for (Node *n = node->body; n; n = n->next)
            gen(n);
        return;
    case ND_FUNCALL:
    {
        int nargs = 0;
        for (Node *arg = node->args; arg; arg = arg->next)
        {
            gen(arg);
            nargs++;
        }

        for (int i = nargs - 1; i >= 0; i--)
            printf("  pop %s\n", argreg[i]);

        jmp_label_count++;
        seq = jmp_label_count;

        // We need to align RSP to a 16 byte boundary before
        // calling a function because it is an ABI requirement.
        // RAX is set to 0 for variadic function.
        printf("  mov rax, rsp\n");
        printf("  and rax, 15\n");
        printf("  jnz .Lcall%d\n", seq);
        printf("  mov rax, 0\n");
        printf("  call %s\n", node->funcname);
        printf("  jmp .Lend%d\n", seq);
        printf(".Lcall%d:\n", seq);
        printf("  sub rsp, 8\n");
        printf("  mov rax, 0\n");
        printf("  call %s\n", node->funcname);
        printf("  add rsp, 8\n");
        printf(".Lend%d:\n", seq);
        printf("  push rax\n");
        return;
    }
    case ND_IF:
        jmp_label_count++;
        seq = jmp_label_count;
        gen(node->cond);
        printf("  pop rax\n");
        printf("  cmp rax, 0\n");
        if (node->els)
        {
            printf("  je .Lelse%d\n", seq);
            gen(node->then);
            printf("  jmp .Lend%d\n", seq);
            printf(".Lelse%d:\n", seq);
            gen(node->els);
            printf(".Lend%d:\n", seq);
        }
        else
        {
            printf("  je .Lend%d\n", seq);
            gen(node->then);
            printf(".Lend%d:\n", seq);
        }
        return;
    case ND_WHILE:
        jmp_label_count++;
        seq = jmp_label_count;
        printf(".Lbegin%d:\n", seq);
        gen(node->cond);
        printf("  pop rax\n");
        printf("  cmp rax, 0\n");
        printf("  je .Lend%d\n", seq);
        gen(node->then);
        printf("  jmp .Lbegin%d\n", seq);
        printf(".Lend%d:\n", seq);
        return;
    case ND_FOR:
        jmp_label_count++;
        seq = jmp_label_count;
        if (node->init)
            gen(node->init);
        printf(".Lbegin%d:\n", seq);
        if (node->cond)
        {
            gen(node->cond);
            printf("  pop rax\n");
            printf("  cmp rax, 0\n");
            printf("  je .Lend%d\n", seq);
        }
        gen(node->then);
        if (node->inc)
            gen(node->inc);
        printf("  jmp .Lbegin%d\n", seq);
        printf(".Lend%d:\n", seq);
    case ND_NUM:
        printf("  push %d\n", node->val);
        return;
    case ND_VAR:
        gen_addr(node);
        printf("  pop rax\n");
        printf("  mov rax, [rax]\n");
        printf("  push rax\n");
        return;
    case ND_ASSIGN:
        gen_addr(node->lhs);
        gen(node->rhs);

        printf("  pop rdi\n");
        printf("  pop rax\n");
        printf("  mov [rax], rdi\n");
        printf("  push rdi\n");
        return;
    case ND_ADDR:
        gen_addr(node->lhs);
        return;
    case ND_DEREF:
        gen(node->lhs);
        printf("  pop rax\n");
        printf("  mov rax, [rax]\n");
        printf("  push rax\n");
        return;
    }

    gen(node->lhs);
    gen(node->rhs);

    printf("  pop rdi\n");
    printf("  pop rax\n");

    switch (node->kind)
    {
    case ND_ADD:
        printf("  add rax, rdi\n");
        break;
    case ND_SUB:
        printf("  sub rax, rdi\n");
        break;
    case ND_MUL:
        printf("  imul rax, rdi\n");
        break;
    case ND_DIV:
        printf("  cqo\n");
        printf("  idiv rdi\n");
        break;
    case ND_EQ:
        printf("  cmp rax, rdi\n");
        printf("  sete al\n");
        printf("  movzb rax,al\n");
        break;
    case ND_NE:
        printf("  cmp rax, rdi\n");
        printf("  setne al\n");
        printf("  movzb rax,al\n");
        break;
    case ND_LE:
        printf("  cmp rax, rdi\n");
        printf("  setle al\n");
        printf("  movzb rax,al\n");
        break;
    case ND_LT:
        printf("  cmp rax, rdi\n");
        printf("  setl al\n");
        printf("  movzb rax,al\n");
        break;
    }

    printf("  push rax\n");
}

void codegen(Function *prog)
{
    printf(".intel_syntax noprefix\n");

    for (Function *fn = prog; fn; fn = fn->next)
    {
        printf(".global %s\n", fn->name);
        printf("%s:\n", fn->name);
        funcname = fn->name;

        // プロローグ
        printf("  push rbp\n");
        printf("  mov rbp, rsp\n");
        if (fn->stack_size)
            printf("  sub rsp, %d\n", fn->stack_size);

        // 定義された関数の引数をスタックに積んでおく
        int i = 0;
        for (VarList *vl = fn->params; vl; vl = vl->next)
        {
            Var *var = vl->var;
            printf("  mov [rbp-%d], %s\n", var->offset, argreg[i++]);
        }

        for (Node *node = fn->node; node; node = node->next)
            gen(node);

        // エピローグ
        // 最後の式の結果がRAXに残っているのでそれが返り値になる
        printf(".Lreturn.%s:\n", funcname);
        printf("  mov rsp, rbp\n");
        printf("  pop rbp\n");
        printf("  ret\n");
    }
}