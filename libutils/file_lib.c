/*
   Copyright (C) CFEngine AS

   This file is part of CFEngine 3 - written and maintained by CFEngine AS.

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; version 3.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA

  To the extent this program is licensed as part of the Enterprise
  versions of CFEngine, the applicable Commercial Open Source License
  (COSL) may apply to this file if you as a licensee so wish it. See
  included file COSL.txt.
*/

#include <file_lib.h>

#include <alloc.h>
#include <logging.h>

bool FileCanOpen(const char *path, const char *modes)
{
    FILE *test = NULL;

    if ((test = fopen(path, modes)) != NULL)
    {
        fclose(test);
        return true;
    }
    else
    {
        return false;
    }
}

#define READ_BUFSIZE 4096

Writer *FileRead(const char *filename, size_t max_size, bool *truncated)
{
    int fd = open(filename, O_RDONLY);
    if (fd == -1)
        return NULL;

    Writer *w = StringWriter();
    for (;;)
    {
        char buf[READ_BUFSIZE];
        /* Reading more data than needed is deliberate. It is a truncation detection. */
        ssize_t read_ = read(fd, buf, READ_BUFSIZE);
        if (read_ == 0)
        {
            if (truncated)
                *truncated = false;
            close(fd);
            return w;
        }
        if (read_ < 0)
        {
            if (errno == EINTR)
                continue;
            WriterClose(w);
            close(fd);
            return NULL;
        }
        if (read_ + StringWriterLength(w) > max_size)
        {
            WriterWriteLen(w, buf, max_size - StringWriterLength(w));
            if (truncated)
                *truncated = true;
            close(fd);
            return w;
        }
        WriterWriteLen(w, buf, read_);
    }
}

int FullWrite(int desc, const char *ptr, size_t len)
{
    int total_written = 0;

    while (len > 0)
    {
        int written = write(desc, ptr, len);

        if (written < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }

            return written;
        }

        total_written += written;
        ptr += written;
        len -= written;
    }

    return total_written;
}

int FullRead(int fd, char *ptr, size_t len)
{
    int total_read = 0;

    while (len > 0)
    {
        int bytes_read = read(fd, ptr, len);

        if (bytes_read < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }

            return -1;
        }

        if (bytes_read == 0)
        {
            return total_read;
        }

        total_read += bytes_read;
        ptr += bytes_read;
        len -= bytes_read;
    }

    return total_read;
}
