#ifndef HEADER_STRING_H
#define HEADER_STRING_H

#include "vector.h"
#include "math/math_funcs.h"

// Encapsulates a zero-terminated string inside a Vector structure
class String : public Vector<Char, 20> {
public:

    static const Char ESCAPE_CHAR = L'\\';

    template <typename C>
    static size_t get_length(const C *p_cstr) {
        assert(p_cstr != nullptr);
        size_t len = 0;
        while (p_cstr[len] != 0)
            ++len;
        return len;
    }

    String() { }
    //String(const String &other);
    String(const Char *p_cstr);

    String &String::operator+=(const char p_cstr);
    String &String::operator+=(const Char p_cstr);
    String &String::operator+=(const char *p_cstr);
    String &String::operator+=(const Char *p_cstr);
    String &String::operator+=(const String &p_other);

    void append_region(const Char *p_cstr, size_t from, size_t len);
    void append_region(const String &p_str, size_t from, size_t len);

    // Length does not include '\0'
    inline size_t length() const {
        size_t s = size();
        return s == 0 ? 0 : s - 1;
    }

    static bool find_not_escaped(const Char *str, size_t len, Char p_c, size_t & out_index, size_t p_from = 0);

    bool find_not_escaped(Char p_c, size_t & out_index, size_t p_from = 0) {
        return find_not_escaped(data(), length(), p_c, out_index, p_from);
    }

private:
    template <typename A>
    static size_t _format_single(const Char *src, size_t src_len, String &dst, size_t from, A arg) {

        if(from >= src_len)
            return from;

        size_t placeholder_pos = 0;
        if (find_not_escaped(src, src_len, L'%', placeholder_pos, from)) {

            dst.append_region(src, from, placeholder_pos);
            to_string(dst, arg);

            return placeholder_pos + 1;
        }

        return src_len;
    }

    template <typename A>
    static void _format(const Char *src, size_t src_len, String &dst, size_t from, A last_arg) {
        size_t next = _format_single(src, src_len, dst, from, last_arg);
        dst.append_region(src, next, src_len - next);
    }

    template <typename A, typename... Args>
    static void _format(const Char *src, size_t src_len, String &dst, size_t from, A arg, Args... args) {
        size_t next = _format_single(src, src_len, dst, from, arg);
        _format(src, src_len, dst, next, args...);
    }

public:
    template <typename... Args>
    void append_format(const Char *src, Args... args) {
        _format(src, get_length(src), *this, 0, args...);
    }

    template <typename... Args>
    static String format(const Char *src, Args... args) {
        String dst;
        dst.append_format(src, args...);
        return dst;
    }

/*private:
    template <typename A>
    static void concat(String &dst, A arg) {
        to_string(dst, arg);
    }

    template <typename A, typename... Args>
    static void concat(String &dst, A arg, Args... args) {
        to_string(dst, arg);
        concat(dst, args...);
    }*/
};

// This one should be overloaded for usual stringification,
// which will allow to make use of the existing buffer rather than potentially allocate one each time
inline void to_string(String &dst, const String &s) {
    dst += s;
}

void append_int(String &p_dst, int64_t p_num, int base = 10, bool capitalize_hex = false);

inline void to_string(String &dst, char p_char) {
    dst += p_char;
}

inline void to_string(String &dst, Char p_char) {
    dst += p_char;
}

inline void to_string(String &dst, int64_t p_num) {
    append_int(dst, p_num);
}

inline void to_string(String &dst, int p_num) {
    append_int(dst, p_num);
}

inline void to_string(String &dst, void *ptr) {
    append_int(dst, (size_t)ptr, 16);
}

// Shortcut if we need the easy version
template <typename T>
String to_string(const T &x) {
    String s;
    to_string(s, x);
    return s;
}

#endif // HEADER_STRING_H



