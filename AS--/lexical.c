/*
 * TaC Assembler Source Code
 *    Tokuyama kousen Educational Computer 16bit Ver.
 *
 * Copyright (C) 2008-2021 by
 *                      Dept. of Computer Science and Electronic Engineering,
 *                      Tokuyama College of Technology, JAPAN
 *
 *   上記著作権者は，Free Software Foundation によって公開されている GNU 一般公
 * 衆利用許諾契約書バージョン２に記述されている条件を満たす場合に限り，本ソース
 * コード(本ソースコードを改変したものを含む．以下同様)を使用・複製・改変・再配
 * 布することを無償で許諾する．
 *
 *   本ソースコードは＊全くの無保証＊で提供されるものである。上記著作権者および
 * 関連機関・個人は本ソースコードに関して，その適用可能性も含めて，いかなる保証
 * も行わない．また，本ソースコードの利用により直接的または間接的に生じたいかな
 * る損害に関しても，その責任を負わない．
 *
 *
 */

/*
 * lexical.c : AS--の字句解析ルーチン
 *
 * 2021.10.14  v4.0.0   : TaC-CPU V3 に対応
 * 2012.09.27           : @,%の意味が逆になっていた。バグ修正
 * 2012.09.19           : FLAG を追加
 * 2012.08.01  v2.0     : TaC-CPU V2 に対応
 * 2012.01.17  v0.7     : Halt 命令を追加
 * 2010.07.20           : Subversion による管理を開始
 * 2009.03.11  v0.6.3   : #include <strings.h> を追加(Ubuntu対応)
 * 2009.03.11  v0.6     : コート中の ';' の処理のバグ修正
 *                        エスケープ文字の処理のバグ修正
 *                        数値が 16bit に収まらない場合のバグ修正
 * 2008.08.31  v0.5     : 実用バージョン
 * 2008.07              : 初期バージョン
 *
 * $Id$
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <strings.h>

#include "util.h"
#include "lexical.h"

/* 予約語の一覧表 */
struct Rsv_Word {
  char  *name;                                     /* つづりと               */
  int    tok;                                      /* トークン値の組         */
};

struct Rsv_Word rsv_word[] = {
  {"NO"  , NO  }, {"LD"  , LD  }, {"ST"  , ST  }, {"ADD" , ADD },
  {"SUB" , SUB }, {"CMP" , CMP }, {"AND" , AND }, {"OR"  , OR  },
  {"XOR" , XOR }, {"ADDS", ADDS}, {"MUL" , MUL }, {"DIV" , DIV }, 
  {"MOD" , MOD },
  {"SHLA", SHLA}, {"SHLL", SHLL}, {"SHRA", SHRA}, {"SHRL", SHRL},
  {"JZ"  , JZ  }, {"JC"  , JC  }, {"JM"  , JM  }, {"JO"  , JO  },
  {"JGT" , JGT }, {"JGE" , JGE }, {"JLE" , JLE }, {"JLT" , JLT },
  {"JNZ" , JNZ }, {"JNC" , JNC }, {"JNM" , JNM }, {"JNO" , JNO },
  {"JHI" , JHI }, {"JLS" , JLS }, {"JMP" , JMP },
  {"CALL", CALL}, {"IN"  , IN  }, {"OUT" , OUT }, {"PUSH", PUSH},
  {"POP" , POP }, {"RET" , RET }, {"RETI", RETI},
  {"SVC" , SVC }, {"HALT", HALT},
  {"DB"  , DB  }, {"DW"  , DW  }, {"BS"  , BS  }, {"WS"  , WS  },
  {"EQU" , EQU }, {"STRING",STR},
  {"G0"  , G0  }, {"G1"  , G1  }, {"G2"  , G2  }, {"G3"  , G3  },
  {"G4"  , G4  }, {"G5"  , G5  }, {"G6"  , G6  }, {"G7"  , G7  },
  {"G8"  , G8  }, {"G9"  , G9  }, {"G10" , G10 }, {"G11" , G11 },
  {"G12" , G12 }, {"FP"  , FP  }, {"SP"  , SP  }, {"USP" , USP },
  {"FLAG", FLAG}, {null  , 0   }
};

int  nextch = 0;                                   /* 次の文字               */
int  ch     = 0;                                   /* 現在の文字             */

int  l_cnt;

/* 一文字を読み込む */
void getNext() {
  ch = nextch;                                     /* 次の文字を現在の文字に */
  nextch = fgetc(in);                              /* 次の文字を読み込む     */
  if (l_cnt<MAX_LINE && ch!='\n' && ch!=EOF)
    line[l_cnt++]=ch;
  else
    line[l_cnt]='\0';
}

/* 行末まで読み飛ばす */
void skipLine(){
  while(ch!='\n' && ch!=EOF) {                     /* 改行以外の間           */
    getNext();                                     /* 1文字読み飛ばす        */
  }
}

