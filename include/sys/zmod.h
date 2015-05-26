/*
 * Copyright 2008 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#ifndef	_ZMOD_H
#define	_ZMOD_H

#pragma ident	"@(#)zmod.h	1.2	08/02/14 SMI"

#ifdef	__cplusplus
extern "C" {
#endif

/*
 * zmod - RFC-1950-compatible decompression routines
 *
 * This file provides the public interfaces to zmod, an in-kernel RFC 1950
 * decompression library.  More information about the implementation of these
 * interfaces can be found in the usr/src/uts/common/zmod/ directory.
 */

#define	Z_OK		0
#define	Z_STREAM_END	1
#define	Z_NEED_DICT	2
#define	Z_ERRNO		(-1)
#define	Z_STREAM_ERROR	(-2)
#define	Z_DATA_ERROR	(-3)
#define	Z_MEM_ERROR	(-4)
#define	Z_BUF_ERROR	(-5)
#define	Z_VERSION_ERROR	(-6)

#define	Z_NO_COMPRESSION	0
#define	Z_BEST_SPEED		1
#define	Z_BEST_COMPRESSION	9
#define	Z_DEFAULT_COMPRESSION	(-1)

extern int z_uncompress(void *, size_t *, const void *, size_t);
extern int z_compress(void *, size_t *, const void *, size_t);
extern int z_compress_level(void *, size_t *, const void *, size_t, int);
extern const char *z_strerror(int);

#ifdef	__cplusplus
}
#endif

#endif	/* _ZMOD_H */
