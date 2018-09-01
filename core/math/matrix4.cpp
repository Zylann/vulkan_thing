#include <cstring> // For memcpy
#include <cmath>

#include "matrix4.h"
#include "math_funcs.h"

// Coordinates systems / left-handed / right-handed
// http://viz.aset.psu.edu/gho/sem_notes/3d_fundamentals/html/3d_coordinates.html

const float g_identity[16] = {
    1, 0, 0, 0,
    0, 1, 0, 0,
    0, 0, 1, 0,
    0, 0, 0, 1
};

Matrix4::Matrix4() {
    load_identity();
}

Matrix4::Matrix4(const Matrix4 & other) {
    set(other);
}

Matrix4::Matrix4(const float values[16]) {
    set(values);
}

void Matrix4::operator=(const Matrix4 & other) {
    set(other);
}

void Matrix4::set(const Matrix4 & other) {
    memcpy(m_v, other.m_v, sizeof(m_v));
}

void Matrix4::set(const float values[16]) {
    memcpy(m_v, values, sizeof(m_v));
}

void Matrix4::load_identity() {
    set(g_identity);
}

void Matrix4::load_perspective_projection(const float fovy, const float ratio, const float near, const float far) {

    //  0   1   2   3
    //  4   5   6   7
    //  8   9  10  11
    // 12  13  14  15
    memset(m_v, 0, 16 * sizeof(float)); // zeros
#if 0
    // Right-handed
    m_v[5] = 1.f / tan(fovy * 0.5f);
    m_v[0] = m_v[5] / ratio;
    m_v[10] = (near + far) / (near - far);
    m_v[11] = -1;
    m_v[14] = (2.f*near*far) / (near - far);
#else
    // Left-handed
    m_v[5] = 1.f / tan(fovy * 0.5f);
    m_v[0] = m_v[5] / ratio;
    m_v[10] = (near + far) / (far - near);
    m_v[11] = 1.f;
    m_v[14] = -2.f * near * far / (far - near);
#endif
}

void Matrix4::load_perspective_projection(Fov fov, float near, float far) {

    m_v[0] = 2.f / (fov.tan_right + fov.tan_left);
    m_v[5] = 2.f / (fov.tan_up + fov.tan_down);
    m_v[8] = (fov.tan_left - fov.tan_right) / (fov.tan_right + fov.tan_left);
    m_v[9] = (fov.tan_up - fov.tan_down) / (fov.tan_up + fov.tan_down);
    m_v[10] = (near + far) / (far - near);
    m_v[11] = 1.f;
    m_v[14] = -2.f * near * far / (far - near);

    //m_v[0] = 2 * near / (right - left);
    //m_v[5] = 2 * near / (top - bottom);
    //m_v[8] = (right + left) / (right - left);
    //m_v[9] = (top + bottom) / (top - bottom);
    //m_v[10] = -(far + near) / (far - near);
    //m_v[11] = -1;
    //m_v[14] = -2.f * near * far / (far - near);
}

void Matrix4::load_ortho2d_projection(
        const float left, const float top,
        const float right, const float bottom,
        const float near, const float far) {

    memset(m_v, 0, 16 * sizeof(float)); // zeros

    //  0   1   2   3
    //  4   5   6   7
    //  8   9  10  11
    // 12  13  14  15
    // TODO FIXME this doesn't works with assymetric view volumes
    m_v[0] = 2.f / (right - left);
    m_v[5] = 2.f / (top - bottom);
    m_v[10] = 2.f / (far - near);
    m_v[14] = near / (near - far);
    m_v[15] = 1;
}

void Matrix4::load_look_at(
        const Vector3 eye,
        const Vector3 target,
        const Vector3 up) {

    // Left-handed

    Vector3 zaxis = target - eye;
    zaxis.normalize();

    Vector3 xaxis = up.cross(zaxis);
    xaxis.normalize();

    Vector3 yaxis = zaxis.cross(xaxis);

    m_v[0] = xaxis.x;
    m_v[1] = yaxis.x;
    m_v[2] = zaxis.x;
    m_v[3] = 0;

    m_v[4] = xaxis.y;
    m_v[5] = yaxis.y;
    m_v[6] = zaxis.y;
    m_v[7] = 0;

    m_v[8] = xaxis.z;
    m_v[9] = yaxis.z;
    m_v[10] = zaxis.z;
    m_v[11] = 0;

    m_v[12] = -xaxis.dot(eye);
    m_v[13] = -yaxis.dot(eye);
    m_v[14] = -zaxis.dot(eye);
    m_v[15] = 1;
}

