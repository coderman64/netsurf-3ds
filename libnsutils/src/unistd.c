/*
 * Copyright 2015 Vincent Sanders <vince@netsurf-browser.org>
 *
 * This file is part of libnsutils.
 *
 * Licensed under the MIT License,
 *                http://www.opensource.org/licenses/mit-license.php
 */

/**
 * \file
 * unistd style operations.
 */

#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

#include "nsutils/unistd.h"

/* exported interface documented in nsutils/unistd.h */
ssize_t nsu_pwrite(int fd, const void *buf, size_t count, off_t offset)
{
#if (defined(__riscos) || defined(__amiga) || defined(_WIN32) || defined(__serenity__))
        off_t sk;

        sk = lseek(fd, offset, SEEK_SET);
        if (sk == (off_t)-1) {
#if defined(__amiga)
                /* amigaos cannot seek past the end of the existing file so use
                 * ftruncate to make the file long enough and retry the seek.
                 */
                int ret;
                if (errno == ESPIPE) {
                        ret = ftruncate(fd, offset);
                        if (ret == -1) {
                                return -1;
                        }
                        sk = lseek(fd, offset, SEEK_SET);
                        if (sk == (off_t)-1) {
                                return -1;
                        }
                } else
#endif
                return -1;
        }
        return write(fd, buf, count);
#else
        return pwrite(fd, buf, count, offset);
#endif
}

/* exported interface documented in nsutils/unistd.h */
ssize_t nsu_pread(int fd, void *buf, size_t count, off_t offset)
{
#if (defined(__riscos) || defined(_WIN32) || defined(__serenity__) || (defined(__amiga) && !defined(__amigaos4__)))
        off_t sk;

        sk = lseek(fd, offset, SEEK_SET);
        if (sk == -1) {
                return (off_t)-1;
        }
        return read(fd, buf, count);
#else
        return pread(fd, buf, count, offset);
#endif

}
