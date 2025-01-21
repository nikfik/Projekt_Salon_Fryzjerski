all: fryzjerzy fotele klienci salon_fryzjerski
fryzjerzy.o: fryzjerzy.c semafor.o
	 gcc -Wall -g -c fryzjerzy.c -o fryzjerzy.o
fotele.o: fotele.c semafor.o
	 gcc -Wall -g -c fotele.c -o fotele.o
klienci.o: klienci.c semafor.o
	 gcc -Wall -g -c klienci.c -o klienci.o
fryzjerzy: fryzjerzy.o semafor.o
	 gcc -Wall -g fryzjerzy.o semafor.o -o fryzjerzy
fotele: fotele.o semafor.o
	 gcc -Wall -g fotele.o semafor.o -o fotele
klienci: klienci.o semafor.o
	 gcc -Wall -g klienci.o semafor.o -o klienci
salon_fryzjerski: salon_fryzjerski.c fryzjerzy.o fotele.o klienci.o semafor.o
	 gcc -Wall -g salon_fryzjerski.c fryzjerzy.o fotele.o klienci.o semafor.o -o salon_fryzjerski
run: all
	 ./salon_fryzjerski
clean:
	 rm -f *.o fryzjerzy fotele klienci salon_fryzjerski
