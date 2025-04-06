CFLAGS = -O3 -Wall -Wextra -Werror -fsanitize=address,leak,object-size,bounds-strict,undefined -fsanitize-address-use-after-scope -DNDEBUG

SRCS = mysh.c builtins.c io_helpers.c variables.c commands.c background.c \
	utils/string.c \
	builtins/cd.c
HEADERS = builtins.h io_helpers.h variables.h commands.h background.c types.h \
	utils/string.h utils/minmax.h \
	builtins/cd.h

OBJS = ${SRCS:.c=.o}

all: mysh

debug: CFLAGS = -g -Wall -Wextra -Werror -fsanitize=address,leak,object-size,bounds-strict,undefined -fsanitize-address-use-after-scope
debug: mysh

# Linking
mysh: ${OBJS}
	gcc ${CFLAGS} ${OBJS} -o $@

# Compiling
%.o: %.c ${HEADERS}
	gcc ${CFLAGS} -c $< -o $@

clean:
	rm ${OBJS} mysh
