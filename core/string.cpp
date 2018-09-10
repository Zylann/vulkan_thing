#include "string.h"

// static

//String::String(const String &other): Vector(other) { }

String::String(const Char *p_cstr) {

    size_t len = get_length(p_cstr);
    resize_no_init(len + 1);
    Char *d = data();
    d[len] = 0;
    memcpy(d, p_cstr, len * sizeof(Char));
}

String &String::operator+=(const char p_char) {
    *this += (Char)p_char;
    return *this;
}

String &String::operator+=(const Char p_char) {
    size_t i = length();
    push_back(0);
    Char *d = data();
    d[i] = p_char;
    return *this;
}

String &String::operator+=(const char *p_cstr) {

    size_t begin = length();
    size_t len = get_length(p_cstr);
    resize_no_init(begin + len + 1);
    Char *d = data() + begin;
    d[len] = 0;
    // TODO Handle multibyte UTF-8
    for(int i = 0; i < len; ++i) {
        d[i] = p_cstr[i];
    }

    return *this;
}

String &String::operator+=(const Char *p_cstr) {

    size_t begin = length();
    size_t len = get_length(p_cstr);
    resize_no_init(begin + len + 1);
    Char *d = data() + begin;
    d[len] = 0;
    memcpy(d, p_cstr, len * sizeof(Char));

    return *this;
}

String &String::operator+=(const String &p_other) {

    if (p_other.length() != 0) {
        size_t begin = length();
        resize_no_init(begin + p_other.size());
        Char *d = data();
        // This copy includes the zero character
        memcpy(d + begin, p_other.data(), p_other.size() * sizeof(Char));
    }

    return *this;
}

void String::append_region(const Char *p_cstr, size_t from, size_t len) {

    // TODO Check this only in debug
    assert(from + len <= get_length(p_cstr));

    size_t begin = length();
    resize_no_init(begin + len + 1);
    Char *d = data() + begin;
    d[len] = 0;
    memcpy(d, p_cstr + from, len * sizeof(Char));
}

void String::append_region(const String &p_str, size_t from, size_t len) {

    // TODO Check this only in debug
    assert(from + len <= p_str.length());
    append_region(p_str.data(), from, len);
}

bool String::find_not_escaped(const Char *str, size_t len, Char p_c, size_t & out_index, size_t p_from) {

    assert(p_from < len);

    for (size_t i = p_from; i < len; ++i) {

        if (str[i] == p_c) {

            if (i == 0) {
                out_index = i;
                return true;

            } else if(i == 1 && str[i - 1] != ESCAPE_CHAR) {
                out_index = i;
                return true;

            } else if(str[i - 2] != ESCAPE_CHAR && str[i - 1] != ESCAPE_CHAR) {
                out_index = i;
                return true;
            }
        }
    }

    return false;
}

void append_int(String &p_dst, int64_t p_num, int base, bool capitalize_hex) {

    bool sign = p_num < 0;

    int64_t n = p_num;

    int chars = 0;
    do {
        n /= base;
        ++chars;
    } while (n);

    if (sign)
        chars++;

    size_t prev_len = p_dst.length();
    p_dst.resize_no_init(p_dst.length() + chars + 1);
    Char *dst = p_dst.data() + prev_len;

    dst[chars] = 0;
    n = p_num;

    do {
        int mod = Math::absi(n % base);
        if (mod >= 10) {
            char a = (capitalize_hex ? 'A' : 'a');
            dst[--chars] = a + (mod - 10);
        } else {
            dst[--chars] = '0' + mod;
        }

        n /= base;
    } while (n);

    if (sign)
        dst[0] = '-';
}

