CFLAGS+=-I../include/ 
all:client

client:client.o
	gcc $^ -o $@ $(CFLAGS)
clean:
	rm -rf *.o client

