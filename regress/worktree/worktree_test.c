/*
 * Copyright (c) 2018 Stefan Sperling <stsp@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/param.h>
#include <sys/queue.h>
#include <sys/limits.h>
#include <sys/stat.h>

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "got_error.h"
#include "got_object.h"
#include "got_refs.h"
#include "got_repository.h"
#include "got_worktree.h"

#include "got_worktree_priv.h"
#include "got_path_priv.h"

#define GOT_REPO_PATH "../../../"

static int verbose;

void
test_printf(char *fmt, ...)
{
	va_list ap;

	if (!verbose)
		return;

	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);
}

static int
remove_got_dir(const char *worktree_path)
{
	char *path;

	if (asprintf(&path, "%s/%s", worktree_path, GOT_WORKTREE_GOT_DIR) == -1)
		return 0;
	rmdir(path);
	free(path);
	return 1;
}

static int
remove_meta_file(const char *worktree_path, const char *name)
{
	char *path;

	if (asprintf(&path, "%s/%s/%s", worktree_path, GOT_WORKTREE_GOT_DIR,
	    name) == -1)
		return 0;
	unlink(path);
	free(path);
	return 1;
}

static void
remove_workdir(const char *worktree_path)
{
	remove_meta_file(worktree_path, GOT_REF_HEAD);
	remove_meta_file(worktree_path, GOT_WORKTREE_FILE_INDEX);
	remove_meta_file(worktree_path, GOT_WORKTREE_REPOSITORY);
	remove_got_dir(worktree_path);
	rmdir(worktree_path);
}

static int
check_meta_file_exists(const char *worktree_path, const char *name)
{
	FILE *f;
	char *path;

	if (asprintf(&path, "%s/%s/%s", worktree_path, GOT_WORKTREE_GOT_DIR,
	    name) == -1)
		return 0;
	f = fopen(path, "r");
	free(path);
	if (f == NULL)
		return 0;
	fclose(f);
	return 1;
}

static int
worktree_init(const char *repo_path)
{
	const struct got_error *err;
	struct got_repository *repo = NULL;
	struct got_reference *head_ref = NULL;
	char worktree_path[PATH_MAX];
	int ok = 0;

	err = got_repo_open(&repo, repo_path);
	if (err != NULL || repo == NULL)
		goto done;
	err = got_ref_open(&head_ref, repo, GOT_REF_HEAD);
	if (err != NULL || head_ref == NULL)
		goto done;

	strlcpy(worktree_path, "worktree-XXXXXX", sizeof(worktree_path));
	if (mkdtemp(worktree_path) == NULL)
		goto done;

	err = got_worktree_init(worktree_path, head_ref, repo);
	if (err != NULL)
		goto done;

	/* Ensure required files were created. */
	if (!check_meta_file_exists(worktree_path, GOT_REF_HEAD))
		goto done;
	if (!check_meta_file_exists(worktree_path, GOT_WORKTREE_FILE_INDEX))
		goto done;
	if (!check_meta_file_exists(worktree_path, GOT_WORKTREE_REPOSITORY))
		goto done;
	ok = 1;
	remove_workdir(worktree_path);
done:
	if (head_ref)
		got_ref_close(head_ref);
	if (repo)
		got_repo_close(repo);
	return ok;
}

static int
obstruct_meta_file(char **path, const char *worktree_path, const char *name)
{
	FILE *f;
	char *s = "This file should not be here\n";

	if (asprintf(path, "%s/%s/%s", worktree_path, GOT_WORKTREE_GOT_DIR,
	    name) == -1)
		return 0;
	f = fopen(*path, "w+");
	if (f == NULL) {
		free(*path);
		return 0;
	}
	if (fwrite(s, 1, strlen(s), f) != strlen(s)) {
		free(*path);
		return 0;
	}
	fclose(f);
	return 1;
}

static int
worktree_init_exists(const char *repo_path)
{
	const struct got_error *err;
	struct got_repository *repo = NULL;
	struct got_reference *head_ref = NULL;
	char worktree_path[PATH_MAX];
	char *gotpath = NULL;
	char *path;
	int ok = 0;
	FILE *f;

	err = got_repo_open(&repo, repo_path);
	if (err != NULL || repo == NULL)
		goto done;
	err = got_ref_open(&head_ref, repo, GOT_REF_HEAD);
	if (err != NULL || head_ref == NULL)
		goto done;

	strlcpy(worktree_path, "worktree-XXXXXX", sizeof(worktree_path));
	if (mkdtemp(worktree_path) == NULL)
		goto done;
	if (mkdir(worktree_path, GOT_DEFAULT_DIR_MODE) == -1 && errno != EEXIST)
		goto done;

	if (asprintf(&gotpath, "%s/%s", worktree_path, GOT_WORKTREE_GOT_DIR)
	    == -1)
		goto done;
	if (mkdir(gotpath, GOT_DEFAULT_DIR_MODE) == -1 && errno != EEXIST)
		goto done;

	/* Create files which got_worktree_init() will try to create as well. */

	if (!obstruct_meta_file(&path, worktree_path, GOT_REF_HEAD))
		goto done;
	err = got_worktree_init(worktree_path, head_ref, repo);
	if (err != NULL && err->code == GOT_ERR_ERRNO && errno == EEXIST)
		ok++;
	unlink(path);
	free(path);

	if (!obstruct_meta_file(&path, worktree_path, GOT_WORKTREE_FILE_INDEX))
		goto done;
	err = got_worktree_init(worktree_path, head_ref, repo);
	if (err != NULL && err->code == GOT_ERR_ERRNO && errno == EEXIST)
		ok++;
	unlink(path);
	free(path);

	if (!obstruct_meta_file(&path, worktree_path, GOT_WORKTREE_REPOSITORY))
		goto done;
	err = got_worktree_init(worktree_path, head_ref, repo);
	if (err != NULL && err->code == GOT_ERR_ERRNO && errno == EEXIST)
		ok++;
	unlink(path);
	free(path);

done:
	if (head_ref)
		got_ref_close(head_ref);
	if (repo)
		got_repo_close(repo);
	free(gotpath);
	if (ok == 3)
		remove_workdir(worktree_path);
	return (ok == 3);
}

#define RUN_TEST(expr, name) \
	{ test_ok = (expr);  \
	printf("test %s %s\n", (name), test_ok ? "ok" : "failed"); \
	failure = (failure || !test_ok); }


void
usage(void)
{
	fprintf(stderr, "usage: worktree_test [-v] [REPO_PATH]\n");
}

int
main(int argc, char *argv[])
{
	int test_ok = 0, failure = 0;
	const char *repo_path;
	int ch;
	int vflag = 0;

	while ((ch = getopt(argc, argv, "v")) != -1) {
		switch (ch) {
		case 'v':
			verbose = 1;
			break;
		default:
			usage();
			return 1;
		}
	}
	argc -= optind;
	argv += optind;

	if (argc == 0)
		repo_path = GOT_REPO_PATH;
	else if (argc == 1)
		repo_path = argv[0];
	else {
		usage();
		return 1;
	}

	RUN_TEST(worktree_init(repo_path), "init");
	RUN_TEST(worktree_init_exists(repo_path), "init exists");

	return failure ? 1 : 0;
}
