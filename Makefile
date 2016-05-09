CC=g++

lda:lda.cpp
	$(CC) lda.cpp -o lda -O3 -std=c++11

.PHONY:clean
clean:
	$(RM) lda
