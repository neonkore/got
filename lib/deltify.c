/*
 * Copyright (c) 2020 Ori Bernstein
 * Copyright (c) 2021 Stefan Sperling <stsp@openbsd.org>
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

#include <sys/types.h>
#include <sys/queue.h>

#include <assert.h>
#include <endian.h>
#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sha1.h>

#include "got_error.h"

#include "got_lib_deltify.h"

#ifndef MIN
#define	MIN(_a,_b) ((_a) < (_b) ? (_a) : (_b))
#endif

static uint32_t geartab[] = {
    0x67ed26b7, 0x32da500c, 0x53d0fee0, 0xce387dc7, 0xcd406d90, 0x2e83a4d4,
    0x9fc9a38d, 0xb67259dc, 0xca6b1722, 0x6d2ea08c, 0x235cea2e, 0x3149bb5f,
    0x1beda787, 0x2a6b77d5, 0x2f22d9ac, 0x91fc0544, 0xe413acfa, 0x5a30ff7a,
    0xad6fdde0, 0x444fd0f5, 0x7ad87864, 0x58c5ff05, 0x8d2ec336, 0x2371f853,
    0x550f8572, 0x6aa448dd, 0x7c9ddbcf, 0x95221e14, 0x2a82ec33, 0xcbec5a78,
    0xc6795a0d, 0x243995b7, 0x1c909a2f, 0x4fded51c, 0x635d334b, 0x0e2b9999,
    0x2702968d, 0x856de1d5, 0x3325d60e, 0xeb6a7502, 0xec2a9844, 0x0905835a,
    0xa1820375, 0xa4be5cab, 0x96a6c058, 0x2c2ccd70, 0xba40fce3, 0xd794c46b,
    0x8fbae83e, 0xc3aa7899, 0x3d3ff8ed, 0xa0d42b5b, 0x571c0c97, 0xd2811516,
    0xf7e7b96c, 0x4fd2fcbd, 0xe2fdec94, 0x282cc436, 0x78e8e95c, 0x80a3b613,
    0xcfbee20c, 0xd4a32d1c, 0x2a12ff13, 0x6af82936, 0xe5630258, 0x8efa6a98,
    0x294fb2d1, 0xdeb57086, 0x5f0fddb3, 0xeceda7ce, 0x4c87305f, 0x3a6d3307,
    0xe22d2942, 0x9d060217, 0x1e42ed02, 0xb6f63b52, 0x4367f39f, 0x055cf262,
    0x03a461b2, 0x5ef9e382, 0x386bc03a, 0x2a1e79c7, 0xf1a0058b, 0xd4d2dea9,
    0x56baf37d, 0x5daff6cc, 0xf03a951d, 0xaef7de45, 0xa8f4581e, 0x3960b555,
    0xffbfff6d, 0xbe702a23, 0x8f5b6d6f, 0x061739fb, 0x98696f47, 0x3fd596d4,
    0x151eac6b, 0xa9fcc4f5, 0x69181a12, 0x3ac5a107, 0xb5198fe7, 0x96bcb1da,
    0x1b5ddf8e, 0xc757d650, 0x65865c3a, 0x8fc0a41a, 0x87435536, 0x99eda6f2,
    0x41874794, 0x29cff4e8, 0xb70efd9a, 0x3103f6e7, 0x84d2453b, 0x15a450bd,
    0x74f49af1, 0x60f664b1, 0xa1c86935, 0xfdafbce1, 0xe36353e3, 0x5d9ba739,
    0xbc0559ba, 0x708b0054, 0xd41d808c, 0xb2f31723, 0x9027c41f, 0xf136d165,
    0xb5374b12, 0x9420a6ac, 0x273958b6, 0xe6c2fad0, 0xebdc1f21, 0xfb33af8b,
    0xc71c25cd, 0xe9a2d8e5, 0xbeb38a50, 0xbceb7cc2, 0x4e4e73f0, 0xcd6c251d,
    0xde4c032c, 0x4b04ac30, 0x725b8b21, 0x4eb8c33b, 0x20d07b75, 0x0567aa63,
    0xb56b2bb7, 0xc1f5fd3a, 0xcafd35ca, 0x470dd4da, 0xfe4f94cd, 0xfb8de424,
    0xe8dbcf40, 0xfe50a37a, 0x62db5b5d, 0xf32f4ab6, 0x2c4a8a51, 0x18473dc0,
    0xfe0cbb6e, 0xfe399efd, 0xdf34ecc9, 0x6ccd5055, 0x46097073, 0x139135c2,
    0x721c76f6, 0x1c6a94b4, 0x6eee014d, 0x8a508e02, 0x3da538f5, 0x280d394f,
    0x5248a0c4, 0x3ce94c6c, 0x9a71ad3a, 0x8493dd05, 0xe43f0ab6, 0x18e4ed42,
    0x6c5c0e09, 0x42b06ec9, 0x8d330343, 0xa45b6f59, 0x2a573c0c, 0xd7fd3de6,
    0xeedeab68, 0x5c84dafc, 0xbbd1b1a8, 0xa3ce1ad1, 0x85b70bed, 0xb6add07f,
    0xa531309c, 0x8f8ab852, 0x564de332, 0xeac9ed0c, 0x73da402c, 0x3ec52761,
    0x43af2f4d, 0xd6ff45c8, 0x4c367462, 0xd553bd6a, 0x44724855, 0x3b2aa728,
    0x56e5eb65, 0xeaf16173, 0x33fa42ff, 0xd714bb5d, 0xfbd0a3b9, 0xaf517134,
    0x9416c8cd, 0x534cf94f, 0x548947c2, 0x34193569, 0x32f4389a, 0xfe7028bc,
    0xed73b1ed, 0x9db95770, 0x468e3922, 0x0440c3cd, 0x60059a62, 0x33504562,
    0x2b229fbd, 0x5174dca5, 0xf7028752, 0xd63c6aa8, 0x31276f38, 0x0646721c,
    0xb0191da8, 0xe00e6de0, 0x9eac1a6e, 0x9f7628a5, 0xed6c06ea, 0x0bb8af15,
    0xf119fb12, 0x38693c1c, 0x732bc0fe, 0x84953275, 0xb82ec888, 0x33a4f1b3,
    0x3099835e, 0x028a8782, 0x5fdd51d7, 0xc6c717b3, 0xb06caf71, 0x17c8c111,
    0x61bad754, 0x9fd03061, 0xe09df1af, 0x3bc9eb73, 0x85878413, 0x9889aaf2,
    0x3f5a9e46, 0x42c9f01f, 0x9984a4f4, 0xd5de43cc, 0xd294daed, 0xbecba2d2,
    0xf1f6e72c, 0x5551128a, 0x83af87e2, 0x6f0342ba,
};

static uint64_t
hashblk(const unsigned char *p, off_t n)
{
	unsigned char buf[SHA1_DIGEST_LENGTH];
	uint64_t h;
	SHA1_CTX ctx;
	SHA1Init(&ctx);
	SHA1Update(&ctx, p, n);
	SHA1Final(buf, &ctx);
	memcpy(&h, buf, sizeof(h));
	return be64toh(h);
}

static const struct got_error *
addblk(struct got_delta_table *dt, FILE *f, off_t len, off_t offset, uint64_t h)
{
	const struct got_error *err = NULL;
	int i, nalloc;
	uint8_t buf[GOT_DELTIFY_MAXCHUNK];
	size_t r = 0;

	if (len == 0)
		return NULL;

	i = h % dt->nalloc;
	while (dt->blocks[i].len != 0) {
		if (r == 0) {
			if (fseeko(f, offset, SEEK_SET) == -1)
				return got_error_from_errno("fseeko");
			r = fread(buf, 1, len, f);
			if (r != len) {
				if (ferror(f))
					return got_ferror(f, GOT_ERR_IO);
				return NULL;
			}
		}
		/*
		 * Avoid adding duplicate blocks.
		 * NB: A matching hash is insufficient for detecting equality.
		 * The hash can only detect inequality, so only check 'len'.
		 */
		if (len == dt->blocks[i].len) {
			uint8_t buf2[GOT_DELTIFY_MAXCHUNK];
			if (fseeko(f, dt->blocks[i].offset, SEEK_SET) == -1)
				return got_error_from_errno("fseeko");
			if (fread(buf2, 1, len, f) != len)
				return got_ferror(f, GOT_ERR_IO);
			if (memcmp(buf, buf2, len) == 0)
				return NULL;
		}

		i = (i + 1) % dt->nalloc;
	}
	assert(dt->blocks[i].len == 0);
	dt->blocks[i].len = len;
	dt->blocks[i].offset = offset;
	dt->blocks[i].hash = h;
	dt->nblocks++;
	if (dt->nalloc < 2 * dt->nblocks) {
		struct got_delta_block *db;
		nalloc = dt->nalloc * 2;
		db = dt->blocks;
		dt->blocks = calloc(nalloc, sizeof(struct got_delta_block));
		if (dt->blocks == NULL) {
			err = got_error_from_errno("calloc");
			dt->blocks = db;
			return err;
		}
		dt->nalloc = nalloc;
		/*
		 * Recompute all block positions. Hash-based indices of blocks
		 * in the array depend on the allocated length of the array.
		 */
		dt->nblocks = 0;
		for (i = 0; i < nalloc; i++) {
			if (db[i].len == 0)
				continue;
			err = addblk(dt, f, db[i].len, db[i].offset,
			    db[i].hash);
			if (err)
				break;
		}
		free(db);
	}

	return err;
}

