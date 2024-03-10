all: TCP_Receiver TCP_Sender File_Generator

File_Generator: File_Generator.o
	gcc -Wall -g -o File_Generator File_Generator.o

TCP_Receiver: TCP_Receiver.o
	gcc -Wall -g -o TCP_Receiver TCP_Receiver.o

TCP_Sender: TCP_Sender.o
	gcc -Wall -g -o TCP_Sender TCP_Sender.o

TCP_Receiver.o: TCP_Receiver.c
	gcc -Wall -g -c TCP_Receiver.c

TCP_Sender.o: TCP_Sender.c 
	gcc -Wall -g -c TCP_Sender.c

File_Generator.o: File_Generator.c
	gcc -Wall -g -c File_Generator.c

clean_files:
	rm -f random_file.txt
	rm -rf assets

clean:
	rm -f *.o TCP_Receiver TCP_Sender File_Generator
	rm -f random_file.txt
	rm -rf assets
