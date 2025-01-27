all: fryzjerzy klienci debug salon_fryzjerski
fryzjerzy: fryzjerzy.c 
	 gcc fryzjerzy.c semafor.c -o fryzjerzy -I.
klienci: klienci.c 
	 gcc klienci.c semafor.c -o klienci -I.
debug: debug.c 
	 gcc debug.c semafor.c -o debug -I.
salon_fryzjerski: salon_fryzjerski.c 
	 gcc salon_fryzjerski.c semafor.c -o salon_fryzjerski
run: all
	 ./salon_fryzjerski
clean:
	 rm -f *.o fryzjerzy fotele klienci salon_fryzjerski
