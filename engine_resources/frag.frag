#version 430 core
#define PI 3.14159265359

out vec4 FragColor;
in vec2 vPosition;

uniform int maxits;
uniform float aspec;
uniform float time;
uniform float sphereRadius;
uniform float thresh;
uniform float smoothing;
uniform float shadowK;
uniform float modulo;
uniform mat4 matrix;
uniform vec4 matColor;
uniform vec3 skyColor;
uniform vec3 orientation;
uniform vec3 sphereloc;
uniform vec3 boxBounds;
uniform vec3 cameraPos;
uniform vec3 boxPos;
uniform vec3 lightPos;

struct Material {
	vec3 albedo;
	float roughness;
	float metalic;
	float absorb;
};

struct Sphere {
	vec3 position;
	float radius;
};

float atan2(in float y, in float x) {
    bool s = (abs(x) > abs(y));
    return mix(PI/2.0 - atan(x,y), atan(y,x), s);
}

float smin(float a, float b, float k) {
	float h = clamp( 0.5 + 0.5*(b-a)/k, 0.0, 1.0 );
	return mix( b, a, h ) - k*h*(1.0-h);
}

float smax(float a, float b, float k) {
    return smin(a, b, -k);
}

float sdPlane(vec3 position, float height) {
	return abs(position.y - height);
}

float sdSphere(vec3 position, vec3 centre, float radius) {
	return length(position-centre) - radius;
}

float sdBox(vec3 position, vec3 origin, vec3 bound) {
	 vec3 d = abs(position-origin) - bound;
	 return min(max(d.x,max(d.y,d.z)),0.0) + length(max(d,0.0));
}


 float map(vec3 point){
	float m = smin( sdSphere(point,sphereloc,sphereRadius), sdBox(point,boxPos,boxBounds) , smoothing);
	m = min(m, sdPlane(point,0));	
	return min(m, sdSphere(point,lightPos,0.1));
 }


 float ShadowMap(vec3 point){
	float m = smin( sdSphere(point,sphereloc,sphereRadius), sdBox(point,boxPos,boxBounds) , smoothing);
	return min(m, sdPlane(point,0));	
 }
 
vec3 getNormal(vec3 p, float offset) {
    vec3 smallStep = vec3(offset, 0.0, 0.0);
    float gradientX = map(p + smallStep.xyy) - map(p - smallStep.xyy);
    float gradientY = map(p + smallStep.yxy) - map(p - smallStep.yxy);
    float gradientZ = map(p + smallStep.yyx) - map(p - smallStep.yyx);
    vec3 normal = vec3(gradientX, gradientY, gradientZ);
    return normalize(normal);
}

int raymarch(in out vec3 origin, vec3 direction) {

	int i = 0;
	float m = 0.0;
	for( i; i<=maxits; i++) {
 
		m = map(origin);
		origin += direction*m;
		if(m < thresh)
			return i;
		else if(m > 1000)
			break;
		
	} 
	return -1;
}

float softShadows( vec3 origin, vec3 rd){

	float res = 1.0;
	float ph = 1e20;
	float t = 0.05;
	
	for(int i=0; i<=maxits && t < 500.0; i++) {
 
		float h = ShadowMap(origin + rd * t);
		
		if(h < 0.001)
			return 0.0;
		
		float y = h*h / (2.0 * ph);
		float d = sqrt(h*h - y*y);
		res = min( res, shadowK* d / (max(0.0,t-y) ) );
		ph = h;
		t += h;
	} 
	
	return res;
}

float hardShadows( vec3 origin, vec3 rd){

	float res = 1.0;
	float t = 0.1;
	
	for(int i=0; i<=maxits && t < 500.0; i++) {
 
		float h = ShadowMap(origin + rd * t);
		
		if(h< 0.001)
			return 0.0;
		t += h;
	} 
	
	return 1.0;
}

void main() {
	vec3 rayOrigin = cameraPos;
	vec2 coord = vec2(vPosition.x*aspec,vPosition.y);
	vec3 rayDirection = (matrix * vec4(normalize(vec3(coord,-1)),0)).xyz;
	int hitSky = raymarch(rayOrigin, rayDirection.xyz);
	vec3 normal = getNormal(rayOrigin,0.001f);
	
	float lightIntensity = dot(normal,normalize(lightPos-rayOrigin));
	float shadow = max(0.0,softShadows(rayOrigin, normalize(lightPos - rayOrigin)));
	

	if(hitSky != -1)
		FragColor = vec4(matColor*lightIntensity*shadow);
	else
		FragColor = vec4(skyColor,1.0);
}