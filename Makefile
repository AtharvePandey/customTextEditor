# Target: executable "kilo" built from kilo.c
kilo: kilo.c
	$(CC) kilo.c -o kilo -Wall -Wextra -pedantic -std=c99 -g

# Flags explained:
# -Wall      : Enable most common compiler warnings
# -Wextra    : Enable additional, more detailed warnings
# -pedantic  : Enforce strict ISO C compliance
# -std=c99   : Compile using the C99 standard
# -g         : Include debugging information for gdb/lldb

# Optional:
# -Werror    : Treat all warnings as errors (forces clean code)
