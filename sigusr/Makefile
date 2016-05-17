SOURCES=sigusr.c
OBJFILES=$(SOURCES:.c=.o)
TARGET=sigusr

all: $(TARGET)

$(TARGET): $(OBJFILES)
	gcc $^ -o $(TARGET)

.c.o:
	gcc -c $< -o $@

clean:
	rm -f $(TARGET)
	rm -f $(OBJFILES)