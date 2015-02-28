CC = g++
CPPFLAGS = -I /usr/local/boost_1_57_0
LDFLAGS = -L /usr/local/boost_1_57_0/stage/lib
LDLIBS = -lboost_system -lboost_thread -lpthread -lrt

all: threadpool_boost

clean:
	rm -rf threadpool_boost
	rm -rf *.o
