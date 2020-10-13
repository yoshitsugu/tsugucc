#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//
// tokenize.c
//

typedef enum
{
    TK_RESERVED, // 記号
    TK_IDENT,    // 識別子
    TK_NUM,      // 整数トークン
    TK_EOF,      // 入力の終わりを表すトークン
} TokenKind;

typedef struct Token Token;

struct Token
{
    TokenKind kind;
    Token *next;
    int val;
    char *str;
    int len;
};

typedef struct Var Var;
struct Var
{
    char *name; // 変数名
    int offset; // RBPからのオフセット
};

typedef struct VarList VarList;
struct VarList
{
    VarList *next;
    Var *var;
};

void error_at(char *loc, char *fmt, ...);
void error(char *fmt, ...);
bool consume(char *op);
Token *consume_ident();
bool expect(char *op);
char *expect_ident();
void expect_ident_str(char *str);
int expect_number();
char *strndup(char *str, int len);
bool at_eof();
Token *new_token(TokenKind kind, Token *cur, char *str, int len);
Token *tokenize();

extern char *user_input;
extern Token *token;

//
// parse.c
//

typedef enum
{
    ND_ADD,     // +
    ND_SUB,     // -
    ND_MUL,     // *
    ND_DIV,     // /
    ND_EQ,      // ==
    ND_NE,      // !=
    ND_LT,      // <
    ND_LE,      // <=
    ND_NUM,     // 整数
    ND_ASSIGN,  // =
    ND_VAR,     // ローカル変数
    ND_RETURN,  // return
    ND_IF,      // if
    ND_WHILE,   // while
    ND_FOR,     // for
    ND_BLOCK,   // 複文(ブロック)
    ND_FUNCALL, // 関数呼出
    ND_FUNCDEF, // 関数定義
    ND_ADDR,    // 単項演算子&
    ND_DEREF,   // 単項演算子*
} NodeKind;

typedef struct Node Node;

// 抽象構文木のノードの型
struct Node
{
    NodeKind kind; // ノードの型
    Node *next;    // Next node

    Node *lhs; // 左辺
    Node *rhs; // 右辺

    // "if", "while" or "for" statement
    Node *cond;
    Node *then;
    Node *els;
    Node *init;
    Node *inc;

    // Block
    Node *body;

    // Function
    char *funcname;
    Node *args;

    Var *var; // kindがND_LVARの場合のみ使う
    int val;  // kindがND_NUMの場合のみ使う
};

typedef struct Function Function;
struct Function
{
    Function *next;
    char *name;
    Node *node;
    int stack_size;
    VarList *params; // 引数
    VarList *locals; // ローカル変数
};

Function *program();
Var *push_var(char *name);

//
// codegen.c
//

void codegen(Function *code);