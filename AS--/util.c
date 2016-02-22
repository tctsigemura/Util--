/*
 * TaC Assembler Source Code
 *    Tokuyama kousen Educational Computer 16bit Ver.
 *
 * Copyright (C) 2008-2010 by
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
 * util.c : AS-- 良く使う関数
 *
 * 2010.07.20           : Subversion による管理を開始
 * 2010.03.14           : エラー表示の見直し
 * 2009.04.08           : error() 中 fprintf の引数が多過ぎ
 * 2008.06.21           : 初期バージョン
 *
 * $Id$
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "util.h"
#include "lexical.h"

/* エラーメッセージを表示して終了する */
void error(char* s) {
  if (ln>0) fprintf(stderr,"%d: \"%s\" : ", ln, line);
  fprintf(stderr,"%s\n", s);
  exit(1);
}

/* 領域を割り当ててエラーチェック */
void *ealloc(int s) {
  void *p = malloc(s);
  if (p==NULL) {
    error("メモリ不足");
  }
  return p;
}

/* 以下は ctype.h にありそうで存在しない関数 */

/* 区切り記号かテスト */
int isDelim(int ch) {
  return !isalnum(ch) && ch!='_' && ch!='.';        /* アルファベット,'_'以外 */
}

/* 8進数字かテスト */
int isOdigit(int ch) {
  return '0' <= ch && ch <= '7';                   /* '0'〜'7'が8進数        */
}
