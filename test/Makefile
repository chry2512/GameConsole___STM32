CC ?= clang
CFLAGS = -Wall -Wextra -Werror -fobjc-arc -I../Core/Inc
LDFLAGS = -framework Cocoa

snake_host: snake_host.m ../Core/Src/snake.c ../Core/Src/console.c ../Core/Inc/snake.h ../Core/Inc/console.h
	$(CC) $(CFLAGS) snake_host.m ../Core/Src/snake.c ../Core/Src/console.c -o snake_host $(LDFLAGS)

run: snake_host
	./snake_host

clean:
	rm -f snake_host