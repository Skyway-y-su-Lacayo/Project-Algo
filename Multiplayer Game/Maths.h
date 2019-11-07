#pragma once



////////////////////////////////////////////////////////////////////////
// DEFINITIONS
////////////////////////////////////////////////////////////////////////

#ifdef PI
#undef PI
#endif
#define PI 3.14159265359f



////////////////////////////////////////////////////////////////////////
// FUNCTIONS
////////////////////////////////////////////////////////////////////////

inline float radiansFromDegrees(float degrees)
{
	const float radians = PI * degrees / 180.0f;
	return radians;
}

inline float fractionalPart(float number)
{
	const float f = number - (int)number;
	return f;
}



////////////////////////////////////////////////////////////////////////
// VEC2
////////////////////////////////////////////////////////////////////////

struct vec2
{
	union
	{
		struct { float x; float y; };
		struct { float u; float v; };
		float coords[2] = {};
	};
};

vec2 operator+(vec2 a, vec2 b) { return vec2{ a.x + b.x, a.y + b.y }; }
vec2 operator-(vec2 a, vec2 b) { return vec2{ a.x - b.x, a.y - b.y }; }
vec2 operator*(vec2 a, vec2 b) { return vec2{ a.x * b.x, a.y * b.y }; }
vec2 operator*(vec2 a, float b) { return vec2{ a.x * b, a.y * b }; }
vec2 operator*(float a, vec2 b) { return b * a; }
vec2 operator/(vec2 a, vec2 b) { return vec2{ a.x / b.x, a.y / b.y }; }
vec2 operator/(vec2 a, float b) { return vec2{ a.x / b, a.y / b }; }
vec2 operator/(float a, vec2 b) { return vec2{ a / b.x, a / b.y }; }
vec2 &operator+=(vec2 &a, vec2 b) { a = a + b; return a; }
vec2 &operator-=(vec2 &a, vec2 b) { a = a - b; return a; }
vec2 &operator*=(vec2 &a, float b) { a = a * b; return a; }
vec2 &operator/=(vec2 &a, float b) { a = a / b; return a; }
vec2 floor(vec2 a) { return vec2{ floorf(a.x), floorf(a.y) }; }
vec2 ceil(vec2 a) { return vec2{ ceilf(a.x), ceilf(a.y) }; }
float dot(vec2 a, vec2 b) { return a.x * b.x + a.y * b.y; }
float length2(vec2 a) { return dot(a, a); }
float length(vec2 a) { return sqrtf(length2(a)); }
vec2 normalize(vec2 a) { return a / length(a); }
bool isZero(vec2 a) { return dot(a, a) < FLT_EPSILON; }

vec2 vec2FromDegrees(float degrees) {
	vec2 result = vec2{
		sinf(radiansFromDegrees(degrees)),
		-cosf(radiansFromDegrees(degrees))
	};
	return result;
}



////////////////////////////////////////////////////////////////////////
// VEC4
////////////////////////////////////////////////////////////////////////

struct vec4
{
	union
	{
		struct { float x; float y; float z; float w; };
		struct { float r; float g; float b; float a; };
		float coords[4] = {};
	};
};

vec4 operator+(vec4 a, vec4 b) { return vec4{ a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w }; }
vec4 operator-(vec4 a, vec4 b) { return vec4{ a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w }; }
vec4 operator*(vec4 a, vec4 b) { return vec4{ a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w }; }
vec4 operator*(vec4 a, float b) { return vec4{ a.x * b, a.y * b, a.z * b, a.w * b }; }
vec4 operator*(float a, vec4 b) { return b * a; }
vec4 operator/(vec4 a, vec4 b) { return vec4{ a.x / b.x, a.y / b.y, a.z / b.z, a.w / b.w }; }
vec4 operator/(vec4 a, float b) { return vec4{ a.x / b, a.y / b, a.z / b, a.w / b }; }
vec4 operator/(float a, vec4 b) { return vec4{ a / b.x, a / b.y, a / b.z, a / b.w }; }
vec4 &operator*=(vec4 &a, float b) { a = a * b; return a; }
vec4 &operator/=(vec4 &a, float b) { a = a / b; return a; }
vec4 lerp(vec4 a, vec4 b, float t) { vec4 c = a + t * (b - a); return c; }
float dot(vec4 a, vec4 b) { return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w; }
bool isZero(vec4 a) { return dot(a, a) < FLT_EPSILON; }



////////////////////////////////////////////////////////////////////////
// MAT4
////////////////////////////////////////////////////////////////////////

struct mat4
{
	union
	{
		struct
		{
			vec4 v0;
			vec4 v1;
			vec4 v2;
			vec4 v3;
		};
		float data[4][4];
	};
};

mat4 identity()
{
	mat4 matrix = {0};
	matrix.v0.x = 1;
	matrix.v1.y = 1;
	matrix.v2.z = 1;
	matrix.v3.w = 1;
	return matrix;
}

mat4 translation(vec2 displacement)
{
	mat4 matrix = identity();
	matrix.v3.x = displacement.x;
	matrix.v3.y = displacement.y;
	return matrix;
}

mat4 rotationZ(float angle)
{
	mat4 matrix = identity();
	matrix.v0.x = cosf(angle);
	matrix.v0.y = sinf(angle);
	matrix.v1.x = -sinf(angle);
	matrix.v1.y = cosf(angle);
	return matrix;
}

mat4 scaling(vec2 scale)
{
	mat4 matrix = identity();
	matrix.v0.x = scale.x;
	matrix.v1.y = scale.y;
	return matrix;
}

vec4 operator*(const mat4 &a, const vec4 &b)
{
	vec4 result;
	result.x = dot(vec4{ a.v0.x, a.v1.x, a.v2.x, a.v3.x }, b);
	result.y = dot(vec4{ a.v0.y, a.v1.y, a.v2.y, a.v3.y }, b);
	result.z = dot(vec4{ a.v0.z, a.v1.z, a.v2.z, a.v3.z }, b);
	result.w = dot(vec4{ a.v0.w, a.v1.w, a.v2.w, a.v3.w }, b);
	return result;
}

mat4 operator*(const mat4 &a, const mat4 &b)
{
	mat4 result = identity();
	result.v0 = a * b.v0;
	result.v1 = a * b.v1;
	result.v2 = a * b.v2;
	result.v3 = a * b.v3;
	return result;
}

vec2 vec2_cast(vec4 a)
{
	vec2 result = { a.x, a.y };
	return result;
}
