#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <time.h>

#include <iostream>
#include <cmath>
#include <string>
#include <vector>
#include <cstdlib>
#include <array>
using namespace std;
using vec2 = std::vector<double>;
using vec3 = std::vector<double>;


constexpr float PI = 3.14159265;
constexpr float angle = 60.0;
//constexpr float fov =  0.52432108931;  //angle * 0.5 * PI / 180.0;
constexpr float sfov = 0.5;  // sin(fov);
constexpr float cfov = 0.86602540378;  // cos(fov);
vec3  cPos = vec3{0.0, 0.0, 2.0};
const float sphereSize = 0.4;
const vec3 lightDir = vec3{-0.577, 0.577, -0.577};
const int resolution = 1000;  // 1000 x 1000

//
// Vec3
//

vec3 operator+(vec3 a, vec3 b){
	if(a.size() == 2)
		return {a[0]+b[0], a[1]+b[1]};
	return {a[0]+b[0], a[1]+b[1], a[2]+b[2]};
}

vec3 operator*(vec3 a, float b){
	if(a.size() == 2)
		return {a[0]*b, a[1]*b};
	return {a[0]*b, a[1]*b, a[2]*b};
}
vec3 operator/(vec3 a, float b){
	if(a.size() == 2)
		return {a[0]/b, a[1]/b};
	return {a[0]/b, a[1]/b, a[2]/b};
}

vec3 normalize(vec3 x){
	float l = sqrt(x[0]*x[0]+x[1]*x[1]+x[2]*x[2]);
	if(l == 0)
		return {1., 1., 1.};
	return x / l;
}

//
// Distance Functions
//

array<float, 2> onRep(float x, float y, float interval){
	x = fmod(x + interval*0.5, interval) -  interval*0.5;
	y = fmod(y + interval*0.5, interval) -  interval*0.5;
	return {x, y};
}

float tubeDist(float x, float y, float interval, float width){
	auto[xr, yr] = onRep(x, y, interval);
	return sqrt(xr * xr + yr * yr) - width;
}

float distanceFunc1(vec3 p){
	float tube_x = tubeDist(p[1], p[2], sphereSize*1.5, sphereSize*0.15);
	float tube_y = tubeDist(p[2], p[0], sphereSize*1.5, sphereSize*0.15);
	float tube_z = tubeDist(p[0], p[1], sphereSize*1.5, sphereSize*0.15);
	return min(min(tube_x, tube_y), tube_z);
}

float distanceFunc3(vec3 p){
	float tube_x = tubeDist(p[1], p[2], sphereSize*0.25, sphereSize*0.05);
	float tube_y = tubeDist(p[2], p[0], sphereSize*0.25, sphereSize*0.05);
	float tube_z = tubeDist(p[0], p[1], sphereSize*0.25, sphereSize*0.05);
	return min(min(tube_x, tube_y), tube_z);
}

float distanceFunc(vec3 p){
	p[0] += sphereSize * 0.6;
	p[1] += sphereSize * 0.2;
	return max(distanceFunc1(p), -distanceFunc3(p));
}

//
// Partial Derivative
//

vec3 getNormal(vec3 p){
    float d = 1./256;
    return normalize(vec3{
        distanceFunc(p + vec3{  d, 0.0, 0.0}) - distanceFunc(p + vec3{ -d, 0.0, 0.0}),
        distanceFunc(p + vec3{0.0,   d, 0.0}) - distanceFunc(p + vec3{0.0,  -d, 0.0}),
        distanceFunc(p + vec3{0.0, 0.0,   d}) - distanceFunc(p + vec3{0.0, 0.0,  -d})
    });
}

//
// Fragment Shader
//

vec3 getPx(float x, float y){
	vec2 xy = {x, y};
	vec3 color = {0, 0, 0};
	vec2 p = (xy * 2) / resolution + vec2{-1, -1};
    vec3 ray = normalize(vec3{sfov * p[0], sfov * p[1], cfov});

    float distance = 0.0;
    float rLen = 0.0;
    vec3  rPos = cPos;
    for(int i = 0; i < 32; i++){
        distance = distanceFunc(rPos);
        rLen += distance;
        rPos = cPos + ray * rLen;
    }

    if(abs(distance) < 1./256){
        vec3 normal = getNormal(rPos);
		float diff = 0;
		for(int i=0; i<3; ++i)
			diff = diff + lightDir[i] * normal[i];
		if(diff > 1) diff = 1;
		if(diff < 0.1) diff = 0.1;
		rLen *= 0.35;
		if(rLen > 1) rLen = 1;
		color = (vec3{0.7, 1, 0.4} * (1-rLen) + vec3{0.4, 0.7, 1} * rLen) * diff;
        //color = vec3{diff * 0.7, diff, diff * 0.4};
    }
	return color;
}

//
// Bitmap
//

void fputc2LowHigh(unsigned short d, FILE *s){
	putc(d & 0xFF, s);
	d >>= CHAR_BIT;
	putc(d & 0xFF, s);
}

void fputc4LowHigh(unsigned long d, FILE *s){
	putc(d & 0xFF, s);
	d >>= CHAR_BIT;
	putc(d & 0xFF, s);
	d >>= CHAR_BIT;
	putc(d & 0xFF, s);
	d >>= CHAR_BIT;
	putc(d & 0xFF, s);
}

void createPic(unsigned char *b, int x, int y){
	int i;
	int j;

	for (i=0;i<y;i++) {
		for (j=0;j<x;j++) {
			vec3 px = getPx(resolution- i + 1, resolution - j + 1);
			*b++ = px[0]*256;
			*b++ = px[1]*256;
			*b++ = px[2]*256;
		}
	}
}

void putBmpHeader(FILE *s){
	int color = 0;
	unsigned long int bfOffBits = 54;
	fputs("BM", s);
	fputc4LowHigh(bfOffBits + resolution * resolution, s);
	fputc2LowHigh(0, s);
	fputc2LowHigh(0, s);
	fputc4LowHigh(bfOffBits, s);
	fputc4LowHigh(40, s);
	fputc4LowHigh(resolution, s);
	fputc4LowHigh(resolution, s);
	fputc2LowHigh(1, s);
	fputc2LowHigh(24, s);
	fputc4LowHigh(0, s);
	fputc4LowHigh(0, s);
	fputc4LowHigh(0, s);
	fputc4LowHigh(0, s);
	fputc4LowHigh(0, s);
	fputc4LowHigh(0, s);
}

int main(int argc, char *argv[]){
	FILE *f;
	int r;
	unsigned char *b;
	int picSize =  (resolution * 3 * resolution);
	b = (unsigned char*)malloc(picSize);
	createPic(b, resolution, resolution);
	f = fopen("test.bmp", "wb");
	putBmpHeader(f);
	fwrite(b, sizeof(unsigned char), picSize, f);
	fclose(f);
	return 0;
}
