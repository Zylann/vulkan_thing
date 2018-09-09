#ifndef HEADER_FILE_H
#define HEADER_FILE_H

#include <stdio.h>
#include "vector.h"

class File {
public:
    enum OpenMode {
        READ,
        WRITE
    };

    enum DataMode {
        TEXT,
        BINARY
    };

    File();
    ~File();

    bool open(const char *fpath, OpenMode open_mode, DataMode data_mode);
    void close();

    void read_all_bytes(Vector<uint8_t> &out_bytes);

    static bool read_all_bytes(const char *fpath, Vector<uint8_t> &out_bytes);

private:
    FILE *_file;
};

#endif // HEADER_FILE_H