/* 空白文字を読み飛ばす */
void skipSpace(){
  while(ch!='\n' && isspace(ch)) {                 /* 空白文字の間           */
    getNext();                                     /* 1文字読み飛ばす        */
  }
  if (ch==';') skipLine();                         /* コメントを読み飛ばす   */
}

/* 字句解析用のエラー表示ルーチン */
void lerror(char *s){
  skipLine();
  error(s);
}

/* 16進数字の値を返す */
int hex(int ch) {
  if ('0'<=ch && ch<='9') return ch - '0';         /* '0'〜'9'の場合         */
  ch = toupper(ch);                                /* 英字は大文字に統一     */
  return ch - 'A' + 10;                            /* 'A'〜'F'の場合         */
}

/* 16進数を読んで値を返す */
int getHex() {
  if (!isxdigit(ch)) {                             /* 0x の次に16進数がない  */
    lerror("16進数の形式");
  }
  int val = 0;                                     /* 初期値は 0             */
  while (isxdigit(ch)) {                           /* 16進数字の間           */
    val = val*16 + hex(ch);                        /* 値を計算               */
    getNext();                                     /* 次の文字を読む         */
  }
  return val;                                      /* 16進数の値を返す       */
}

/* 8進数を読んで値を返す */
int getOct() {
  int val = 0;                                     /* 初期値は 0             */
  while (isOdigit(ch)) {                           /* 8進数字の間            */
    val = val*8 + ch - '0';                        /* 値を計算               */
    getNext();                                     /* 次の文字を読む         */
  }
  return val;                                      /* 8進数の値を返す        */
}

/* 10進数を読んで値を返す */
int getDec() {
  int val = 0;                                     /* 初期値は 0             */
  while (isdigit(ch)) {                            /* 10進数字の間           */
    val = val*10 + ch - '0';                       /* 値を計算               */
    getNext();                                     /* 次の文字を読む         */
  }
  return val;                                      /* 10進数の値を返す       */
}

/* 数値を読み込む */
int getDigit() {
  boolean neg = false;
  val = 0;
  if (ch=='-') {                                   /* 負の数の場合          */
    neg = true;
    getNext();
    if (!isdigit(ch)) lerror("'-'の次に数値以外");
  }
  if(ch=='0' && (nextch=='x' || nextch=='X')){     /* '0x' で始まれば16進数  */
    getNext();                                     /* '0'を読み飛ばす        */
    getNext();                                     /* 'x'を読み飛ばす        */
    val = getHex();                                /* 16進数の読み込み       */
  }else if(ch=='0') {                              /* '0' で始まれば8進数    */
    val = getOct();                                /* 8進数を読み込む        */
  }else{                                           /* それ以外は10進のはず   */
    val = getDec();                                /* 10進数を読み込む       */
  }
  if (neg) val = -val;
  return INTEGER;                                  /* tok=INTEGER, val=値    */
}

/* 名前か予約語を読み込む */
int getWord(){
  int i;
  for (i=0; !isDelim(ch) && i<MAX_NAME; i=i+1) {   /* 区切り文字以外なら     */
    str[i] = ch;                                   /* strに読み込む          */
    getNext();                                     /* 1文字読み飛ばす        */
  }
  if (!isDelim(ch)) {                              /* 区切り文字以外なのに終 */
    lerror("名前が長すぎる");                      /* 了していれば名前の長さ */
  }                                                /* が長すぎる場合         */
  str[i] = '\0';                                   /* 文字列を完成させる     */
  for (i=0; rsv_word[i].tok!=0; i=i+1) {           /* 予約語の一覧表を捜す   */
    if (!strcasecmp(str, rsv_word[i].name))        /* 同じつづりが見付かれば */
      return rsv_word[i].tok;                      /* それのトークン値を返す */
  }                                                /* 予約語以外なら         */
  return NAME;                                     /* tok=NAME, str=値       */
}

/* エスケープ文字の読み込み */
int getEsc() {
  int n  = 0;                                      /* 文字コードの初期値は 0 */
  getNext();                                       /* '\' を読み飛ばす       */
  if (ch == 'n'){                                  /* '\n' の場合            */
    n = '\n';                                      /* n に LF を格納         */
    getNext();                                     /* 'n' を読み飛ばす       */
  } else if (ch == 't'){                           /* '\t' の場合            */
    n = '\t';                                      /* n に TAB を格納        */
    getNext();                                     /* 't' を読み飛ばす       */
  } else if (ch == 'r'){                           /* '\r' の場合            */
    n = '\r';                                      /* n に CR を格納         */
    getNext();                                     /* 'r' を読み飛ばす       */
  } else if (ch == 'x' || ch == 'X') {             /* '\x' の場合            */
    getNext();                                     /* 'x' を読み飛ばす       */
    n = getHex();                                  /* n に16進数を読み込む   */
  } else if (isOdigit(ch)) {                       /* '\数値' の場合         */
    n = getOct();                                  /* n に8進数を読み込む    */
  } else if (isprint(ch)) {                        /* そのほか印刷可能文字の */
    n = ch;                                        /* 場合'\c'は'c'と同じ    */
    getNext();                                     /* 'c' を読み飛ばす       */
  } else lerror("未定義のエスケープ文字");         /* それ以外はエラー       */
  return n;                                        /* 文字コードを返す       */
}

