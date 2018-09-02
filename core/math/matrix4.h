#ifndef HEADER_MATRIX4_H
#define HEADER_MATRIX4_H

#include "vector3.h"
#include "vector4.h"
#include "fov.h"
#include "quaternion.h"

/// \brief A 4x4 float matrix.
/// Values are stored in row-major layout.
/// Translations are stored in the last row.
class Matrix4 {
public :

    /// \brief Constructs a matrix initialized to identity
    Matrix4();

    /// \brief Constructs a matrix which is a copy of another
    Matrix4(const Matrix4 & other);

    /// \brief Constructs a matrix from raw values
    Matrix4(const float values[16]);

    /// \brief Copies values from another matrix
    void set(const Matrix4 & other);

    /// \brief Copies raw values into the matrix
    void set(const float values[16]);

    /// \brief Gets the one-dimensional index of one cell's coordinates
    /// \param row: row
    /// \param col: column
    inline int get_cell_index(int row, int col) const { return col | (row * 4); }

    /// \brief Gets a cell's value from row and column
    inline float Matrix4::get_cell(int row, int col) const { return m_v[get_cell_index(row, col)]; }

    /// \brief Sets a cell's value from row and column
    inline void Matrix4::set_cell(int row, int col, float v) { m_v[get_cell_index(row, col)] = v; }

    /// \brief Sets the matrix to identity
    void load_identity();

    /// \brief Sets the matrix to a 3D perspective projection matrix with vertical field of view and aspect ratio
    void load_perspective_projection(
        const float fov, const float ratio,
        const float near, const float far
    );

    /// \brief Sets the matrix to a 3D perspective projection matrix with general field of view
    void load_perspective_projection(Fov fov, float near, float far);

    /// \brief Sets the matrix to an orthographic projection matrix
    void load_ortho2d_projection(
        const float left, const float top,
        const float right, const float bottom,
        const float near, const float far
    );

    /// \brief Sets the matrix to a lookat matrix for use in 3D cameras.
    /// \param eye: position of the camera
    /// \param target: position to look at
    /// \param up: normalized up vector
    void load_look_at(
        const Vector3 eye,
        const Vector3 target,
        const Vector3 up
    );

    /// \brief Sets the matrix to a translation matrix.
    /// \param vx: translation X delta
    /// \param vy: translation Y delta
    /// \param vz: translation Z delta
    void load_translation(const float vx, const float vy, const float vz);

    /// \brief Sets the matrix to a rotation matrix.
    /// The rotation is defined from an angle around an axis.
    /// \param t: angle around the axis, in radians
    /// \param x: normalized X coordinate of the axis
    /// \param y: normalized Y coordinate of the axis
    /// \param z: normalized Z coordinate of the axis
    void load_rotation(const float t, const float x, const float y, const float z);

    /// \brief Sets the matrix to a scaling matrix
    void load_scale(const float sx, const float sy, const float sz);

    /// \brief Sets the matrix to the result of the product of the given matrices
    void set_by_product(const Matrix4 & in_a, const Matrix4 & in_b);

    /// \brief Sets the matrix to the result of the product of the given matrices, as if they were 3x3.
    void set_by_product_3x3(const Matrix4 & a, const Matrix4 & b);

    /// \brief Gets the transposed version of this matrix into another
    /// \param out_result: matrix that will be filled with the result
    void get_transposed(Matrix4 & out_result) const;

    /// \brief Transposes the matrix.
    void transpose();

    /// \brief Transposes the first 3 rows and columns of the matrix (as if it was 3x3)
    void transpose_3x3();

    /// \brief Sets the translation part of the matrix
    void set_translation(const Vector3 v);

    /// \brief Sets the rotation part of the matrix.
    void set_rotation(const Quaternion q);

    /// \brief Scales the transformation represented by the matrix by a factor.
    void scale_transform(const Vector3 s);

    /// \brief Gets the raw values of the matrix as a one-dimensional array.
    /// \return array of 16 float values
    inline const float * values() const { return m_v; }

    /// \brief Calculates and returns the determinant of the matrix.
    float get_det() const;

    /// \brief Tries to compute the inverse matrix.
    /// \param out_result: result of the inversion. Not modified if it fails.
    /// \return true if the matrix could be inverted, false otherwise.
    bool get_inverse(Matrix4 & out_result) const;

    /// \brief Applies the transformation represented by the matrix to a 3D point.
    /// \param p: point to transform
    /// \return transformed point
    Vector3 transform(const Vector3 p) const;

    Vector4 transform(const Vector4 p) const;

    //-------------------------------------
    // Operators
    //-------------------------------------

    void operator=(const Matrix4 & other);
    inline float & operator()(const int row, const int col) { return m_v[get_cell_index(row, col)]; }
    inline float operator()(const int row, const int col) const { return m_v[get_cell_index(row, col)]; }

private :

    inline float operator[](int i) const { return m_v[i]; }

    /// \brief Cell values.
    ///
    ///  0   1   2   3 | i(x, y, z) right
    ///  4   5   6   7 | j(x, y, z) up
    ///  8   9  10  11 | k(x, y, z) front
    /// 12  13  14  15 | t(x, y, z) offset
    ///
    float m_v[16];
};

#endif // HEADER_MATRIX4_H
