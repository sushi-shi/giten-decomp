#ifndef GITEN_PLATFORM_D3DMATH_H
#define GITEN_PLATFORM_D3DMATH_H

#include <rva.h>

#include <Platform/Direct3D.h>

// Matrix helpers of the Direct3D layer (C++ only), in the style of the DX5
// SDK's D3DUtil_Set*Matrix functions; angles are in degrees.

extern D3DMATRIX g_identityMatrix;

void SetZeroMatrix(D3DMATRIX& m);
void SetTranslateMatrix(D3DMATRIX& m, D3DVALUE x, D3DVALUE y, D3DVALUE z);
void SetScaleMatrix(D3DMATRIX& m, D3DVALUE x, D3DVALUE y, D3DVALUE z);
void SetRotateXMatrix(D3DMATRIX& m, D3DVALUE degrees);
void SetRotateYMatrix(D3DMATRIX& m, D3DVALUE degrees);
void SetRotateZMatrix(D3DMATRIX& m, D3DVALUE degrees);
void MultiplyMatrix(D3DMATRIX& a, D3DMATRIX& b, D3DMATRIX& product);

// Projects `in` through the view `matrix` onto the 640x328 view (x, y in
// pixels, z the projected depth).
void ProjectVector(D3DMATRIX* matrix, D3DVECTOR* in, D3DVECTOR* out);

// Looks from `from` toward `at` with +y up.
void SetViewMatrix(D3DMATRIX& m, D3DVECTOR& from, D3DVECTOR& at);

// @identity-TODO: the roles of the three values (the device setup passes
// 160, 160, 1281: near plane, focal scale, far plane) are read from the
// matrix entries only.
void SetProjectionMatrix(D3DMATRIX& m, D3DVALUE nearPlane, D3DVALUE scale, D3DVALUE farPlane);

#endif // GITEN_PLATFORM_D3DMATH_H
