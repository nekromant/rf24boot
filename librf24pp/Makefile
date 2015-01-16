

CPPFLAGS=-MD -MP -std=c++0x -Iinclude -Wall -g

objects+=\
	rf24debug.o \
	rf24packet.o \
	rf24transfer.o \
	rf24adaptor.o \
	main.o

-include $(objects:%.o=%.d)

test: $(objects)
	g++ $(CPPFLAGS) -o $(@) $^

%.o: %.cpp
	g++ -c $(CPPFLAGS) -o $(@) $<

clean: 
	rm -f *.o *~ test *.d
