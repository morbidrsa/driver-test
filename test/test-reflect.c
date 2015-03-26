#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

#define DEVPATH "/dev/reflect"

int main(int argc, char **argv)
{
	int fd;
	size_t len;
	char *buf;
	int ret  = 0;

	if (argc != 2) {
		fprintf(stderr, "Usage: %s <test-string>\n", argv[0]);
		ret = 1;
		goto out;
	}

	len = strlen(argv[1]);

	buf = malloc(len);
	if (!buf) {
		fprintf(stderr, "Unable to allocate %ld bytes for buffer: %s\n",
			len, strerror(errno));
		ret = 1;
		goto out;
	}

	fd = open(DEVPATH, O_RDWR);
	if (!fd) {
		fprintf(stderr, "Unable to open %s: %s\n",
			DEVPATH, strerror(errno));
		ret = 1;
		goto out_free;
	}

	ret = write(fd, argv[1], len);
	if (ret < 0) {
		fprintf(stderr, "Unable to write to %s: %s\n",
			DEVPATH, strerror(errno));
		goto out_free;
	}

	ret = read(fd, buf, len);
	if (ret < 0) {
		fprintf(stderr, "Unable to read from %s: %s\n",
			DEVPATH, strerror(errno));
		goto out_free;
	}

	ret = memcmp(buf, argv[1], len);
	if (ret) {
		printf("Buffer doesn't match\n");
	} else {

		printf("Buffer matches\n");
	}

out_free:
	free(buf);
out:
	return ret;
}
