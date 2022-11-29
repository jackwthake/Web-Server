CXXFLAGS=--std=c++17 -Wall -ggdb -pthread
INCLUDE=-Ilib/openssl/include/
LIBFLAGS=-Llib/openssl -lssl
OBJDIR=bin/obj/
OUTDIR=bin/
CC=g++

# build program but not run
$(OUTDIR)serve: config $(OBJDIR)main.o $(OBJDIR)route.o $(OBJDIR)serve.o $(OBJDIR)log.o $(OBJDIR)pool.o
	$(CC) $(OBJDIR)*.o $(LIBFLAGS) -o $(OUTDIR)serve

# build source 
$(OBJDIR)main.o: src/main.cpp
	$(CC) $(CXXFLAGS) -c $< $(INCLUDE) -o $@

$(OBJDIR)route.o: src/net/route.cpp
	$(CC) $(CXXFLAGS) -c $< $(INCLUDE) -o $@

$(OBJDIR)serve.o: src/net/serve.cpp
	$(CC) $(CXXFLAGS) -c $< $(INCLUDE) -o $@

$(OBJDIR)log.o: src/util/log.cpp
	$(CC) $(CXXFLAGS) -c $< $(INCLUDE) -o $@

$(OBJDIR)pool.o: src/util/pool.cpp
	$(CC) $(CXXFLAGS) -c $< $(INCLUDE) -o $@

# run program building if necessary
run: $(OUTDIR)serve
	./$(OUTDIR)serve

debug: $(OUTDIR)serve
	lldb ./$(OUTDIR)serve

config:
	mkdir -p $(OBJDIR)

clean:
	rm -rf $(OUTDIR)
