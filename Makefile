CC := gcc
CFLAGS := -Isrc -static
AR := ar

.POSIX: clean

all: libbiglib.a

clean:
	rm -f libbiglib.a $(shell find src/ -name "*.o")

libbiglib.a: $(patsubst %.c, %.o, $(shell find src/ -name "*.c"))
	$(AR) r $@ $^

%.o: %.c
	$(CC) -c $< -o $@ $(CFLAGS)