static const struct got_error *
lookupblk(struct got_delta_block **block, struct got_delta_table *dt,
    unsigned char *p, off_t len, FILE *basefile)
{
	int i;
	uint64_t h;
	uint8_t buf[GOT_DELTIFY_MAXCHUNK];
	size_t r;

	*block = NULL;

	h = hashblk(p, len);
	for (i = h % dt->nalloc; dt->blocks[i].len != 0;
	     i = (i + 1) % dt->nalloc) {
		if (dt->blocks[i].hash != h ||
		    dt->blocks[i].len != len)
			continue;
		if (fseeko(basefile, dt->blocks[i].offset, SEEK_SET) == -1)
			return got_error_from_errno("fseeko");
		r = fread(buf, 1, len, basefile);
		if (r != len)
			return got_ferror(basefile, GOT_ERR_IO);
		if (memcmp(p, buf, len) == 0) {
			*block = &dt->blocks[i];
			break;
		}
	}
	return NULL;
}

static const struct got_error *
nextblk(uint8_t *buf, off_t *blocklen, FILE *f)
{
	uint32_t gh;
	const unsigned char *p;
	size_t r;
	off_t pos = ftello(f);

	*blocklen = 0;

	r = fread(buf, 1, GOT_DELTIFY_MAXCHUNK, f);
	if (r == 0 && ferror(f))
		return got_ferror(f, GOT_ERR_IO);
	if (r < GOT_DELTIFY_MINCHUNK)
		return NULL; /* no more delta-worthy blocks left */

	/* Got a deltifiable block. Find the split-point where it ends. */
	p = buf + GOT_DELTIFY_MINCHUNK;
	gh = 0;
	while (p != buf + r) {
		gh = (gh << 1) + geartab[*p++];
		if ((gh & GOT_DELTIFY_SPLITMASK) == 0)
			break;
	}

	*blocklen = (p - buf);
	if (fseeko(f, pos + *blocklen, SEEK_SET) == -1)
		return got_error_from_errno("fseeko");

	return NULL;
}

