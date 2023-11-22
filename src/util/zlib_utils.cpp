// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include "zlib.h"
#include "zlib_utils.h"
#include <assert.h>
namespace ZlibUtils {
static const size_t kDefaultBlockSize = 8192;

int deflate(unsigned char* data, size_t len, tinynet::IOBuffer* out_buf) {
    int ret, flush;
    unsigned have;
    z_stream zs;

    /* allocate deflate state */
    zs.zalloc = Z_NULL;
    zs.zfree = Z_NULL;
    zs.opaque = Z_NULL;
    ret = deflateInit2(&zs, Z_DEFAULT_COMPRESSION, Z_DEFLATED, MAX_WBITS + 16, MAX_MEM_LEVEL, Z_DEFAULT_STRATEGY);
    if (ret != Z_OK) {
        return ret;
    }

    zs.avail_in = (uInt)len;
    flush = Z_FINISH;
    zs.next_in = data;

    /* run deflate() on input until output buffer not full, finish
       compression if all of source has been read in */
    do {
        out_buf->reserve(out_buf->size() + kDefaultBlockSize);
        zs.avail_out = kDefaultBlockSize;
        zs.next_out = reinterpret_cast<Bytef*>(out_buf->end());
        ret = deflate(&zs, flush);    /* no bad return value */
        assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
        have = kDefaultBlockSize - zs.avail_out;
        out_buf->commit(have);
    } while (zs.avail_out == 0);

    /* clean up and return */
    (void)deflateEnd(&zs);
    return Z_OK;
}

int inflate(unsigned char* data, size_t len, tinynet::IOBuffer* out_buf) {
    int ret;
    unsigned have;
    z_stream zs;

    /* allocate inflate state */
    zs.zalloc = Z_NULL;
    zs.zfree = Z_NULL;
    zs.opaque = Z_NULL;
    zs.avail_in = 0;
    zs.next_in = Z_NULL;
    ret = inflateInit2(&zs, MAX_WBITS + 16);
    if (ret != Z_OK) {
        return ret;
    }


    /* decompress until deflate stream ends or end of file */
    do {
        zs.avail_in = (uInt)len;
        if (zs.avail_in == 0)
            break;
        zs.next_in = data;

        /* run inflate() on input until output buffer not full */
        do {
            out_buf->reserve(out_buf->size() + kDefaultBlockSize);
            zs.avail_out = kDefaultBlockSize;
            zs.next_out = reinterpret_cast<Bytef*>(out_buf->end());
            ret = inflate(&zs, Z_NO_FLUSH);
            assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
            switch (ret) {
            case Z_NEED_DICT:
                ret = Z_DATA_ERROR;     /* and fall through */
            case Z_DATA_ERROR:
            case Z_MEM_ERROR:
                (void)inflateEnd(&zs);
                return ret;
            }
            have = kDefaultBlockSize - zs.avail_out;
            out_buf->commit(have);
        } while (zs.avail_out == 0);

        /* done when inflate() says it's done */
    } while (ret != Z_STREAM_END);

    /* clean up and return */
    (void)inflateEnd(&zs);
    return ret == Z_STREAM_END ? Z_OK : Z_DATA_ERROR;
}
}