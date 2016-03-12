SOURCES=hello_world.c
OBJFILES=$(SOURCES:.c=.o)
TARGET=hello_world

all: $(TARGET)

$(TARGET): $(OBJFILES)
	gcc $^ -o $(TARGET)

.c.o:
	gcc -c $< -o $@

clean:
	rm -f $(TARGET)
	rm -f $(OBJFILES)