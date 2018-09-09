#include "file.h"

File::File() {
    _file = nullptr;
}

File::~File() {
    close();
}

bool File::open(const char *fpath, OpenMode open_mode, DataMode data_mode) {

    close();

    char smode[3] = {};
    switch(open_mode)
    {
    case READ:
        smode[0] = 'r';
        break;
    case WRITE:
        smode[0] = 'w';
        break;
    default:
        assert(!"Unknown mode");
        break;
    }

    if (data_mode == BINARY)
        smode[1] = 'b';

    _file = fopen(fpath, smode);

    return _file != nullptr;
}

void File::close() {
    if (_file) {
        fclose(_file);
        _file = nullptr;
    }
}

void File::read_all_bytes(Vector<uint8_t> &out_bytes) {
    assert(_file != nullptr);
    fseek(_file, 0, SEEK_END); // Non portable, but `EOF, SEEK_CUR` didn't work...
    size_t len = ftell(_file);
    out_bytes.resize_no_init(len);
    fseek(_file, 0, 0);
    fread(out_bytes.data(), sizeof(uint8_t), out_bytes.size(), _file);
}

// Static
bool File::read_all_bytes(const char *fpath, Vector<uint8_t> &out_bytes) {
    File f;
    if(!f.open(fpath, READ, BINARY))
        return false;
    f.read_all_bytes(out_bytes);
    f.close();
    return true;
}


