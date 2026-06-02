#pragma once

static char const *vertex_shader_source = 
"struct in_t {\n"
"  @location(0) pos : vec3<f32>,\n"
"  @location(1) uv : vec2<f32>\n"
"};\n"

"struct out_t {\n"
"  @builtin(position) pos : vec4<f32>,\n"
"  @location(0) uv : vec2<f32>,\n"
"  @location(1) z : f32,\n"
"};\n"

"struct camera_t {\n"
"  view : mat4x4f,\n"
"  proj : mat4x4f,\n"
"};\n"

"@group(0) @binding(0) var<storage> camera : camera_t;\n"
"@group(1) @binding(2) var<storage> model : mat4x4f;\n"

"@vertex\n"
"fn main(in : in_t) -> out_t {\n"
"  var out : out_t;\n"
// "  let pos = model * vec4<f32>(in.pos, 1.0);\n"
// "  out.pos = camera.proj * camera.view * pos;\n"
"  let pos = vec4<f32>(in.pos, 1.0) * model;\n"
"  out.pos = pos * camera.view * camera.proj;\n"
"  out.uv = in.uv;\n"
"  out.z = pos.z;\n"
"  return out;\n"
"}\n";

static char const *fragment_shader_source =
"@group(0) @binding(1) var<storage> opacity : f32;\n"

"@group(1) @binding(0) var b_texture : texture_2d<f32>;\n"
"@group(1) @binding(1) var b_sampler : sampler;\n"

"@fragment\n"
"fn main(@location(0) uv : vec2<f32>, @location(1) z : f32) -> @location(0) vec4<f32> {\n"
"  var out_color : vec4<f32> = select(textureSample(b_texture, b_sampler, vec2<f32>(uv.x, uv.y)), vec4<f32>(0., 0., 0., 1.), uv.x < 0 && uv.y < 0);\n"
"  out_color.a = opacity;\n"  
"  return out_color;\n"
"}\n";

static char const *post_process_vertex_shader_source = 
"struct out_t {\n"
"  @builtin(position) pos : vec4f,\n"
"  @location(0) uv : vec2f,\n"
"};\n"

"@vertex\n"
"fn main(@location(0) pos : vec2f) -> out_t {\n"
"  var out : out_t;\n"
"  out.pos = vec4f(pos, 0., 1.);\n"
"  out.uv = pos * 0.5 + 0.5;\n"
"  return out;\n"
"}\n";

#define post_process_fragment_shader_source_prelude \
"struct post_input_t {\n" \
"  view : mat4x4f,\n" \
"  one_texel : vec2f,\n" \
"  res : vec2f,\n" \
"  time : f32,\n" \
"  opac : f32,\n" \
"};\n" \
"@group(0) @binding(0) var<storage> uni : post_input_t;\n"

static char const *stars_fragment_shader_source =
post_process_fragment_shader_source_prelude

"fn cosg(t : f32) -> vec3f {\n"
"  let a = vec3f(0.938, 0.328, 0.718);\n"
"  let b = vec3f(0.659, 0.438, 0.328);\n"
"  let c = vec3f(0.388, 0.388, 0.296);\n"
"  let d = vec3f(2.538, 2.478, 0.168);\n"
"  return clamp(a + b * cos(2. * 3.1415926 * (c * t + d)), vec3f(0.), vec3f(1.));\n"
"}\n"

"fn hash43x(p : vec3f) -> vec4f {\n"
"  var x = vec3u(vec3i(p));\n"
"  x = 1103515245u * ((x.xyz >> vec3u(1u)) ^ x.yzx);\n"
"  let h = 1103515245u * ((x.x ^ x.z) ^ (x.y >> 3u));\n"
"  let rz = vec4u(h, h*16807u, h*48271u, h*69621u);\n"
"  return vec4f((rz >> vec4u(1u)) & vec4u(0x7fffffffu)) / f32(0x7fffffff);\n"
"}\n"

"fn star(_rd : vec3f) -> vec3f {\n"
"  var accum = vec3f(0.);\n"
"  var rd = _rd;\n"
"  let n = 25.;\n"

"  for (var i = 0; i < 3; i++) {\n"
"    rd = mat3x3f(0.63322557,0.6840354,0.36210626,"
"                 0.41076351,-0.69354841,0.591831,"
"                 0.65597158,-0.22602248,-0.72014937) * rd;\n"
"    rd = normalize(rd);\n"
"    rd = -rd.zxy;\n"
"    rd = mat3x3f(0.93855724,0.19917015,0.28185379,"
"                 0.18209392,-0.97952923,0.08581546,"
"                 0.2931759,-0.02921886,-0.95561192) * rd;\n"

