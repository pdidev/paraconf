default: app_1

app_1: main_1.c types_1.h
	mpicc -Wall main_1.c -lparaconf -o app_1


clean:
	@rm -rfv *~ *.o

cleanall: clean
	@rm -rfv app_1
