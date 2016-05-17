SOURCES=main.cpp
OBJFILES=$(SOURCES:.cpp=.o)
TARGET=simplesh

all: $(TARGET)

$(TARGET): $(OBJFILES)
	g++ $^ -o $(TARGET)

.cpp.o:
	g++ -c $< -o $@

clean:
	rm -f $(TARGET)
	rm -f $(OBJFILES)
