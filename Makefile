psdatabase.o: psdatabase/psdatabase.cpp psdatabase/psdatabase.hpp
	g++ -Wall -g ./psdatabase/psdatabase.cpp -c

server.o: server/server.cpp server/server.hpp
	g++ -Wall -g ./server/server.cpp -c

serialize.o: serialize/serialize.cpp serialize/serialize.hpp
	g++ -Wall -g ./serialize/serialize.cpp -c

main_server: main_server.cpp psdatabase.o server.o serialize.o
	g++ -Wall -g ./main_server.cpp ./psdatabase.o ./server.o ./serialize.o -o main_server

main_client: main_client.cpp psdatabase.o server.o serialize.o
	g++ -Wall -g ./main_client.cpp ./psdatabase.o ./server.o ./serialize.o -o main_client
