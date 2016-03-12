#
# TaC Assembler Source Code
#    Tokuyama kousen Educational Computer 16 bit Version
#
# Copyright (C) 2008-2016 by
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
# Makefile : AS-- の Makefile
#
# 2016.02.05  v2.1.1 : BSS の２重定義に関するバグ訂正
# 2012.09.29  v2.1.0 : OP Rd,0,Rx を OP Rd,%Rx と解釈するなどの拡張
# 2012.07.31  v2.0.0 : TaC-CPU V2 に対応
# 2012.01.12  v0.7.0 : VER, DATE の追加
# 2010.07.20         : Subversion の管理に入る
# 2010.04.09  v0.6.3 : install のオプションから -g wheel を削除(Ubuntu対応)
# 2008.07            : 初期バージョン
#
# $Id$
#

CC=cc -std=c99
VER=\"2.1.1\"
DATE="\"`date`\""

as-- : syntax.c lexical.c lexical.h util.h util.c
	${CC} ${CFLAGS} -DDATE=${DATE} -DVER=${VER} \
	-o as-- syntax.c lexical.c util.c

install : as--
	install -d -m 755 /usr/local/bin
	install -m 755 as-- /usr/local/bin

clean :
	rm -f *.o as-- *.c~ *.h~ *.txt~ Makefile~ *.s~ *.lst *.pdf
