all: producer consumer

producer: producer.cpp ; g++ -o producer producer.cpp -pthread -lrt

consumer: consumer.cpp ; g++ -o consumer consumer.cpp -pthread -lrt