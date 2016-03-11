SOURCES=cat.c
OBJFILES=$(SOURCES:.c=.o)
TARGET=cat

all: $(TARGET)

$(TARGET): $(OBJFILES)
	gcc $^ -o $(TARGET)

.c.o:
	gcc -c $< -o $@

clean:
	rm $(TARGET)
	rm $(OBJFILES)