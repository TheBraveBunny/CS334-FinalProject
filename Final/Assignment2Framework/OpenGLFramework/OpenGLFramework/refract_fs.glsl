
#version 330
//Based on Anton's examples

in vec3 point;
in vec3 eye;
in vec2 coord;
in vec3 normal;

uniform int clicked;
uniform samplerCube cube_texture;
uniform sampler2D textureData;
uniform sampler2D normalData;
uniform mat4 V; //viewMatrix;

// fixed point light properties
vec3 light_position_world  = vec3 (0.0, 0.0, 2.0);
vec3 Ls = vec3 (1.0, 1.0, 1.0); // white specular colour
vec3 Ld = vec3 (0.7, 0.7, 0.7); // dull white diffuse light colour
vec3 La = vec3 (0.2, 0.2, 0.2); // grey ambient colour
  
// surface reflectance
vec3 Ks = vec3 (1.0, 1.0, 1.0); // fully reflect specular light
vec3 Kd = vec3 (1.0, 0.5, 0.0); // orange diffuse surface reflectance
vec3 Ka = vec3 (1.0, 1.0, 1.0); // fully reflect ambient light
float specular_exponent = 100.0; // specular 'power'


// Calculate the TBN framework using the normal vector of the fragment, 
// the point of the vertex and the texture coordinate of the fragment
// From http://gamedev.stackexchange.com/questions/86530/is-it-possible-to-calculate-the-tbn-matrix-in-the-fragment-shader
mat3 cotangent_frame( vec3 N, vec3 p, vec2 uv )
{
    // get edge vectors of the pixel triangle
    vec3 dp1 = dFdx( p );
    vec3 dp2 = dFdy( p );
    vec2 duv1 = dFdx( uv );
    vec2 duv2 = dFdy( uv );

    // solve the linear system
    vec3 dp2perp = cross( dp2, N );
    vec3 dp1perp = cross( N, dp1 );
    vec3 T = dp2perp * duv1.x + dp1perp * duv2.x;
    vec3 B = dp2perp * duv1.y + dp1perp * duv2.y;

    // construct a scale-invariant frame 
    float invmax = inversesqrt( max( dot(T,T), dot(B,B) ) );
    return mat3( T * invmax, B * invmax, N );
}



void main()
{
	// ambient intensity
	vec3 Ia = La * Ka;

	// diffuse intensity
	// raise light position to eye space
	vec3 light_position_eye = light_position_world;
	vec3 distance_to_light_eye = light_position_eye - eye;
	
	
	
	// NORMAL MAPPING STEPS:
	// 1. Get the Light source vector
	vec3 L = normalize (distance_to_light_eye);
	
	// 2. Get the normal vector from the normal map and rescale from [0,1] to [-1,1]
	vec3 Ntex = vec3(texture(normalData, coord));
	vec3 N = Ntex * 2.0 - 1.0;
	N = normalize(N);
	
	// 3. Get the TBN matrix
	// Note: eye is the point but already transformed with the model matrix
	mat3 TBN = cotangent_frame(N, eye, coord);
	
	// 4. Update N and L vectors using the TBN matrix
	N = TBN * N;
	L = TBN * L;
	
	// 5. HERE OCCURS THE NORMAL MAPPING!!!!!
	float dot_prod = dot (L, N);
	
	// The rest of the code is just light configuration
	
	
	
	dot_prod = max (dot_prod, 0.0);
	vec3 Id = Ld * Kd * dot_prod; // final diffuse intensity
	
	// specular intensity
	vec3 surface_to_viewer_eye = normalize (-eye);

	vec3 incident_eye = normalize (eye);
	float ratio = 1.0 /1.3333;
	vec3 refracted = refract (incident_eye, N, ratio);
	refracted = vec3 (inverse (V) * vec4 (refracted, 0.0));
	
	// blinn
	//vec3 half_way_eye = normalize (surface_to_viewer_eye + L);
	//float dot_prod_specular = max (dot (half_way_eye, normal), 0.0);
	//float specular_factor = pow (dot_prod_specular, specular_exponent);
	
	vec3 Is = Ls * Ks * refracted; // final specular intensity
	vec4 regularTexture = texture(cube_texture, refracted);
	
	if (clicked == 0) {
		gl_FragColor = regularTexture;
	} else {
		vec4 boxTexture = texture(textureData, coord);
		gl_FragColor = mix(regularTexture, boxTexture, 0.5);
	}
}
