#include "tsugucc.h"

LVar *locals;

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        error("引数の個数が正しくありません");
        return 1;
    }
    locals = NULL;

    // トークナイズしてパースする
    user_input = argv[1];
    token = tokenize();
    Node *code[100];
    program(code);

    codegen(code);
    return 0;
}