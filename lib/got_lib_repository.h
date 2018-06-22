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

#define GOT_PACKIDX_CACHE_SIZE	64
#define GOT_PACK_CACHE_SIZE	GOT_PACKIDX_CACHE_SIZE

#define GOT_OBJECT_CACHE_SIZE	8192

struct got_objcache_entry {
	SIMPLEQ_ENTRY(got_objcache_entry) entry;
	struct got_object_id id;
	struct got_object *obj;
};

struct got_repository {
	char *path;
	char *path_git_dir;

	/* The pack index cache speeds up search for packed objects. */
	struct got_packidx *packidx_cache[GOT_PACKIDX_CACHE_SIZE];

	/* Open file handles for pack files. */
	struct got_pack packs[GOT_PACK_CACHE_SIZE];

	struct got_object_idset *objcache;
	int ncached;
	int cache_hit;
	int cache_miss;
};

const struct got_error*
got_repo_cache_object(struct got_repository *, struct got_object_id *,
    struct got_object *);
struct got_object *got_repo_get_cached_object(struct got_repository *,
    struct got_object_id *);
