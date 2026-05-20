#pragma once

#include <math.h>
#include <emscripten/em_math.h>
#include <emscripten/html5.h>
#define em(x, ...) emscripten_math_##x(__VA_ARGS__)

inline static float v_lerp(float a, float b, float t)
{
  return a + (b - a) * t;
}

inline static float r_lerp(float a, float b, float t)
{
  float delta = em(fmod, em(fmod, b - a, M_PI * 2.) + M_PI * 3., M_PI * 2.) - M_PI;
  return a + delta * t;
}

typedef struct v2_t
{
  float x, y;
} v2_t;

inline static bool v2_eq(v2_t a, v2_t b)
{
  return a.x == b.x && a.y == b.y;
}

typedef struct v3_t
{
  float x, y, z;
} v3_t;

inline static v3_t v3_cross(v3_t a, v3_t b)
{
  return (v3_t){
    a.y * b.z - a.z * b.y, 
    a.z * b.x - a.x * b.z, 
    a.x * b.y - a.y * b.x
  };
}

inline static v3_t v3_mul(v3_t a, float b) 
{
  return (v3_t){a.x * b, a.y * b, a.z * b};
}

inline static v3_t v3_add(v3_t a, v3_t b)
{
  return (v3_t){a.x+b.x, a.y+b.y, a.z+b.z};
}

inline static v3_t v3_sub(v3_t a, v3_t b)
{
  return (v3_t){a.x-b.x, a.y-b.y, a.z-b.z};
}

inline static v3_t v3_lerp(v3_t a, v3_t b, float c)
{
  return v3_add(a, v3_mul(v3_sub(b, a), c));
}

inline static void v3_normalize(v3_t *a)
{
  float length = em(sqrt, a->x*a->x + a->y*a->y + a->z*a->z);
  if (length == 0) return;

  a->x /= length;
  a->y /= length;
  a->z /= length;
}

inline static float v3_dot(v3_t a, v3_t b)
{
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

typedef union
{
  struct {
    float x, y, z, w;
  };

  float elem[4];
  v3_t xyz;
} v4_t;

typedef union
{
  struct {
    float a00, a01, a02, a03,
          a10, a11, a12, a13,
          a20, a21, a22, a23,
          a30, a31, a32, a33;
  };

  v4_t row[4];
} m4_t;

inline static v4_t v4_mul(v4_t a, float b) 
{
  return (v4_t){a.x * b, a.y * b, a.z * b, a.w * b};
}

inline static float v4_dot(v4_t a, v4_t b)
{
  return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

inline static v2_t get_window_size()
{
  int canvas_width, canvas_height;
  emscripten_get_canvas_element_size("canvas", &canvas_width, &canvas_height);

  return (v2_t){(float)canvas_width, (float)canvas_height};
}

inline static m4_t create_perspective_matrix(
    // float aspect, (calculated in this function via emscripten_get_canvas_element_size)
    float fovy,
    float near,
    float far)
{
  v2_t window_size = get_window_size();

  float aspect = window_size.x / window_size.y;

  float f = 1.f / em(tan, fovy * .5f);
  float fn = 1.f / (near - far);

  return (m4_t){
    f / aspect, 0, 0, 0,
    0, f, 0, 0,
    0, 0, (near + far) * fn, 2 * near * far * fn,
    0, 0, -1, 0
  };
}

typedef struct cam_t
{
  m4_t view, proj;
  v3_t pos, front, right, up;
  float yaw, pitch;
} cam_t;

inline static void cam_recalculate_matrix(cam_t *cam)
{
  static v3_t const world_up = {0, 1, 0};
  v3_t pos = cam->pos;
  float yaw = cam->yaw;
  float pitch = cam->pitch;

  v3_t front = {
    em(cos, yaw) * em(cos, pitch),
    em(sin, pitch),
    em(sin, yaw) * em(cos, pitch)
  };

  v3_normalize(&front);

  v3_t right = v3_cross(front, world_up);
  v3_normalize(&right);

  v3_t up = v3_cross(right, front);

  cam->front = front;
  cam->right = right;
  cam->up = up;

  // cam->view = (m4_t){
  //   right.x, up.x, -front.x, 0,
  //   right.y, up.y, -front.y, 0,
  //   right.z, up.z, -front.z, 0,
  //   -v3_dot(pos, right), -v3_dot(pos, up), v3_dot(pos, front), 1
  // };

  cam->view = (m4_t){
    right.x, right.y, right.z, -v3_dot(pos, right),
    up.x, up.y, up.z, -v3_dot(pos, up),
    -front.x, -front.y, -front.z, v3_dot(pos, front),
    0, 0, 0, 1
  };
}

inline static void m4_translate(m4_t *m, v3_t pos)
{
  m->a03 += pos.x; //v3_dot(m->row[0].xyz, pos);
  m->a13 += pos.y; //v3_dot(m->row[1].xyz, pos);
  m->a23 += pos.z; //v3_dot(m->row[2].xyz, pos);
}

inline static void m4_scale(m4_t *m, float scale)
{
  m->row[0] = v4_mul(m->row[0], scale);
  m->row[1] = v4_mul(m->row[1], scale);
  m->row[2] = v4_mul(m->row[2], scale);
}

inline static v4_t m4_col(m4_t *m, int n)
{
  return (v4_t){m->row[0].elem[n], m->row[1].elem[n], m->row[2].elem[n], m->row[3].elem[n]};
}

inline static void m4_rotate_x(m4_t *m, float angle)
{
  // multiply by the following matrix:
  // 1 0 0 0
  // 0 c s 0
  // 0 -s c 0
  // 0 0 0 1

  float c = em(cos, angle), s = em(sin, angle);
  v4_t r1 = {0, c, s, 0}, r2 = {0, -s, c, 0};

  m->a10 = v4_dot(r1, m4_col(m, 0));
  m->a11 = v4_dot(r1, m4_col(m, 1));
  m->a12 = v4_dot(r1, m4_col(m, 2));
  m->a13 = v4_dot(r1, m4_col(m, 3));
  m->a20 = v4_dot(r2, m4_col(m, 0));
  m->a21 = v4_dot(r2, m4_col(m, 1));
  m->a22 = v4_dot(r2, m4_col(m, 2));
  m->a23 = v4_dot(r2, m4_col(m, 3));
}

inline static void m4_rotate_y(m4_t *m, float angle)
{
  // multiply by the following matrix:
  // s 0 c 0
  // 0 1 0 0
  // c 0 -s 0
  // 0 0 0 1

  float c = em(cos, angle), s = em(sin, angle);
  v4_t r0 = {s, 0, c, 0}, r2 = {c, 0, -s, 0};

  m->a00 = v4_dot(r0, m4_col(m, 0));
  m->a01 = v4_dot(r0, m4_col(m, 1));
  m->a02 = v4_dot(r0, m4_col(m, 2));
  m->a03 = v4_dot(r0, m4_col(m, 3));
  m->a20 = v4_dot(r2, m4_col(m, 0));
  m->a21 = v4_dot(r2, m4_col(m, 1));
  m->a22 = v4_dot(r2, m4_col(m, 2));
  m->a23 = v4_dot(r2, m4_col(m, 3));
}

typedef struct {
  uint32_t a;
} xorshift32_state_t;

// The state must be initialized to non-zero
inline static uint32_t xorshift32(xorshift32_state_t* state) {
	// Algorithm "xor" from p. 4 of Marsaglia, "Xorshift RNGs"
	uint32_t x = state->a;
	x ^= x << 13;
	x ^= x >> 17;
	x ^= x << 5;
	return state->a = x;
}
