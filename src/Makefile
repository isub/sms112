sms112:
	g++ -DDEBUG -g -I ~/project/utils/src/utils/ipconnector -c ~/project/utils/src/utils/ipconnector/ipconnector.cpp -o ipconnector.o
	g++ -DDEBUG -g -I ~/project/utils/src/utils/config -c ~/project/utils/src/utils/config/config.cpp -o config.o
	g++ -DDEBUG -g -I ~/project/utils/src/utils/log -c ~/project/utils/src/utils/log/log.cpp -o log.o
	gcc -DDEBUG -g --std=c++11 -I ~/project/utils/src -I ../libsmpp34-1.10/src -I /usr/local/include ipconnector.o config.o log.o sms112_*.cpp -lsmpp34 -lpthread -lstdc++ -L/usr/local/lib -lcurl -o ../sms112

install:
	cp -f ../sms112 /usr/local/bin/sms112