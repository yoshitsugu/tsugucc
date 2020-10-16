#include "tsugucc.h"

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        error("引数の個数が正しくありません");
        return 1;
    }
    user_input = argv[1];
    token = tokenize();
    Function *prog = program();

    for (Function *fn = prog; fn; fn = fn->next)
    {
        int offset = 0;
        for (VarList *vl = fn->locals; vl; vl = vl->next)
        {
            offset += offset_size(vl->var->ty);
            vl->var->offset = offset;
        }
        fn->stack_size = offset;
    }

    codegen(prog);
    return 0;
}