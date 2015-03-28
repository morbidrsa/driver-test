#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

#define __unused __attribute__((unused))

#define DEVPATH "/dev/reflect"

enum test_result {
	TEST_PASS,
	TEST_FAIL,
	TEST_SKIP,
	TEST_ERROR,
};

struct test_ctx {
	int reserved;
};

/* ---------- Test cases ---------- */
static enum test_result test_open_close(struct test_ctx __unused *ctx)
{
	int fd;

	fd = open(DEVPATH, O_RDWR);
	if (!fd)
		return TEST_ERROR;

	close(fd);

	return TEST_PASS;
}

static enum test_result test_simple(struct test_ctx __unused *ctx)
{
	int fd;
	size_t len;
	char *buf;
	enum test_result ret;
	int rc;
	char *fixture = "This is a test\n";

	len = strlen(fixture);

	buf = malloc(len);
	if (!buf) {
		ret = TEST_ERROR;
		goto out;
	}

	fd = open(DEVPATH, O_RDWR);
	if (!fd) {
		ret = TEST_ERROR;
		goto out_free;
	}

	rc = write(fd, fixture, len);
	if (rc < 0) {
		ret = TEST_ERROR;
		goto out_close;
	}

	rc = read(fd, buf, len);
	if (rc < 0) {
		ret = TEST_ERROR;
		goto out_close;
	}

	rc = memcmp(buf, fixture, len);
	if (rc) {
		ret = TEST_FAIL;
	} else {
		ret = TEST_PASS;
	}

out_close:
	close(fd);

out_free:
	free(buf);
out:
	return ret;
}

struct test_case {
	char name[30];
	char description[60];
	enum test_result (*test_fn)(struct test_ctx *ctx);
} test_cases[] = {
	{
		.name = "open/close",
		.description = "Simple open/close test cycle",
		.test_fn = test_open_close,
	},
	{
		.name = "simple",
		.description = "Simple open/write/read/close test cycle",
		.test_fn = test_simple,
	},
};

#define N_TESTS (sizeof(test_cases)/sizeof(test_cases[0]))

static const char *print_result(enum test_result res)
{
	switch (res) {
	case TEST_PASS:
		return "PASS";
	case TEST_FAIL:
		return "FAIL";
	case TEST_SKIP:
		return "SKIP";
	case TEST_ERROR:
		return "ERROR";
	}
	return "ERROR";
}


int main(int __unused argc, char __unused **argv)
{
	enum test_result res;
	unsigned int i, j;

	for (i = 0; i < N_TESTS; i++) {
		struct test_ctx ctx;

		printf("Testing %s", test_cases[i].name);
		for (j = 0; j < 60 - strlen(test_cases[i].name); j++)
			printf(".");

		res = test_cases[i].test_fn(&ctx);
		printf("%s\n", print_result(res));
	}

	return 0;
}
