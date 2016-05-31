SOURCES=rshd.cpp
OBJFILES=$(SOURCES:.cpp=.o)
TARGET=rshd

all: $(TARGET)

$(TARGET): $(OBJFILES)
	g++ $^ -o $(TARGET)

.cpp.o:
	g++ -c -std=c++11 $< -o $@

clean:
	rm -f $(TARGET)
	rm -f $(OBJFILES)
