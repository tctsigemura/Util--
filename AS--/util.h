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
 * util.h : AS-- 良く使う関数の外部インタフェース
 *
 * 2012.07.21           : null を追加
 * 2010.07.20           : Subversion による管理を開始
 *
 * $Id$
 *
 */

#define boolean int      /* boolean 型がC言語でも使えるとよいのに ...  */
#define true     1
#define false    0
#define null     NULL

void error(char*s);      /* エラーメッセージを表示後終了 */
void *ealloc(int s);     /* エラーチェックつきの malloc  */
int isDelim(int ch);     /* 区切り記号かテスト           */
int isOdigit(int ch);    /* 8進数字かテスト              */
