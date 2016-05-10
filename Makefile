lda:lda.cpp
	$(CXX) lda.cpp -o lda -O3 -std=c++11

.PHONY:clean
clean:
	$(RM) lda