const struct got_error *
got_deltify_init(struct got_delta_table **dt, FILE *f, off_t fileoffset,
    off_t filesize)
{
	const struct got_error *err = NULL;
	uint64_t h;

	*dt = calloc(1, sizeof(**dt));
	if (*dt == NULL)
		return got_error_from_errno("calloc");

	(*dt)->nblocks = 0;
	(*dt)->nalloc = 128;
	(*dt)->blocks = calloc((*dt)->nalloc, sizeof(struct got_delta_block));
	if ((*dt)->blocks == NULL) {
		err = got_error_from_errno("calloc");
		goto done;
	}

	if (fseeko(f, fileoffset, SEEK_SET) == -1)
		return got_error_from_errno("fseeko");

	while (fileoffset < filesize) {
		uint8_t buf[GOT_DELTIFY_MAXCHUNK];
		off_t blocklen;
		err = nextblk(buf, &blocklen, f);
		if (err)
			goto done;
		if (blocklen == 0)
			break;
		h = hashblk(buf, blocklen);
		err = addblk(*dt, f, blocklen, fileoffset, h);
		if (err)
			goto done;
		fileoffset += blocklen;
	}
done:
	if (err) {
		free((*dt)->blocks);
		free(*dt);
		*dt = NULL;
	}

	return err;
}

