#pragma once

typedef
struct v2_t
{
  float x, y;
} v2_t;

typedef 
struct v3_t
{
  float x, y, z;
} v3_t;

v3_t
v3_cross(v3_t a, v3_t b)
{
  return (v3_t){
    a.y * b.z - a.z * b.y, 
    a.z * b.x - a.x * b.z, 
    a.x * b.y - a.y * b.x
  };
}

v3_t
v3_mul(v3_t a, float b) 
{
  return (v3_t){a.x * b, a.y * b, a.z * b};
}

v3_t
v3_add(v3_t a, v3_t b)
{
  return (v3_t){a.x+b.x, a.y+b.y, a.z+b.z};
}

void
v3_normalize(v3_t *a)
{
  float length = em(sqrt)(a->x*a->x + a->y*a->y + a->z*a->z);
  if (length == 0) return;

  a->x /= length;
  a->y /= length;
  a->z /= length;
}

float
v3_dot(v3_t a, v3_t b)
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

v4_t
v4_mul(v4_t a, float b) 
{
  return (v4_t){a.x * b, a.y * b, a.z * b, a.w * b};
}

float
v4_dot(v4_t a, v4_t b)
{
  return a.x*b.x+a.y*b.y+a.z*b.z+a.w*b.w;
}

m4_t
create_perspective_matrix(
    // float aspect, (calculated in this function via emscripten_get_canvas_element_size)
    float fovy,
    float near,
    float far)
{
  int canvas_width, canvas_height;
  emscripten_get_canvas_element_size("canvas", &canvas_width, &canvas_height);

  float aspect = (float)canvas_width / (float)canvas_height;

  float f = 1.f / tan(fovy * .5f);
  float fn = 1.f / (near - far);

  return (m4_t){
    f / aspect, 0, 0, 0,
    0, f, 0, 0,
    0, 0, (near + far) * fn, 2 * near * far * fn,
    0, 0, -1, 0
  };
}

typedef
struct camera_t
{
  m4_t view, proj;
  v3_t pos, front, right, up;
  float yaw, pitch;
} camera_t;

void
camera_recalculate_matrix(camera_t *camera)
{
  static v3_t const world_up = {0, 1, 0};
  v3_t pos = camera->pos;
  float yaw = camera->yaw;
  float pitch = camera->pitch;

  v3_t front = {
    em(cos)(yaw) * em(cos)(pitch),
    em(sin)(pitch),
    em(sin)(yaw) * em(cos)(pitch)
  };

  v3_normalize(&front);

  v3_t right = v3_cross(front, world_up);
  v3_t up = v3_cross(right, front);

  camera->front = front;
  camera->right = right;
  camera->up = up;

  // camera->view = (m4_t){
  //   right.x, up.x, -front.x, 0,
  //   right.y, up.y, -front.y, 0,
  //   right.z, up.z, -front.z, 0,
  //   -v3_dot(pos, right), -v3_dot(pos, up), v3_dot(pos, front), 1
  // };

  camera->view = (m4_t){
    right.x, right.y, right.z, -v3_dot(pos, right),
    up.x, up.y, up.z, -v3_dot(pos, up),
    -front.x, -front.y, -front.z, v3_dot(pos, front),
    0, 0, 0, 1
  };
}

camera_t camera;
// number of vertices per frame: 24 quads (frame) + 12 quads (weird cubes) + 1 quad (picture)
// 37 quads -> 37*6 = 222 vertices
typedef struct
picture_vertex_t
{
  v3_t pos;
  v2_t uv; // or -1
} picture_vertex_t;
picture_vertex_t cpu_vertex_buffer[n_textures][6];

float animation_start_time;
int current_index;
static float const animation_time = 800;
int requested_animation;

struct {
  float front, right, up, turn_up, turn_left;
} input;

bool
key_down_callback(
    int type,
    EmscriptenKeyboardEvent const *event,
    void *user_data)
{
  if (type != EMSCRIPTEN_EVENT_KEYDOWN) return false;

  if (!requested_animation) {
    if (strcmp(event->code, "ArrowLeft") == 0) {
      animation_start_time = emscripten_get_now();
      requested_animation = -1;
    } else if (strcmp(event->code, "ArrowRight") == 0) {
      animation_start_time = emscripten_get_now();
      requested_animation = 1;
    }
  }

  if (strcmp(event->code, "KeyW") == 0) input.front++;
  if (strcmp(event->code, "KeyA") == 0) input.right--;
  if (strcmp(event->code, "KeyS") == 0) input.front--;
  if (strcmp(event->code, "KeyD") == 0) input.right++;
  if (strcmp(event->code, "ShiftLeft") == 0) input.up--;
  if (strcmp(event->code, "Space") == 0) input.up++;
  if (strcmp(event->code, "KeyQ") == 0) input.turn_left++;
  if (strcmp(event->code, "KeyE") == 0) input.turn_left--;
  if (strcmp(event->code, "KeyR") == 0) input.turn_up++;
  if (strcmp(event->code, "KeyF") == 0) input.turn_up--;

  emscripten_mini_stdio_printf("key down: code=%s\n", event->code);
  return false;
}

float 
animation_get(float t)
{
  // ended up just approximating this
  // https://www.cssportal.com/css-cubic-bezier-generator/?x1=0.5&y1=-0.4&x2=0.5&y2=1.4
  // because I don't wanna deal with solving a cubic here.

  // return 0.558625*em(sin)(4.065545*x-2.01651032)-0.558625*em(sin)(-2.01651032);
#define it(x) (em(sin)((x) * M_PI - M_PI * .5)/2 + .5)
  float y = it(t);
  return em(sqrt)(it(y));
}

void
m4_translate(m4_t *m, v3_t pos)
{
  m->a03 += pos.x; //v3_dot(m->row[0].xyz, pos);
  m->a13 += pos.y; //v3_dot(m->row[1].xyz, pos);
  m->a23 += pos.z; //v3_dot(m->row[2].xyz, pos);
}

void
m4_scale(m4_t *m, float scale)
{
  m->row[0] = v4_mul(m->row[0], scale);
  m->row[1] = v4_mul(m->row[1], scale);
  m->row[2] = v4_mul(m->row[2], scale);
}

v4_t
m4_col(m4_t *m, int n)
{
  return (v4_t){m->row[0].elem[n], m->row[1].elem[n], m->row[2].elem[n], m->row[3].elem[n]};
}

void
m4_rotate_x(m4_t *m, float angle)
{
  // multiply by the following matrix:
  // 1 0 0 0
  // 0 c s 0
  // 0 -s c 0
  // 0 0 0 1

  float c = em(cos)(angle), s = em(sin)(angle);
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

void
m4_rotate_y(m4_t *m, float angle)
{
  // multiply by the following matrix:
  // s 0 c 0
  // 0 1 0 0
  // c 0 -s 0
  // 0 0 0 1

  float c = em(cos)(angle), s = em(sin)(angle);
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
