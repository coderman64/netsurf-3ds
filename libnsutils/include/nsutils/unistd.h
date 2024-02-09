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
 * Time related operations.
 */

#ifndef NSUTILS_UNISTD_H_
#define NSUTILS_UNISTD_H_

#include <unistd.h>

/**
 * Perform a write operation at a specific offset
 *
 * This writes the data into the file specifid by teh file handle at teh given
 * offset. The offset may be beyond the existing file extent.
 *
 * This provides a uniform interface to the pwrite operation without system
 * specific implementation details.
 *
 * \note The write pointer on the fd may be moved by this operation which
 * differs from the posix version.
 *
 * \param fd The file descriptor.
 * \param buf The data to write.
 * \param count The length of data in \a buf to write.
 * \param offset The offset into the file to write the data.
 * \return the number of bytes written or -1 on error and errno set.
 */
ssize_t nsu_pwrite(int fd, const void *buf, size_t count, off_t offset);

/**
 * Perform a read from a specific offset.
 *
 * This provides a uniform interface to the POSIX pread operation without
 * system specific implementation details.
 *
 * \note The read pointer on the fd may be moved by this operation which
 * differs from the POSIX version.
 *
 * \param fd The file descriptor.
 * \param buf The buffer to place the data into.
 * \param count The length of data to read.
 * \param offset The offset into the file to read the data from.
 * \return the number of bytes read or -1 on error and errno set.
 */
ssize_t nsu_pread(int fd, void *buf, size_t count, off_t offset);

#endif
