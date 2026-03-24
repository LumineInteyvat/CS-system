#include "nemu.h"
#include <sys/types.h>
#include <regex.h>
#include <string.h>
#include <stdlib.h>

enum
{
  TK_NOTYPE = 256,
  TK_DEC,
  TK_HEX,
  TK_REG,

  TK_EQ,
  TK_NEQ,
  TK_LT,
  TK_LE,
  TK_GT,
  TK_GE,
  TK_AND,
  TK_OR,

  TK_NEG,
  TK_DEREF
};

static struct rule
{
  char *regex;
  int token_type;
} rules[] = {
    {" +", TK_NOTYPE},
    {"0[xX][0-9a-fA-F]+", TK_HEX},
    {"[0-9]+", TK_DEC},
    {"\\$[a-zA-Z][a-zA-Z0-9]*", TK_REG},

    {"==", TK_EQ},
    {"!=", TK_NEQ},
    {"<=", TK_LE},
    {">=", TK_GE},
    {"<", TK_LT},
    {">", TK_GT},
    {"&&", TK_AND},
    {"\\|\\|", TK_OR},

    {"\\+", '+'},
    {"-", '-'},
    {"\\*", '*'},
    {"/", '/'},
    {"\\(", '('},
    {"\\)", ')'}};

#define NR_REGEX (sizeof(rules) / sizeof(rules[0]))

static regex_t re[NR_REGEX];