/* 文字列 "..." を読み込み  PASCAL 型文字列にする */
int getStr(){
  int i;
  getNext();
  for (i=1;ch!='\"'&&ch!=EOF&&i<MAX_NAME;i=i+1){   /* (")以外の間            */
    if (iscntrl(ch)) {                             /* ch が制御文字なら      */
      break;                                       /* 異常終了               */
    } else if (ch=='\\') {
      str[i] = getEsc();                           /* 文字コードを決めて     */
    } else {
      str[i] = ch;                                 /* str に格納する         */
      getNext();
    }
  }
  if (ch!='\"')                                    /* 前のforが(")以外で終了 */
    lerror("文字列が正しく終わっていないか、長すぎる");
  getNext();                                       /* (")を読み飛ばす        */
  str[i] = '\0';
  str[0] = i;                                      /* 文字列の長さ('\0'含む) */
  return STRING;                                   /* tok=STRING, str=値     */
}

/* 文字定数 'x' を読み込む */
int getChar(){
  getNext();                                       /* コート(')を読み飛ばす  */
  if(ch == '\\'){                                  /* コートの次が '\\' なら */
    val = getEsc();                                /* エスケープ文字         */
  }else{                                           /* そうでなければ ch が   */
    val = ch;                                      /* 定数値になる           */
    getNext();                                     /* ch を読み飛ばす        */
  }
  if(ch != '\'')                                   /* コート(')で文字定数が  */
    lerror("文字が正しく終わっていない");          /* 終了しているか         */
  getNext();                                       /* コート(')を読み飛ばす  */
  return INTEGER;                                  /* 文字定数は整数扱い     */
}

/* ソースの最初に戻る */
void rewindSrc() {
  rewind(in);
  nextch=fgetc(in);
}

/* 一行読み込む */
int getLine(){
  l_cnt = 0;
  getNext();                                       /* 次の行に進む           */
  if (ch==EOF) return EOF;

  op=opr[0]=opr[1]=opr[2]=0;                       /* クリア                 */
  lab=imd=ind=bind=false;

  if (isspace(ch) || ch==';') {                    /* ラベル欄が空白         */
  } else if (!isDelim(ch) && getWord()==NAME) {    /* ラベルが存在           */
    lab = true;
    strcpy(label, str);
  } else {                                         /* 空行以外はエラー       */
    lerror("不適切なラベル");
  }

  skipSpace();
  if (ch!='\n' && ch!=EOF) {
    if (isDelim(ch)) lerror("命令欄に記号が入力された");
    op = getWord();
    skipSpace();
    if (ch!='\n' && ch!=EOF) {                     /* オペランドが存在する   */
      int x = 0;
      if (JZ<=op && op<=CALL) {                    // JMP,CALL命令は特別
	opr[0] = G0;                               //  1オペランドに G0 が
	x = 1;                                     //  あったことにする
      }
      for (int i=x; i<3; i++) {
	if (i==1 && (ch=='#'||ch=='@'||ch=='%')) { // 2オペランドは#,@,%
	  imd  = ch == '#';                        // #:イミディエイト
	  ind  = ch == '%';                        // %:インダイレクト
	  bind = ch == '@';                        // @:バイトインダイレクト
	  getNext();
	  skipSpace();
	}
	if (ch=='-' || isdigit(ch)) opr[i] = getDigit();
	else if (ch=='\"')          opr[i] = getStr();
	else if (ch=='\'')          opr[i] = getChar();
	else if (!isDelim(ch))      opr[i] = getWord();
	else lerror("','の次が不正");

	if (opr[i]==NAME) strcpy(oprlab, str);     /* ラベルの綴りを記憶 */
	skipSpace();
	if (ch!=',') break;
	getNext();
	skipSpace();
      }
    }
    if (ch!='\n' && ch!=EOF) lerror("オペランドの記述が不正");
  }
  return 0;
}

/*
int main() {
  in = stdin;
  rewindSrc();
  ln = 1;
  while (getLine()!=EOF) {
    char *a = "";
    char *b = "";
    char *c = "";
    if (imd) a = "#";
    if (ind) b = "@";
    if (bind) c = "%";
    if (lab)
      printf("%s\t%04x\t%04x,%s%s%s%04x,%04x\n",
	     label,op,opr[0],a,b,c,opr[1],opr[2]);
    else
      printf("\t%04x\t%04x,%s%s%s%04x,%04x\n",op,opr[0],a,b,c,opr[1],opr[2]);
    ln++;
  }
  return 0;
}
*/