void Matrix4::load_translation(const float vx, const float vy, const float vz) {
    load_identity();
    m_v[12] = vx;
    m_v[13] = vy;
    m_v[14] = vz;
}

void Matrix4::load_scale(const float sx, const float sy, const float sz) {
    memset(m_v, 0, 16 * sizeof(float)); // zeros
    m_v[0] = sx;
    m_v[5] = sy;
    m_v[10] = sz;
    m_v[15] = 1;
}

void Matrix4::load_rotation(const float t, const float x, const float y, const float z) {

    const float cost = cos(t);
    const float sint = sin(t);

    m_v[0] = x * x * (1.f - cost) + cost;
    m_v[4] = x * y * (1.f - cost) + z * sint;
    m_v[8] = x * z * (1.f - cost) + y * sint;
    m_v[12] = 0;

    m_v[1] = x * y * (1.f - cost) - z * sint;
    m_v[5] = y * y * (1.f - cost) + cost;
    m_v[9] = y * z * (1.f - cost) + x * sint;
    m_v[13] = 0;

    m_v[2] = x * z * (1.f - cost) + y * sint;
    m_v[6] = y * z * (1.f - cost) + x * sint;
    m_v[10] = z * z * (1.f - cost) + cost;
    m_v[14] = 0;

    m_v[3] = 0;
    m_v[7] = 0;
    m_v[11] = 0;
    m_v[15] = 1;
}

void Matrix4::set_translation(const Vector3 v) {

    m_v[12] = v.x;
    m_v[13] = v.y;
    m_v[14] = v.z;
}

void Matrix4::set_rotation(const Quaternion q) {
#if 0
    // http://www.euclideanspace.com/maths/geometry/rotations/conversions/quaternionToMatrix/index.htm

    //1 - 2 * qy2 - 2 * qz2 	2 * qx*qy - 2 * qz*qw 	2 * qx*qz + 2 * qy*qw
    //2 * qx*qy + 2 * qz*qw 	1 - 2 * qx2 - 2 * qz2 	2 * qy*qz - 2 * qx*qw
    //2 * qx*qz - 2 * qy*qw 	2 * qy*qz + 2 * qx*qw 	1 - 2 * qx2 - 2 * qy2

    m_v[0 ] = 1.f - 2.f * q.getY()*q.getY() - 2.f * q.getZ()*q.getZ();
    m_v[1 ] =       2.f * q.getX()*q.getY() - 2.f * q.getZ()*q.getW();
    m_v[2 ] =       2.f * q.getX()*q.getZ() + 2.f * q.getY()*q.getW();

    m_v[4 ] =       2.f * q.getX()*q.getY() + 2.f * q.getZ()*q.getW();
    m_v[5 ] = 1.f - 2.f * q.getX()*q.getX() - 2.f * q.getZ()*q.getZ();
    m_v[6 ] =       2.f * q.getY()*q.getZ() - 2.f * q.getX()*q.getW();

    m_v[8 ] =       2.f * q.getX()*q.getZ() - 2.f * q.getY()*q.getW();
    m_v[9 ] =       2.f * q.getY()*q.getZ() + 2.f * q.getX()*q.getW();
    m_v[10] = 1.f - 2.f * q.getX()*q.getX() - 2.f * q.getY()*q.getY();

#else

    m_v[0 ] = 1.f - 2.f * q.y * q.y - 2.f * q.z * q.z;
    m_v[1 ] =       2.f * q.x * q.y + 2.f * q.z * q.w;
    m_v[2 ] =       2.f * q.x * q.z - 2.f * q.y * q.w;

    m_v[4 ] =       2.f * q.x * q.y - 2.f * q.z * q.w;
    m_v[5 ] = 1.f - 2.f * q.x * q.x - 2.f * q.z * q.z;
    m_v[6 ] =       2.f * q.y * q.z + 2.f * q.x * q.w;

    m_v[8 ] =       2.f * q.x * q.z + 2.f * q.y * q.w;
    m_v[9 ] =       2.f * q.y * q.z - 2.f * q.x * q.w;
    m_v[10] = 1.f - 2.f * q.x * q.x - 2.f * q.y * q.y;

#endif
}

void Matrix4::scale_transform(const Vector3 s) {

    m_v[0] *= s[0];
    m_v[5] *= s[1];
    m_v[10] *= s[2];
}

