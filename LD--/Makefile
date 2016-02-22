#
# TaC linker Source Code
#    Tokuyama kousen Educational Computer 16 bit Version
#
# Copyright (C) 2008-2010 by
#                      Dept. of Computer Science and Electronic Engineering,
#                      Tokuyama College of Technology, JAPAN
#
#   上記著作権者は，Free Software Foundation によって公開されている GNU 一般公
# 衆利用許諾契約書バージョン２に記述されている条件を満たす場合に限り，本ソース
# コード(本ソースコードを改変したものを含む．以下同様)を使用・複製・改変・再配
# 布することを無償で許諾する．
#
#   本ソースコードは＊全くの無保証＊で提供されるものである。上記著作権者および
# 関連機関・個人は本ソースコードに関して，その適用可能性も含めて，いかなる保証
# も行わない．また，本ソースコードの利用により直接的または間接的に生じたいかな
# る損害に関しても，その責任を負わない．
#
#
# Makefile : LD-- の makefile
#
# 2012.08.01 v2.0    : TaC-CPU V2 に対応
# 2010.12.07 v0.0.4  : バージョン情報等とともにコンパイルするように変更
# 2010.08.02         : テスト用のサンプルを Test ディレクトリに移動
# 2010.07.20         : Subversion の管理に入る
# 2010.04.09 v0.0.3  : install のオプションから -g wheel を削除(Ubuntu対応)
# 2008.07            : 初期バージョン
#
# $Id$
#

CC=cc -std=c99
VER=\"2.0.0\"
DATE="\"`date`\""

all: ld--

ld-- : ld.c
	${CC} ${CFLAGS} -DDATE=${DATE} -DVER=${VER} -DARC=\"TaC\" -o ld-- ld.c

install: ld--
	install -o root -m 755 ld-- /usr/local/bin

clean:
	rm -f *~ *.o *.lst ld-- a.s b.s c.s a.out *.pdf
