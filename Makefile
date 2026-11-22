all: producer consumer

producer: producer.cpp
	g++ -o producer producer.cpp -lrt -pthread

consumer: consumer.cpp
	g++ -o consumer consumer.cpp -lrt -pthread