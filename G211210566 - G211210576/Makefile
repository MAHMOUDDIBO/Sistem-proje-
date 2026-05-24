CC = gcc
CFLAGS = -Wall -Wextra -O2

# Tüm hedefler
all: tarsau

# Statik kütüphane oluşturma
libtarsau.a: tarsau.o
	ar rcs libtarsau.a tarsau.o

# Kütüphane objesinin derlenmesi
tarsau.o: tarsau.c tarsau.h
	$(CC) $(CFLAGS) -c tarsau.c

# Main dosyasının derlenmesi
main.o: main.c tarsau.h
	$(CC) $(CFLAGS) -c main.c

# Programı kütüphane ile statik olarak bağlama (Linking)
tarsau: main.o libtarsau.a
	$(CC) $(CFLAGS) -o tarsau main.o -L. -ltarsau

# Temizlik
clean:
	rm -f tarsau *.o *.a