void init_regex()
{
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i++)
  {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0)
    {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token
{
  int type;
  char str[32];
} Token;

static Token tokens[64];
static int nr_token;

static bool is_binary_op(int type)
{
  return type == '+' || type == '-' || type == '*' || type == '/' ||
         type == TK_EQ || type == TK_NEQ ||
         type == TK_LT || type == TK_LE || type == TK_GT || type == TK_GE ||
         type == TK_AND || type == TK_OR;
}

static bool is_value_type(int type)
{
  return type == TK_DEC || type == TK_HEX || type == TK_REG || type == ')';
}

static bool make_token(char *e)
{
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  while (e[position] != '\0')
  {
    for (i = 0; i < NR_REGEX; i++)
    {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 &&
          pmatch.rm_so == 0)
      {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        position += substr_len;


        if (rules[i].token_type == TK_NOTYPE)
        {
          break;
        }

        Assert(nr_token < 64, "too many tokens");

        tokens[nr_token].type = rules[i].token_type;
        memset(tokens[nr_token].str, 0, sizeof(tokens[nr_token].str));

        if (substr_len >= (int)sizeof(tokens[nr_token].str))
        {
          Assert(0, "token too long");
        }

        switch (rules[i].token_type) {
  case TK_DEC:
  case TK_HEX:
  case TK_REG:
    strncpy(tokens[nr_token].str, substr_start, substr_len);
    tokens[nr_token].str[substr_len] = '\0';
    nr_token++;
    break;

  case '+':
  case '-':
  case '*':
  case '/':
  case '(':
  case ')':
  case TK_EQ:
  case TK_NEQ:
  case TK_LT:
  case TK_LE:
  case TK_GT:
  case TK_GE:
  case TK_AND:
  case TK_OR:
    nr_token++;
    break;

  default:
    assert(0);
        }

        break;
      }
    }

    if (i == NR_REGEX)
    {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

  return true;
}

static bool check_parentheses(int p, int q)
{
  if (tokens[p].type != '(' || tokens[q].type != ')')
  {
    return false;
  }

  int bal = 0;
  for (int i = p; i <= q; i++)
  {
    if (tokens[i].type == '(')
      bal++;
    else if (tokens[i].type == ')')
      bal--;

    if (bal < 0)
    {
      return false;
    }

    {
      return false;
    }
  }

  return bal == 0;
}

static bool check_expr_valid(int p, int q)
{
  int bal = 0;
  for (int i = p; i <= q; i++)
  {
    if (tokens[i].type == '(')
      bal++;
    else if (tokens[i].type == ')')
      bal--;
    if (bal < 0)
      return false;
  }
  return bal == 0;
}

static int precedence(int type)
{
  switch (type)
  {
  case TK_OR:
    return 1;
  case TK_AND:
    return 2;
  case TK_EQ:
  case TK_NEQ:
    return 3;
  case TK_LT:
  case TK_LE:
  case TK_GT:
  case TK_GE:
    return 4;
  case '+':
  case '-':
    return 5;
  case '*':
  case '/':
    return 6;
  case TK_NEG:
  case TK_DEREF:
    return 7;
  default:
    return 100;
  }
}

static int dominant_operator(int p, int q) {
  int op = -1;
  int min_pri = 100;
  int bal = 0;

  for (int i = p; i <= q; i++) {
    int type = tokens[i].type;

    if (type == '(') {
      bal++;
      continue;
    }
    if (type == ')') {
      bal--;
      continue;
    }
    if (bal != 0) continue;

    if (!is_binary_op(type)) {
      continue;
    }

    int pri = precedence(type);
    if (pri <= min_pri) {
      min_pri = pri;
      op = i;
    }
  }

  return op;
}

static uint32_t eval(int p, int q, bool *success)
{
  if (p > q)
  {
    *success = false;
    return 0;
  }

  if (p == q)
  {
    switch (tokens[p].type)
    {
    case TK_DEC:
      return strtoul(tokens[p].str, NULL, 10);

    case TK_HEX:
      return strtoul(tokens[p].str, NULL, 16);

    case TK_REG:
    {
      bool ok = true;
      uint32_t val = isa_reg_str2val(tokens[p].str + 1, &ok);
      *success = ok;
      return val;
    }

    default:
      *success = false;
      return 0;
    }
  }

  if (!check_expr_valid(p, q))
  {
    *success = false;
    return 0;
  }

  if (check_parentheses(p, q))
  {
    return eval(p + 1, q - 1, success);
  }

  if (tokens[p].type == TK_NEG) {
    uint32_t val = eval(p + 1, q, success);
    if (!*success) return 0;
    return -val;
    }

    if (tokens[p].type == TK_DEREF) {
        uint32_t addr = eval(p + 1, q, success);
    if (!*success) return 0;
        return vaddr_read(addr, 4);
    }

    int op = dominant_operator(p, q);
    if (op < 0) {
        *success = false;
        return 0;
    }

  uint32_t val1 = eval(p, op - 1, success);
  if (!*success)
    return 0;
  uint32_t val2 = eval(op + 1, q, success);
  if (!*success)
    return 0;

  switch (tokens[op].type)
  {
  case '+':
    return val1 + val2;
  case '-':
    return val1 - val2;
  case '*':
    return val1 * val2;
  case '/':
    if (val2 == 0)
    {
      *success = false;
      return 0;
    }
    return val1 / val2;

  case TK_EQ:
    return val1 == val2;
  case TK_NEQ:
    return val1 != val2;
  case TK_LT:
    return val1 < val2;
  case TK_LE:
    return val1 <= val2;
  case TK_GT:
    return val1 > val2;
  case TK_GE:
    return val1 >= val2;

  case TK_AND:
    return val1 && val2;
  case TK_OR:
    return val1 || val2;

  default:
    *success = false;
    return 0;
  }
}

uint32_t expr(char *e, bool *success)
{
  if (!make_token(e))
  {
    *success = false;
    return 0;
  }

  for (int i = 0; i < nr_token; i++)
  {
    if (tokens[i].type == '-')
    {
      if (i == 0 || (!is_value_type(tokens[i - 1].type)))
      {
        tokens[i].type = TK_NEG;
      }
    }
    else if (tokens[i].type == '*')
    {
      if (i == 0 || (!is_value_type(tokens[i - 1].type)))
      {
        tokens[i].type = TK_DEREF;
      }
    }
  }

  if (nr_token == 0)
  {
    *success = false;
    return 0;
  }

  return eval(0, nr_token - 1, success);
}
