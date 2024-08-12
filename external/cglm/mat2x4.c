/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "cglm.h"
#include "call.h"

CGLM_EXPORT
void
glmc_mat2x4_copy(mat2x4 mat, mat2x4 dest) {
  glm_mat2x4_copy(mat, dest);
}

CGLM_EXPORT
void
glmc_mat2x4_zero(mat2x4 mat) {
  glm_mat2x4_zero(mat);
}

CGLM_EXPORT
void
glmc_mat2x4_make(const float * __restrict src, mat2x4 dest) {
  glm_mat2x4_make(src, dest);
}

CGLM_EXPORT
void
glmc_mat2x4_mul(mat2x4 m1, mat4x2 m2, mat4 dest) {
  glm_mat2x4_mul(m1, m2, dest);
}

CGLM_EXPORT
void
glmc_mat2x4_mulv(mat2x4 m, vec2 v, vec4 dest) {
  glm_mat2x4_mulv(m, v, dest);
}

CGLM_EXPORT
void
glmc_mat2x4_transpose(mat2x4 m, mat4x2 dest) {
  glm_mat2x4_transpose(m, dest);
}

CGLM_EXPORT
void
glmc_mat2x4_scale(mat2x4 m, float s) {
  glm_mat2x4_scale(m, s);
}
