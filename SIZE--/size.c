/*
 * TaC size utility Source Code
 *    Tokuyama kousen Educational Computer 16bit Ver.
 *
 * Copyright (C) 2009-2016 by
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
 * size.c : SIZE-- 本体
 * リロケータブル形式から text, data, bss サイズを調べて表示する
 *
 * 2012.09.12           : ファイルがオープンできない時のエラー処理追加
 * 2012.08.06  v.2.0.0  : TaC-CPU V2 対応
 * 2010.07.20           : Subversion による管理を開始
 * 2009.04.20           : 作成 by T.Shigemura
 *
 * $Id$
 *
 */

#include <stdio.h>
#include <stdlib.h>

int magic;                                  /* マジックナンバー             */
int textSize;                               /* TEXT セグメントサイズ        */
int dataSize;                               /* DATA セグメントサイズ        */
int bssSize;                                /* BSS セグメントサイズ         */
int symSize;                                /* 名前表サイズ                 */
int trSize;                                 /* TEXT 再配置情報サイズ        */
int drSize;                                 /* DATA 再配置情報サイズ        */

FILE *in;                                   /* 入力ファイル                 */

int getW() {                                /* 1ワード入力ルーチン          */
  int x1 = getc(in);
  int x2 = getc(in);
  if (x1==EOF || x2==EOF) {
    fprintf(stderr, "bad file format\n");
    exit(1);
  }
  return (x1 << 8) | x2;
}

void readHdr() {
  magic=getW();                            /* マジックナンバー              */
  textSize=getW();                         /* TEXTサイズ                    */
  dataSize=getW();                         /* DATAサイズ                    */
  bssSize=getW();                          /* BSS サイズ                    */
  symSize=getW();                          /* SYMSサイズ                    */
  getW();                                  /* ENTRY                         */
  trSize=getW();                           /* Trサイズ                      */
  drSize=getW();                           /* Drサイズ                      */
}

int main(int argc, char*argv[]) {
  if (argc!=2) {
    fprintf(stderr, "使用方法 : %s <file name>\n", argv[0]);
    fprintf(stderr, "<file name> : TaC リロケータブル形式のファイル\n");
    exit(1);
  }

  if ((in = fopen(argv[1], "rb"))==NULL) {
    perror(argv[1]);
    exit(1);
  }

  readHdr();

  if (magic!=0x0107) {
    fprintf(stderr, "マジックナンバーが 0x0107 以外\n");
    exit(1);
  }

  printf("text\tdata\tbss\tdec\tfilename\n");
  printf("%5d\t%5d\t%5d\t%5d\t%s\n",
	 textSize,dataSize,bssSize,
	 textSize+dataSize+bssSize,
	 argv[1]);
  printf("(%04x)\t(%04x)\t(%04x)\t(%04x)\t(hex)\n",
	 textSize,dataSize,bssSize,
	 textSize+dataSize+bssSize
	 );
  return 0;
}
