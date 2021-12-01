all: xwiibb

xwiibb: xwiibb.o
	g++ xwiibb.o -o xwiibb -lxwiimote -lX11 -lXtst

xwiibb.o:
	g++ -c xwiibb.cpp -lxwiimote -lX11 -lXtst

clean:
	rm -rf *.o xwiibb

rebuild: clean all