void Matrix4::set_by_product(const Matrix4 & in_a, const Matrix4 & in_b) {

    //  0   1   2   3
    //  4   5   6   7
    //  8   9  10  11
    // 12  13  14  15

    const Matrix4 & a = in_a;
    const Matrix4 & b = in_b;

    m_v[0] = a[0] * b[0] + a[4] * b[1] + a[8] * b[2] + a[12] * b[3];
    m_v[1] = a[1] * b[0] + a[5] * b[1] + a[9] * b[2] + a[13] * b[3];
    m_v[2] = a[2] * b[0] + a[6] * b[1] + a[10] * b[2] + a[14] * b[3];
    m_v[3] = a[3] * b[0] + a[7] * b[1] + a[11] * b[2] + a[15] * b[3];

    m_v[4] = a[0] * b[4] + a[4] * b[5] + a[8] * b[6] + a[12] * b[7];
    m_v[5] = a[1] * b[4] + a[5] * b[5] + a[9] * b[6] + a[13] * b[7];
    m_v[6] = a[2] * b[4] + a[6] * b[5] + a[10] * b[6] + a[14] * b[7];
    m_v[7] = a[3] * b[4] + a[7] * b[5] + a[11] * b[6] + a[15] * b[7];

    m_v[8] = a[0] * b[8] + a[4] * b[9] + a[8] * b[10] + a[12] * b[11];
    m_v[9] = a[1] * b[8] + a[5] * b[9] + a[9] * b[10] + a[13] * b[11];
    m_v[10] = a[2] * b[8] + a[6] * b[9] + a[10] * b[10] + a[14] * b[11];
    m_v[11] = a[3] * b[8] + a[7] * b[9] + a[11] * b[10] + a[15] * b[11];

    m_v[12] = a[0] * b[12] + a[4] * b[13] + a[8] * b[14] + a[12] * b[15];
    m_v[13] = a[1] * b[12] + a[5] * b[13] + a[9] * b[14] + a[13] * b[15];
    m_v[14] = a[2] * b[12] + a[6] * b[13] + a[10] * b[14] + a[14] * b[15];
    m_v[15] = a[3] * b[12] + a[7] * b[13] + a[11] * b[14] + a[15] * b[15];
}

void Matrix4::set_by_product_3x3(const Matrix4 & a, const Matrix4 & b) {

    //  0   1   2
    //  4   5   6
    //  8   9  10

    m_v[0] = a[0] * b[0] + a[4] * b[1] + a[8] * b[2];
    m_v[1] = a[1] * b[0] + a[5] * b[1] + a[9] * b[2];
    m_v[2] = a[2] * b[0] + a[6] * b[1] + a[10] * b[2];
    m_v[3] = 0;

    m_v[4] = a[0] * b[4] + a[4] * b[5] + a[8] * b[6];
    m_v[5] = a[1] * b[4] + a[5] * b[5] + a[9] * b[6];
    m_v[6] = a[2] * b[4] + a[6] * b[5] + a[10] * b[6];
    m_v[7] = 0;

    m_v[8] = a[0] * b[8] + a[4] * b[9] + a[8] * b[10];
    m_v[9] = a[1] * b[8] + a[5] * b[9] + a[9] * b[10];
    m_v[10] = a[2] * b[8] + a[6] * b[9] + a[10] * b[10];
    m_v[11] = 0;

    m_v[12] = a[0] * b[12] + a[4] * b[13] + a[8] * b[14];
    m_v[13] = a[1] * b[12] + a[5] * b[13] + a[9] * b[14];
    m_v[14] = a[2] * b[12] + a[6] * b[13] + a[10] * b[14];
    m_v[15] = 1;
}

void Matrix4::get_transposed(Matrix4 & out_result) const {

    out_result.m_v[0] = m_v[0];
    out_result.m_v[1] = m_v[4];
    out_result.m_v[2] = m_v[8];
    out_result.m_v[3] = m_v[12];

    out_result.m_v[4] = m_v[1];
    out_result.m_v[5] = m_v[5];
    out_result.m_v[6] = m_v[9];
    out_result.m_v[7] = m_v[13];

    out_result.m_v[8] = m_v[2];
    out_result.m_v[9] = m_v[6];
    out_result.m_v[10] = m_v[10];
    out_result.m_v[11] = m_v[14];

    out_result.m_v[12] = m_v[3];
    out_result.m_v[13] = m_v[7];
    out_result.m_v[14] = m_v[11];
    out_result.m_v[15] = m_v[15];
}

static inline void swap(float &a, float &b) {
    float tmp = a;
    a = b;
    b = tmp;
}

