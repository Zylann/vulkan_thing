#include "variant.h"

bool Variant::get_bool() const {
    assert(_type == BOOL);
    return _data.bool_value;
}

int64_t Variant::get_int() const {
    assert(_type == INT);
    return _data.int_value;
}

double Variant::get_float() const {
    assert(_type == FLOAT);
    return _data.float_value;
}

TaggedPointer Variant::get_tagged_pointer() const {
    assert(_type == TAGGED_POINTER);
    return _data.tagged_pointer_value;
}

String Variant::get_string() const {
    assert(_type == STRING);
    assert(!"Not implemented");
    //return _data.string_value;
    return String();
}

void Variant::reset() {
    switch(_type) {
    case STRING:
        assert(!"Not implemented");
        //_data.string_value.~String();
        break;
    }
    _type = NIL;
}

void Variant::set(bool v) {
    reset();
    _data.bool_value = v;
    _type = BOOL;
}

void Variant::set(int64_t v) {
    reset();
    _data.int_value = v;
    _type = INT;
}

void Variant::set(double v) {
    reset();
    _data.float_value = v;
    _type = FLOAT;
}

void Variant::set(TaggedPointer v) {
    reset();
    _data.tagged_pointer_value = v;
    _type = TAGGED_POINTER;
}

void Variant::set(String v) {
    reset();
    assert(!"Not implemented");
    //_data.string_value.String(v);
    _type = STRING;
}

void to_string(String &dst, const Variant &v) {

    switch(v.get_type()) {

    case Variant::NIL:
        dst += L"null";
        break;

    case Variant::BOOL:
        if(v.get_bool())
            dst += L"true";
        else
            dst += L"false";
        break;

    case Variant::INT:
        to_string(dst, v.get_int());
        break;

    case Variant::FLOAT:
        // TODO Float tostring
        dst += L"<float>";
        break;

    case Variant::TAGGED_POINTER:
        dst.append_format(L"<ptr: %, tag: %>", v.get_tagged_pointer().ptr, v.get_tagged_pointer().tag);
        break;

    case Variant::STRING:
        dst += v.get_string();
        break;

    default:
        assert(false);
        break;
    }
}


