CXXFLAGS=--std=c++17 -Wall -ggdb
INCLUDE=
LIBFLAGS=
OBJDIR=bin/obj/
OUTDIR=bin/
CC=g++

# run program building if necessary
run: $(OUTDIR)serve
	./$(OUTDIR)serve

debug: $(OUTDIR)serve
	lldb ./$(OUTDIR)serve

# build program but not run
$(OUTDIR)serve: config $(OBJDIR)main.o $(OBJDIR)serve.o $(OBJDIR)file.o
	$(CC) $(OBJDIR)*.o $(LIBFLAGS) -o $(OUTDIR)serve

# build source 
$(OBJDIR)main.o: src/main.cpp
	$(CC) $(CXXFLAGS) -c $< $(INCLUDE) -o $@

$(OBJDIR)serve.o: src/net/serve.cpp
	$(CC) $(CXXFLAGS) -c $< $(INCLUDE) -o $@

$(OBJDIR)file.o: src/util/file.cpp
	$(CC) $(CXXFLAGS) -c $< $(INCLUDE) -o $@

config:
	mkdir -p $(OBJDIR)

clean:
	rm -rf $(OUTDIR)