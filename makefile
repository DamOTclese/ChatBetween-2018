
# -----------------------------------------------------------------------
# Makefile for a tiny chat program which uses UDP broadcasted frames
#
# Fredric L. Rice, June 2018
# http://www.crystallake.name
# fred @ crystal lake . name
# 
# -----------------------------------------------------------------------

chat : main.o ChatClass.o LoggingClass.o
	g++ -o chat main.o ChatClass.o LoggingClass.o

main.o : main.cpp
	g++ $(WARN_FLAGS) -c main.cpp

ChatClass.o : ChatClass.cpp
	g++ $(WARN_FLAGS) -c ChatClass.cpp

LoggingClass.o : LoggingClass.cpp
	g++ $(WARN_FLAGS) -c LoggingClass.cpp

clean :
	rm chat main.o ChatClass.o LoggingClass.o
