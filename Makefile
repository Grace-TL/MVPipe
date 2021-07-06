EXEC += mvpipe mvpipe_byte 
EXEC2D += mvpipe2D mvpipe2D_byte


SRCCOM += $(wildcard lib/*.c) \
	          $(wildcard lib/*.cpp) 

INCLUCOM += $(wildcard lib/*.h) \
	          $(wildcard lib/*.hpp) 



CFLAGS = -Wall -O3 -std=c++11 
CFLAGS += -D TRACE_DIR_RAM
CC=g++ 

LIBS = -lPcap++ -lPacket++ -lCommon++ -lpcap -lpthread -lm -liniparser 
INCLUDES= -I/usr/local/include/pcapplusplus/ -Ilib/


mvpipe: main_mv.cpp  
	$(CC) $(CFLAGS) -DBITLEVEL $(INCLUDES) -o $@ $< -DTOPSS $(SRCCOM) src/mvpipe.cpp $(LIBS) 

mvpipe_byte: main_mv.cpp  
	$(CC) $(CFLAGS) $(INCLUDES)  -o $@ $< -DTOPSS $(SRCCOM) src/mvpipe.cpp $(LIBS) 

mvpipe2D: main_mv2D.cpp  
	$(CC) $(CFLAGS) -DBITLEVEL -DDIMENSION2 $(INCLUDES) -o $@ $< -DTOPSS $(SRCCOM) src/mvpipe2D.cpp $(LIBS) 

mvpipe2D_byte: main_mv2D.cpp  
	$(CC) $(CFLAGS) -DDIMENSION2 $(INCLUDES) -o $@ $< -DTOPSS $(SRCCOM) src/mvpipe2D.cpp $(LIBS) 



clean_1D:
	rm -rf $(EXEC)

clean_2D:
	rm -rf $(EXEC2D)

