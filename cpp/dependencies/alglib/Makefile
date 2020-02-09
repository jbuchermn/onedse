CXX = g++

SRCS=$(wildcard alglib/*.cpp)
OBJS=$(SRCS:.cpp=.o)

.phony: all clean 

%.o: %.cpp
	$(CXX) -c -O3 $<

libalglib.a: $(OBJS)
	ar r libalglib.a *.o

clean:
	rm -f *.o libalglib.a 
