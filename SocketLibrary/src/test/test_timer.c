#include <Timer.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(void)
{
	Timer t = create_timer(360, 10);

	int fd = fileno(stdin), rv;

	printf("stdin is %d\n", fd);

	char buff[256] = {0};

	int s = 0;
	while (s < 10) {
		printf("read_possible returned %d at step %d\n", rv = read_possible(t, s, fd), s);
		if (0 != rv) {
			printf("read returned %d\n", rv = read(fd, buff, 256));
			buff[rv] = '\0';
			printf("I read \"%s\"\n", buff);
		}
		++s;
	}

	return 0;
}


