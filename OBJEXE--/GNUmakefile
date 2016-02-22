#Makefile : OBJEXE用のmakefile

#2015.05.13　コンパイルやアセンブルに対する記述を削除し、OBJEXEを実行するだけに変更
#2014.12.11　OBJBIN用のmakefileをコピー、OBJEXE用に修正
#2015.02.19　OBJEXEの実行環境を追加

#---------------------------------------------------------------
# OBJEXEの実行環境

# プリプロセッサ
CPP=cc -E -std=c99 -nostdinc ${CPFLAGS}


#---------------------------------------------------------------
# OBJEXEのインストール
CC=cc -std=c99

# objexe.cをコンパイルしてobjexe--というプログラムを生成
objexe-- : objexe.c
	${CC} ${CFLAGS} -o objexe-- objexe.c

install : objexe--
	install -o root -g wheel -m 755 objexe-- /usr/local/bin

clean	:
	rm -f objexe-- *.o *~

