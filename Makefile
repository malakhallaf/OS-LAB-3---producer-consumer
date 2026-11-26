.PHONY: all cleanup

all: producer consumer

producer: producer.cpp
	g++ -Wall -std=c++20 producer.cpp -o producer -pthread -lrt

consumer: consumer.cpp
	g++ -Wall -std=c++20 consumer.cpp -o consumer -pthread -lrt

cleanup:
	rm -f producer consumer