void
got_deltify_free(struct got_delta_table *dt)
{
	if (dt == NULL)
		return;
	free(dt->blocks);
	free(dt);
}

static const struct got_error *
emitdelta(struct got_delta_instruction **deltas, int *ndeltas, int copy,
    off_t offset, off_t len)
{
	struct got_delta_instruction *d, *p;

	*ndeltas += 1;
	p = reallocarray(*deltas, *ndeltas,
	    sizeof(struct got_delta_instruction));
	if (p == NULL)
		return got_error_from_errno("realloc");
	*deltas = p;
	d = &(*deltas)[*ndeltas - 1];
	d->copy = copy;
	d->offset = offset;
	d->len = len;
	return NULL;
}

static const struct got_error *
stretchblk(FILE *basefile, struct got_delta_block *block, FILE *f,
    off_t filesize, off_t *blocklen)
{
	uint8_t basebuf[GOT_DELTIFY_MAXCHUNK], buf[GOT_DELTIFY_MAXCHUNK];
	size_t base_r, r, i;
	off_t orig_blocklen = *blocklen;
	off_t pos = ftello(f);
	int buf_equal = 1;

	if (fseeko(basefile, block->offset, SEEK_SET) == -1)
		return got_error_from_errno("fseeko");

	while (buf_equal && *blocklen < (1 << 24) - 1) {
		base_r = fread(basebuf, 1, sizeof(basebuf), basefile);
		if (base_r == 0) {
			if (ferror(basefile))
				return got_ferror(basefile, GOT_ERR_IO);
			break;
		}
		r = fread(buf, 1, sizeof(buf), f);
		if (r == 0) {
			if (ferror(f))
				return got_ferror(f, GOT_ERR_IO);
			break;
		}
		for (i = 0; i < MIN(base_r, r); i++) {
			if (buf[i] != basebuf[i]) {
				buf_equal = 0;
				break;
			}
			(*blocklen)++;
		}
	}

	if (fseeko(f, pos + *blocklen - orig_blocklen, SEEK_SET) == -1)
		return got_error_from_errno("fseeko");

	return NULL;
}

const struct got_error *
got_deltify(struct got_delta_instruction **deltas, int *ndeltas,
    FILE *f, off_t fileoffset, off_t filesize,
    struct got_delta_table *dt, FILE *basefile,
    off_t basefile_size)
{
	const struct got_error *err = NULL;
	const off_t offset0 = fileoffset;

	*deltas = NULL;
	*ndeltas = 0;

	/*
	 * offset0 indicates where data to be deltified begins.
	 * For example, we want to avoid deltifying a Git object header at
	 * the beginning of the file.
	 */
	if (fseeko(f, offset0, SEEK_SET) == -1)
		return got_error_from_errno("fseeko");

	while (fileoffset < filesize) {
		uint8_t buf[GOT_DELTIFY_MAXCHUNK];
		off_t blocklen;
		struct got_delta_block *block;
		err = nextblk(buf, &blocklen, f);
		if (err)
			break;
		if (blocklen == 0)
			break;
		err = lookupblk(&block, dt, buf, blocklen, basefile);
		if (err)
			break;
		if (block != NULL) {
			/*
			 * We have found a matching block in the delta base.
			 * Attempt to stretch the block as far as possible and
			 * generate a copy instruction.
			 */
			err = stretchblk(basefile, block, f, filesize,
			    &blocklen);
			if (err)
				break;
			emitdelta(deltas, ndeltas, 1, block->offset, blocklen);
		} else {
			/*
			 * No match.
			 * This block needs to be sourced from the file itself.
			 */
			emitdelta(deltas, ndeltas, 0, fileoffset - offset0,
			    blocklen);
		}
		fileoffset += blocklen;
	}

	if (err) {
		free(*deltas);
		*deltas = NULL;
		*ndeltas = 0;
	}
	return err;
}