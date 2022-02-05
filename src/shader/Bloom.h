#pragma once


namespace shader
{

const static std::string passthrough_vs = std::string(
"\
#version 410\n\
layout(location = 0) in vec3 position;\n\
layout(location = 1) in vec2 texcoord;\n\
out vec2 uvcoordsvar;\n\
void main() {;\n\
	uvcoordsvar = vec2(texcoord.x, texcoord.y);\n\
	gl_Position = vec4(position.xyz, 1);\n\
}");


const static std::string bloom_fs = std::string(
"\
uniform sampler2D sourceBuffer; /* Buffer to filter */\
uniform vec2 sourceBufferTexelSize;\
\
/* Step Blit */\
uniform vec4 curveThreshold;\
uniform float clampIntensity;\
\
/* Step Upsample */\
uniform sampler2D baseBuffer; /* Previous accumulation buffer */\
uniform vec2 baseBufferTexelSize;\
uniform float sampleScale;\
\
/* Step Resolve */\
uniform vec3 bloomColor;\
uniform bool bloomAddBase;\
\
in vec2 uvcoordsvar;\
\
out vec4 FragColor;\
\
float max_v3(vec3 v) { return max(v.x, max(v.y, v.z)); }\
/* -------------- Utils ------------- */\
\
vec3 safe_color(vec3 c)\
{\
	/* Clamp to avoid black square artifacts if a pixel goes NaN. */\
	return clamp(c, vec3(0.0), vec3(1e20)); /* 1e20 arbitrary. */\
}\
\
/* 3-tap median filter */\
vec3 median(vec3 a, vec3 b, vec3 c)\
{\
	return a + b + c - min(min(a, b), c) - max(max(a, b), c);\
}\
/* ------------- Filters ------------ */\
\
vec3 downsample_filter_high(sampler2D tex, vec2 uv, vec2 texelSize)\
{\
	/* Downsample with a 4x4 box filter + anti-flicker filter */\
	vec4 d = texelSize.xyxy * vec4(-1, -1, +1, +1);\
\
	vec3 s1 = textureLod(tex, uv + d.xy, 0.0).rgb;\
	vec3 s2 = textureLod(tex, uv + d.zy, 0.0).rgb;\
	vec3 s3 = textureLod(tex, uv + d.xw, 0.0).rgb;\
	vec3 s4 = textureLod(tex, uv + d.zw, 0.0).rgb;\
\
	/* Karis's luma weighted average (using brightness instead of luma) */\
	float s1w = 1.0 / (max_v3(s1) + 1.0);\
	float s2w = 1.0 / (max_v3(s2) + 1.0);\
	float s3w = 1.0 / (max_v3(s3) + 1.0);\
	float s4w = 1.0 / (max_v3(s4) + 1.0);\
	float one_div_wsum = 1.0 / (s1w + s2w + s3w + s4w);\
\
	return (s1 * s1w + s2 * s2w + s3 * s3w + s4 * s4w) * one_div_wsum;\
}\
\
vec3 downsample_filter(sampler2D tex, vec2 uv, vec2 texelSize)\
{\
	/* Downsample with a 4x4 box filter */\
	vec4 d = texelSize.xyxy * vec4(-1, -1, +1, +1);\
\
	vec3 s;\
	s = textureLod(tex, uv + d.xy, 0.0).rgb;\
	s += textureLod(tex, uv + d.zy, 0.0).rgb;\
	s += textureLod(tex, uv + d.xw, 0.0).rgb;\
	s += textureLod(tex, uv + d.zw, 0.0).rgb;\
\
	return s * (1.0 / 4);\
}\
\
vec3 upsample_filter_high(sampler2D tex, vec2 uv, vec2 texelSize)\
{\
	/* 9-tap bilinear upsampler (tent filter) */\
	vec4 d = texelSize.xyxy * vec4(1, 1, -1, 0) * sampleScale;\
\
	vec3 s;\
	s = textureLod(tex, uv - d.xy, 0.0).rgb;\
	s += textureLod(tex, uv - d.wy, 0.0).rgb * 2;\
	s += textureLod(tex, uv - d.zy, 0.0).rgb;\
\
	s += textureLod(tex, uv + d.zw, 0.0).rgb * 2;\
	s += textureLod(tex, uv, 0.0).rgb * 4;\
	s += textureLod(tex, uv + d.xw, 0.0).rgb * 2;\
\
	s += textureLod(tex, uv + d.zy, 0.0).rgb;\
	s += textureLod(tex, uv + d.wy, 0.0).rgb * 2;\
	s += textureLod(tex, uv + d.xy, 0.0).rgb;\
\
	return s * (1.0 / 16.0);\
}\
\
vec3 upsample_filter(sampler2D tex, vec2 uv, vec2 texelSize)\
{\
	/* 4-tap bilinear upsampler */\
	vec4 d = texelSize.xyxy * vec4(-1, -1, +1, +1) * (sampleScale * 0.5);\
\
	vec3 s;\
	s = textureLod(tex, uv + d.xy, 0.0).rgb;\
	s += textureLod(tex, uv + d.zy, 0.0).rgb;\
	s += textureLod(tex, uv + d.xw, 0.0).rgb;\
	s += textureLod(tex, uv + d.zw, 0.0).rgb;\
\
	return s * (1.0 / 4.0);\
}\
\
/* ----------- Steps ----------- */\
\
vec4 step_blit(void)\
{\
	vec2 uv = uvcoordsvar.xy + sourceBufferTexelSize.xy * 0.5;\
\n\
#ifdef HIGH_QUALITY /* Anti flicker */\n\
	vec3 d = sourceBufferTexelSize.xyx * vec3(1, 1, 0);\
	vec3 s0 = safe_color(textureLod(sourceBuffer, uvcoordsvar.xy, 0.0).rgb);\
	vec3 s1 = safe_color(textureLod(sourceBuffer, uvcoordsvar.xy - d.xz, 0.0).rgb);\
	vec3 s2 = safe_color(textureLod(sourceBuffer, uvcoordsvar.xy + d.xz, 0.0).rgb);\
	vec3 s3 = safe_color(textureLod(sourceBuffer, uvcoordsvar.xy - d.zy, 0.0).rgb);\
	vec3 s4 = safe_color(textureLod(sourceBuffer, uvcoordsvar.xy + d.zy, 0.0).rgb);\
	vec3 m = median(median(s0.rgb, s1, s2), s3, s4);\n\
#else\n\
	vec3 s0 = safe_color(textureLod(sourceBuffer, uvcoordsvar.xy, 0.0).rgb);\
	vec3 m = s0.rgb;\n\
#endif\n\
\n\
	/* Pixel brightness */\
	float br = max_v3(m);\
\
	/* Under-threshold part: quadratic curve */\
	float rq = clamp(br - curveThreshold.x, 0, curveThreshold.y);\
	rq = curveThreshold.z * rq * rq;\
\
	/* Combine and apply the brightness response curve. */\
	m *= max(rq, br - curveThreshold.w) / max(1e-5, br);\
\
	/* Clamp pixel intensity if clamping enabled */\
	if (clampIntensity > 0.0) {\
		br = max(1e-5, max_v3(m));\
		m *= 1.0 - max(0.0, br - clampIntensity) / br;\
	}\
\
	return vec4(m, 1.0);\
}\
\
vec4 step_downsample(void)\
{\n\
#ifdef HIGH_QUALITY /* Anti flicker */\n\
	vec3 samp = downsample_filter_high(sourceBuffer, uvcoordsvar.xy, sourceBufferTexelSize);\n\
#else\n\
	vec3 samp = downsample_filter(sourceBuffer, uvcoordsvar.xy, sourceBufferTexelSize);\n\
#endif\n\
	return vec4(samp, 1.0);\
}\
\
vec4 step_upsample(void)\
{\n\
#ifdef HIGH_QUALITY\n\
	vec3 blur = upsample_filter_high(sourceBuffer, uvcoordsvar.xy, sourceBufferTexelSize);\n\
#else\n\
	vec3 blur = upsample_filter(sourceBuffer, uvcoordsvar.xy, sourceBufferTexelSize);\n\
#endif\n\
	vec3 base = textureLod(baseBuffer, uvcoordsvar.xy, 0.0).rgb;\n\
	return vec4(base + blur, 1.0);\
}\
\
vec4 step_resolve(void)\
{\n\
#ifdef HIGH_QUALITY\n\
	vec3 blur = upsample_filter_high(sourceBuffer, uvcoordsvar.xy, sourceBufferTexelSize);\n\
#else\n\
	vec3 blur = upsample_filter(sourceBuffer, uvcoordsvar.xy, sourceBufferTexelSize);\n\
#endif\n\
	vec4 base = bloomAddBase ? textureLod(baseBuffer, uvcoordsvar.xy, 0.0) : vec4(0.0);\n\
	vec3 cout = base.rgb + blur * bloomColor;\
	return vec4(cout, base.a);\
}\
void main(void)\n\
{\n\
#if defined(STEP_BLIT)\n\
	FragColor = step_blit();\n\
#elif defined(STEP_DOWNSAMPLE)\n\
	FragColor = step_downsample();\n\
#elif defined(STEP_UPSAMPLE)\n\
	FragColor = step_upsample();\n\
#elif defined(STEP_RESOLVE)\n\
	FragColor = step_resolve();\n\
#endif\n\
}\
");
}