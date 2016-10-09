#
# TaC size utility Source Code
#    Tokuyama kousen Educational Computer 16 bit Version
#
# Copyright (C) 2009-2016 by
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
# GNUmakefile : SIZE-- の makefile
#
# 2016.10.09  v3.0.0 : バージョン番号をUtil--で共用にする
# 2010.07.20         : Subversion の管理に入る
# 2010.04.09 v0.0.0  : install のオプションから -g wheel を削除(Ubuntu対応)
#
# $Id$
#

CC=cc -std=c99
all: size--
VER=\"`cat ../VERSION`\"
DATE="\"`date`\""

size-- : size.c GNUmakefile
	${CC} ${CFLAGS} -DDATE=${DATE} -DVER=${VER} -DARC=\"TaC\" \
	-o size-- size.c

install : size--
	install -d -m 755 /usr/local/bin
	install -m 755 size-- /usr/local/bin

clean :
	rm -f size-- *~ *.o
