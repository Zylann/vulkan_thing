#ifndef HEADER_VARIANT_H
#define HEADER_VARIANT_H

#include "string.h"

struct TaggedPointer {
    void *ptr;
    void *tag;
};

class Variant {
public:
    enum Type {
        // PODs
        NIL,
        BOOL,
        INT,
        FLOAT,
        TAGGED_POINTER,
        // Non-PODs
        STRING,
    };

    union Data {
        bool bool_value;
        int64_t int_value;
        double float_value;
        //String string_value; // TODO Non-POD in union, use a buffer for these
        TaggedPointer tagged_pointer_value;
    };

    Variant(): _type(NIL) { }
    Variant(bool v): _type(BOOL) { _data.bool_value = v; }
    Variant(int64_t v): _type(INT) { _data.int_value = v; }
    Variant(double v): _type(FLOAT) { _data.float_value = v; }
    Variant(TaggedPointer v): _type(TAGGED_POINTER) { _data.tagged_pointer_value = v; }
    //Variant(String v): _type(STRING) { _data.string_value.String(v); }

    Type get_type() const { return _type; }

    bool get_bool() const;
    int64_t get_int() const;
    double get_float() const;
    TaggedPointer get_tagged_pointer() const;
    String get_string() const;

    void reset();

    void set(bool v);
    void set(int64_t v);
    void set(double v);
    void set(TaggedPointer v);
    void set(String v);

private:
    Data _data;
    Type _type;
};

void to_string(String &dst, const Variant &v);

#endif // HEADER_VARIANT_H
