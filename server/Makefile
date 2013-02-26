all:server

server:obj/server.o obj/calender.o
	g++ -Wall -pthread obj/server.o obj/calender.o -o exec/server

obj/server.o:src/server.cpp
	g++ -Wall -pthread -c src/server.cpp -o obj/server.o

obj/calender.o:src/calender.cpp
	g++ -Wall -pthread -c src/calender.cpp -o obj/calender.o

clean:
	rm -rf obj/*o exec/*