"    let r = abs(rd);\n"
"    let p = r / max(r.x, max(r.y, r.z));\n"
"    let a = 1. - step(vec3f(0.9999999), p);\n"
"    let s = p * a * sign(rd);\n"

"    let ip = floor(s * n);\n"
"    let fp = fract(s * n);\n"
"    let h = hash43x(ip + vec3f(vec3i(i) * 900));\n"

"    var g = (fp - .5) - (h.xyz * .6 - .3);\n"
"    g.x = mix(g.z, g.x, a.x);\n"
"    g.y = mix(g.z, g.y, a.y);\n"
"    g.z = 0.;\n"

"    let w = h.z * 2. * 3.1415926 + 0.5 * (sin(0.5 + uni.time + 40. * h.z) + cos(0.2 * uni.time - 2. + 40. * h.z));\n"
"    let _c = cos(w); let _s = sin(w);\n"
"    let g_tmp = mat2x2f(_c, _s, -_s, _c) * g.xy;\n"
"    g.x = g_tmp.x; g.y = g_tmp.y;\n"

"    let e = smoothstep(0.8, 1.2, 1. / min(max((200. + 50. * h.w) * abs(g.x * g.y), 0.01), 1.2));\n"

"    let b = pow((1. - length(g)) * 1.1 * h.y, 4.)\n"
"            * (sin(32. * h.x + uni.time) * .5 + .5)\n"
"            * pow(max(r.x, max(r.y, r.z)), 6.)\n"
"            * e;\n"

"    accum += cosg(max(b, 0.)) * max(b, 0.);\n"
"  }\n"

"  return accum;\n"
"}\n"

"@fragment\n"
"fn main(@location(0) _uv : vec2f) -> @location(0) vec4f {\n"
"  var uv = _uv * 2. - 1.;\n"
"  uv.x *= uni.res.x / uni.res.y;\n"
"  let fovy = 3.1415926 / 4.;\n"
"  let rd = uni.view * vec4f(normalize(vec3f(uv, -1. / tan(fovy))), 0.);\n"
"  return vec4f(star(rd.xyz), 1.);\n"
"}\n";

static char const *crt_fragment_shader_source =
post_process_fragment_shader_source_prelude
"@group(1) @binding(0) var b_texture : texture_2d<f32>;\n"
"@group(1) @binding(1) var b_sampler : sampler;\n"
"@fragment\n"
"fn main(@location(0) uv : vec2f) -> @location(0) vec4f {\n"
"  var crtUV = uv * 2. - 1.;\n"
"  let offset = crtUV.yx / 5.1;\n"
"  crtUV += crtUV * offset * offset;\n"
"  crtUV = crtUV * .5 + .5;\n"

"  let edge = smoothstep(vec2f(0.), vec2f(.012), crtUV) * (1. - smoothstep(vec2f(1. - .012), vec2f(1.), crtUV));\n"

"  let scan=.75*.5*abs(sin(uv.y * uni.res.y));\n"
"  var fragColor : vec3f;\n"
"  fragColor = vec3f(\n"
"    textureSample(b_texture, b_sampler, (crtUV-.5)*1.002+.5).r,\n"
"    textureSample(b_texture, b_sampler, crtUV).g,\n"
"    textureSample(b_texture, b_sampler, (crtUV-.5)/1.002+.5).b\n"
"  ) * edge.x * edge.y;\n"

"  return vec4f(fragColor * (1. - scan) * uni.opac, 1.);\n"
"}\n";

static char const *font_vertex_shader_source =
"struct in_t {\n"
"  @location(0) pos : vec3f,\n"
"  @location(1) tex : vec2f\n"
"};\n"

"struct out_t {\n"
"  @builtin(position) pos : vec4f,\n"
"  @location(0) tex : vec2f\n"
"};\n"

"struct camera_t {\n"
"  view : mat4x4f,\n"
"  proj : mat4x4f,\n"
"}\n"

"@group(0) @binding(0) var<storage> cam : camera_t;\n"
"@group(1) @binding(2) var<storage> model : mat4x4f;\n"

"@vertex\n"
"fn main(in : in_t) -> out_t {\n"
"  var out : out_t;\n"
"  out.pos = vec4f(in.pos, 1.) * cam.view * cam.proj;\n"
"  out.tex = in.tex;\n"
"  return out;\n"
"}\n";

static char const *font_fragment_shader_source =
"@group(1) @binding(0) var b_texture : texture_2d<f32>;\n"
"@group(1) @binding(1) var b_sampler : sampler;\n"

"@fragment\n"
"fn main(@location(0) tex : vec2f) -> @location(0) vec4f {\n"
"  let c = textureSample(b_texture, b_sampler, tex);\n"
"  if c.a < 0.01 { discard; }\n"
"  return c;\n"
"}\n";

