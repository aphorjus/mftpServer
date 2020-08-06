all:
	gcc -c -std=c99 -Wall -pedantic mftp.c
	gcc -c -std=c99 -Wall -pedantic mftpserve.c
	gcc mftp.o -o mftp
	gcc mftpserve.o -o mftpserve

clean:
	rm mftpserve mftp *.o

run: all
	./ok