%{
#define BISON_Y_FILE  
#include <cstdlib>
#include "commands/loadCommand.h"
#include "commands/saveCommand.h"
#include "commands/replaceCommand.h"
#include "commands/shellCommand.h"
#include "commands/dwarfscriptCommand.h"
#include "commands/infoCommand.h"
#include "commands/hashCommand.h"
#include "commands/patchCommand.h"
#include "arrayAccessParam.h"  
#include "parse_helper.h"
  


ParseNode rootParseNode;
  
#define YYSTYPE ParseNode
#define YYDEBUG 1

//so we don't get any warnings in the code generated by bison, which
//wasn't originally meant to be compiled as C++
#pragma GCC diagnostic ignored "-Wwrite-strings"

 
extern int yylex();
extern "C"
{
#include "util/dictionary.h"
  int yyerror(char *s);

  extern Dictionary* shellVariables;  
  
  //saved values from lexing
  int lineNumber=1;
  char* savedVarName=NULL;
  char* savedString=NULL;
  int savedInt=0;
}


%}

%expect 0
//Token definitions
%token T_LOAD T_SAVE T_TRANSLATE T_REPLACE T_SECTION T_VARIABLE T_STRING_LITERAL
%token T_DATA T_DWARFSCRIPT T_COMPILE T_EMIT T_SHELL_COMMAND
%token T_HASH T_ELF
%token T_INFO T_EXCEPTION_HANDLING
%token T_PATCH T_GENERATE T_APPLY
%token T_NONNEG_INT T_RAW
%token T_INVALID_TOKEN
%token T_EOL T_EOF

%%

root : line_list
{
  $$=$1;
  rootParseNode=$1;
}

line_list : line_list line line_terminator
{
  CommandList* listItem=(CommandList*)zmalloc(sizeof(CommandList));
  listItem->cmd=$2.u.cmd;
  listItem->next=NULL;
  if(PNT_EMPTY==$1.type)
  {
    $$.type=PNT_LIST;
    listItem->tail=listItem;
    $$.u.listItem=listItem;
  }
  else
  {
    assert(PNT_LIST==$$.type);
    $1.u.listItem->tail->next=listItem;
    $1.u.listItem->tail=listItem;
    $$=$1;
  }
}
|
{
  $$.type=PNT_EMPTY;
}

line : assignment 
{
  $$=$1;
}
| commandline 
{
  $$=$1;
}

line_terminator : line_term_token {}
| line_terminator line_term_token {}

line_term_token : T_EOL {}
| T_EOF {}
| ';' {}

assignment : variable '=' commandline
{
  $3.u.cmd->setOutputVariable($1.u.var);
  $$=$3;
}

commandline : loadcmd {$$=$1;$$.type=PNT_CMD;}
| savecmd {$$=$1;$$.type=PNT_CMD;}
| dwarfscriptcmd {$$=$1;$$.type=PNT_CMD;}
| replacecmd {$$=$1;$$.type=PNT_CMD;}
| shellcmd {$$=$1;$$.type=PNT_CMD;}
| infocmd {$$=$1;$$.type=PNT_CMD;}
| hashcmd {$$=$1;$$.type=PNT_CMD;}
| patchcmd {$$=$1;$$.type=PNT_CMD;}


loadcmd : T_LOAD param
{
  $$.u.cmd=new LoadCommand($2.u.param);
  $2.u.param->drop();
}

savecmd : T_SAVE param param
{
  $$.u.cmd=new SaveCommand($2.u.param,$3.u.param);
  $2.u.param->drop();
  $3.u.param->drop();
}
| T_SAVE error
{
  fprintf(stderr,"save takes two parameters\n");
  YYERROR;
}

dwarfscriptcmd : T_DWARFSCRIPT T_COMPILE param 
{
  $$.u.cmd=new DwarfscriptCommand(DWOP_COMPILE,$3.u.param,NULL);
}
| T_DWARFSCRIPT T_COMPILE param param
{
  $$.u.cmd=new DwarfscriptCommand(DWOP_COMPILE,$3.u.param,$4.u.param);
}
| T_DWARFSCRIPT T_EMIT param param param
{
  $$.u.cmd=new DwarfscriptCommand(DWOP_EMIT,$3.u.param,$4.u.param,$5.u.param);
}
| T_DWARFSCRIPT T_EMIT param param
{
  ShellParam* section=new ShellParam(".eh_frame");
  $$.u.cmd=new DwarfscriptCommand(DWOP_EMIT,section,$3.u.param,$4.u.param);
  section->drop();
}
| T_DWARFSCRIPT T_EMIT error
{
  fprintf(stderr,"Usage: dwarfscript emit (\".eh_frame\"|\".debug_frame\") ELF [OUTFILE]\n");
  YYERROR;
}
| T_DWARFSCRIPT T_COMPILE error
{
  fprintf(stderr,"Usage: dwarfscript compile  DWARFSCIRPT_FILENAME [DEBUG_DATA_OUT]\n");
  YYERROR;
}
| T_DWARFSCRIPT error
{
  fprintf(stderr,"Valid dwarfscript operations are 'compile' and 'emit'\n");
  YYERROR;
}