void Matrix4::transpose() {

    swap(m_v[1], m_v[4]);
    swap(m_v[2], m_v[8]);
    swap(m_v[3], m_v[12]);

    swap(m_v[4], m_v[1]);
    swap(m_v[6], m_v[9]);
    swap(m_v[7], m_v[13]);

    swap(m_v[8], m_v[2]);
    swap(m_v[9], m_v[6]);
    swap(m_v[11], m_v[14]);

    swap(m_v[12], m_v[3]);
    swap(m_v[13], m_v[7]);
    swap(m_v[14], m_v[11]);
}

void Matrix4::transpose_3x3() {

    swap(m_v[1], m_v[4]);
    swap(m_v[2], m_v[8]);

    swap(m_v[4], m_v[1]);
    swap(m_v[6], m_v[9]);

    swap(m_v[8], m_v[2]);
    swap(m_v[9], m_v[6]);
}

float Matrix4::get_det() const {
    const Matrix4 & m = *this;
    return
        (m(0, 0) * m(1, 1) - m(0, 1) * m(1, 0)) * (m(2, 2) * m(3, 3) - m(2, 3) * m(3, 2)) -
        (m(0, 0) * m(1, 2) - m(0, 2) * m(1, 0)) * (m(2, 1) * m(3, 3) - m(2, 3) * m(3, 1)) +
        (m(0, 0) * m(1, 3) - m(0, 3) * m(1, 0)) * (m(2, 1) * m(3, 2) - m(2, 2) * m(3, 1)) +
        (m(0, 1) * m(1, 2) - m(0, 2) * m(1, 1)) * (m(2, 0) * m(3, 3) - m(2, 3) * m(3, 0)) -
        (m(0, 1) * m(1, 3) - m(0, 3) * m(1, 1)) * (m(2, 0) * m(3, 2) - m(2, 2) * m(3, 0)) +
        (m(0, 2) * m(1, 3) - m(0, 3) * m(1, 2)) * (m(2, 0) * m(3, 1) - m(2, 1) * m(3, 0));
}

