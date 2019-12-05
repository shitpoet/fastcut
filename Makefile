#compile:;tcc  -O0 -w s2.c -o scrscr `pkg-config --cflags --libs gtk+-2.0`
#compile:;gcc -std=c99  -O0 -w fastcut.c -o fastcut `pkg-config --cflags --libs gtk+-2.0`
compile:;tcc -std=c99  -O0 -Wimplicit-function-declaration fastcut.c -o fastcut `pkg-config --cflags --libs gtk+-2.0`

#run:;rm scrscr;tcc  -O0 -w main.c -o scrscr `pkg-config --cflags --libs gtk+-2.0`;./scrscr &