/* translatecmd : T_TRANSLATE translate_fmt translate_fmt param */
/* { */
/*   $$.u.cmd=new TranslateCommand($2.intval,$3.intval,$4.u.param); */
/* } */
/* //the form that take the output file as a parameter */
/* | T_TRANSLATE translate_ftmt paratranslate_fmt param param */
/* { */
/*   $$.u.cmd=new TranslateCommand($2.intval,$3.intval,$4.u.param,$5.u.param); */
/* } */

/* translate_fmt : T_DATA {$$.u.intval=TFT_DATA} */
/* | T_DWARFSCRIPT {$$.u.intval=TFT_DWARFSCRIPT} */

replacecmd : T_REPLACE T_SECTION param param param
{
  $$.u.cmd=new ReplaceCommand(RT_SECTION,$3.u.param,$4.u.param,$5.u.param);
  $3.u.param->drop();
  $4.u.param->drop();
  $5.u.param->drop();
}
| T_REPLACE T_RAW param param param
{
  $$.u.cmd=new ReplaceCommand(RT_RAW,$3.u.param,$4.u.param,$5.u.param);
  $3.u.param->drop();
  $4.u.param->drop();
  $5.u.param->drop();
}

shellcmd : T_SHELL_COMMAND param
{
  $$.u.cmd=new SystemShellCommand($2.u.param);
  $2.u.param->drop();

}

infocmd : T_INFO T_EXCEPTION_HANDLING param param
{
  $$.u.cmd=new InfoCommand(IOP_EXCEPTION,$3.u.param,$4.u.param);
  $3.u.param->drop();
  $4.u.param->drop();
}
| T_INFO T_EXCEPTION_HANDLING param
{
  $$.u.cmd=new InfoCommand(IOP_EXCEPTION,$3.u.param);
  $3.u.param->drop();
}
| T_INFO T_PATCH param
{
  $$.u.cmd=new InfoCommand(IOP_PATCH,$3.u.param);
  $3.u.param->drop();
}
| T_INFO error
{
  fprintf(stderr,"Unrecognized info usage\n");
  YYERROR;
}

hashcmd : T_HASH T_ELF param
{
  $$.u.cmd=new HashCommand(HT_ELF,$3.u.param);
  $3.u.param->drop();
}
| T_HASH error
{
  fprintf(stderr,"Usage: hash TYPE STRING_TO_HASH\n");
  fprintf(stderr,"The only hash type currently supported is 'elf'\n");
  YYERROR;
}

patchcmd : T_PATCH T_GENERATE param param param
{
  $$.u.cmd=new PatchCommand(PO_GENERATE_PATCH,$3.u.param,$4.u.param,$5.u.param);
  $3.u.param->drop();
  $4.u.param->drop();
  $5.u.param->drop();
}
| T_PATCH T_APPLY param param
{
  $$.u.cmd=new PatchCommand(PO_APPLY_PATCH,$3.u.param,$4.u.param);
  $3.u.param->drop();
  $4.u.param->drop();
}

param : variable
{
  $$.u.param=$1.u.var;
  $$.u.param->grab();
  $$.type=PNT_PARAM;
    
}
| stringParam
{
  $$.u.param=new ShellParam($1.u.string);
  $$.type=PNT_PARAM;
}
| variable '[' nonneg_int_lit ']'
{
  $$.u.param=new ShellArrayAccessParam($1.u.var,$3.u.intval);
  $$.type=PNT_PARAM;
}
| nonneg_int_lit
{
  $$.u.param=new ShellIntParam($1.u.intval);
  $$.type=PNT_PARAM;
}

variable : T_VARIABLE
{
  $$.type=PNT_VAR;
  $$.u.var=(ShellVariable*)dictGet(shellVariables,savedVarName);
  if(!$$.u.var)
  {
    $$.u.var=new ShellVariable(savedVarName);
    dictInsert(shellVariables,savedVarName,$$.u.var);
  }
}

stringParam : T_STRING_LITERAL
{
  $$.u.string=savedString;
  $$.type=PNT_STR;
}

nonneg_int_lit : T_NONNEG_INT
{
  $$.u.intval=savedInt;
}

%%
int yyerror(char *s)
{
  fprintf(stderr, "%s at line %d\n", s, lineNumber);
  return 0;
}