bool Matrix4::get_inverse(Matrix4 & out_result) const {

    float d = get_det();
    if (Math::absf(d) < Math::ROUNDING_ERROR_F32) {
        // The matrix cannot be inverted
        return false;
    }

    d = 1.f / d;

    const Matrix4 & m = *this;

    out_result(0, 0) = d*(
        m(1, 1) * (m(2, 2) * m(3, 3) - m(2, 3) * m(3, 2)) +
        m(1, 2) * (m(2, 3) * m(3, 1) - m(2, 1) * m(3, 3)) +
        m(1, 3) * (m(2, 1) * m(3, 2) - m(2, 2) * m(3, 1))
    );
    out_result(0, 1) = d*(
        m(2, 1) * (m(0, 2) * m(3, 3) - m(0, 3) * m(3, 2)) +
        m(2, 2) * (m(0, 3) * m(3, 1) - m(0, 1) * m(3, 3)) +
        m(2, 3) * (m(0, 1) * m(3, 2) - m(0, 2) * m(3, 1))
    );
    out_result(0, 2) = d*(
        m(3, 1) * (m(0, 2) * m(1, 3) - m(0, 3) * m(1, 2)) +
        m(3, 2) * (m(0, 3) * m(1, 1) - m(0, 1) * m(1, 3)) +
        m(3, 3) * (m(0, 1) * m(1, 2) - m(0, 2) * m(1, 1))
    );
    out_result(0, 3) = d*(
        m(0, 1) * (m(1, 3) * m(2, 2) - m(1, 2) * m(2, 3)) +
        m(0, 2) * (m(1, 1) * m(2, 3) - m(1, 3) * m(2, 1)) +
        m(0, 3) * (m(1, 2) * m(2, 1) - m(1, 1) * m(2, 2))
    );
    out_result(1, 0) = d*(
        m(1, 2) * (m(2, 0) * m(3, 3) - m(2, 3) * m(3, 0)) +
        m(1, 3) * (m(2, 2) * m(3, 0) - m(2, 0) * m(3, 2)) +
        m(1, 0) * (m(2, 3) * m(3, 2) - m(2, 2) * m(3, 3))
    );
    out_result(1, 1) = d*(
        m(2, 2) * (m(0, 0) * m(3, 3) - m(0, 3) * m(3, 0)) +
        m(2, 3) * (m(0, 2) * m(3, 0) - m(0, 0) * m(3, 2)) +
        m(2, 0) * (m(0, 3) * m(3, 2) - m(0, 2) * m(3, 3))
    );
    out_result(1, 2) = d*(
        m(3, 2) * (m(0, 0) * m(1, 3) - m(0, 3) * m(1, 0)) +
        m(3, 3) * (m(0, 2) * m(1, 0) - m(0, 0) * m(1, 2)) +
        m(3, 0) * (m(0, 3) * m(1, 2) - m(0, 2) * m(1, 3))
    );
    out_result(1, 3) = d*(
        m(0, 2) * (m(1, 3) * m(2, 0) - m(1, 0) * m(2, 3)) +
        m(0, 3) * (m(1, 0) * m(2, 2) - m(1, 2) * m(2, 0)) +
        m(0, 0) * (m(1, 2) * m(2, 3) - m(1, 3) * m(2, 2))
    );
    out_result(2, 0) = d*(
        m(1, 3) * (m(2, 0) * m(3, 1) - m(2, 1) * m(3, 0)) +
        m(1, 0) * (m(2, 1) * m(3, 3) - m(2, 3) * m(3, 1)) +
        m(1, 1) * (m(2, 3) * m(3, 0) - m(2, 0) * m(3, 3))
    );
    out_result(2, 1) = d*(
        m(2, 3) * (m(0, 0) * m(3, 1) - m(0, 1) * m(3, 0)) +
        m(2, 0) * (m(0, 1) * m(3, 3) - m(0, 3) * m(3, 1)) +
        m(2, 1) * (m(0, 3) * m(3, 0) - m(0, 0) * m(3, 3))
    );
    out_result(2, 2) = d*(
        m(3, 3) * (m(0, 0) * m(1, 1) - m(0, 1) * m(1, 0)) +
        m(3, 0) * (m(0, 1) * m(1, 3) - m(0, 3) * m(1, 1)) +
        m(3, 1) * (m(0, 3) * m(1, 0) - m(0, 0) * m(1, 3))
    );
    out_result(2, 3) = d*(
        m(0, 3) * (m(1, 1) * m(2, 0) - m(1, 0) * m(2, 1)) +
        m(0, 0) * (m(1, 3) * m(2, 1) - m(1, 1) * m(2, 3)) +
        m(0, 1) * (m(1, 0) * m(2, 3) - m(1, 3) * m(2, 0))
    );
    out_result(3, 0) = d*(
        m(1, 0) * (m(2, 2) * m(3, 1) - m(2, 1) * m(3, 2)) +
        m(1, 1) * (m(2, 0) * m(3, 2) - m(2, 2) * m(3, 0)) +
        m(1, 2) * (m(2, 1) * m(3, 0) - m(2, 0) * m(3, 1))
    );
    out_result(3, 1) = d*(
        m(2, 0) * (m(0, 2) * m(3, 1) - m(0, 1) * m(3, 2)) +
        m(2, 1) * (m(0, 0) * m(3, 2) - m(0, 2) * m(3, 0)) +
        m(2, 2) * (m(0, 1) * m(3, 0) - m(0, 0) * m(3, 1))
    );
    out_result(3, 2) = d*(
        m(3, 0) * (m(0, 2) * m(1, 1) - m(0, 1) * m(1, 2)) +
        m(3, 1) * (m(0, 0) * m(1, 2) - m(0, 2) * m(1, 0)) +
        m(3, 2) * (m(0, 1) * m(1, 0) - m(0, 0) * m(1, 1))
    );
    out_result(3, 3) = d*(
        m(0, 0) * (m(1, 1) * m(2, 2) - m(1, 2) * m(2, 1)) +
        m(0, 1) * (m(1, 2) * m(2, 0) - m(1, 0) * m(2, 2)) +
        m(0, 2) * (m(1, 0) * m(2, 1) - m(1, 1) * m(2, 0))
    );

    return true;
}

Vector3 Matrix4::transform(const Vector3 p) const {
    return Vector3(
        m_v[0] * p.x + m_v[4] * p.y + m_v[8] * p.z + m_v[12],
        m_v[1] * p.x + m_v[5] * p.y + m_v[9] * p.z + m_v[13],
        m_v[2] * p.x + m_v[6] * p.y + m_v[10] * p.z + m_v[14]
    );
}

Vector4 Matrix4::transform(const Vector4 p) const {
    return Vector4(
        m_v[0] * p.x + m_v[4] * p.y + m_v[8] * p.z + m_v[12] * p.w,
        m_v[1] * p.x + m_v[5] * p.y + m_v[9] * p.z + m_v[13] * p.w,
        m_v[2] * p.x + m_v[6] * p.y + m_v[10] * p.z + m_v[14] * p.w,
        m_v[3] * p.x + m_v[7] * p.y + m_v[11] * p.z + m_v[15] * p.w
    );
}


