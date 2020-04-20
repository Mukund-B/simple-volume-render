#version 440 core

layout(origin_upper_left) in vec4 gl_FragCoord;

uniform vec2 viewport;
uniform float focal_length;
uniform vec3 ray_origin;
uniform mat4 MV;

uniform sampler1D xferTex ; // 1D transfer function texture LUT
uniform sampler3D voldataTex ; // 3D dataset texture
uniform float h; //stepsize
uniform int renderType; 
out vec4 fragColor;

struct Ray{
    vec3 d;
    vec3 o;
    vec3 start;
    vec3 stop;
    float length;
    vec3 pos;
    vec3 step;
};

//helper function to calculate points of intersection of ray with bounding box.
void intersectBox (Ray r, out float t_near, out float t_far){
    //unitary volume bounding box
    vec3 boxmin = vec3(-0.5f);
    vec3 boxmax = vec3(0.5f);

    //compute intersection of ray with all six box planes
    vec3 invR = rcp(r.d);
    vec3 t_bot = invR * (boxmin - r.o);
    vec3 t_top = invR * (boxmax - r.o);

    vec3 t_min = vec3(100.0f);
    vec3 t_max = vec3 (0.0f);

    t_min = min(t_min, min(t_top, t_bot));
    t_max = max(t_max, max(t_top, t_bot));

    t_near = max(max(t_min.x, t_min.y), t_min.z);
    t_far = min(min(t_max.x, t_max.y), t_max.z);
}
void main()
{
    float t_near, t_far;
    float thresh = 0.4f;

    Ray r;
    r.d.xy = 2.0 * (gl_FragCoord.xy / viewport) - 1.0;
    r.d.z = -focal_length;
    r.d = (vec4(r.d, 0) * MV).xyz;
    r.d = normalize(r.d);

    r.o = ray_origin;  //divergent case

    intersectBox (r, t_near, t_far);

    r.start = (r.o + r.d * t_near - vec3(-0.5f));
    r.stop =  (r.o + r.d * t_far  - vec3(-0.5f));
    r.length = distance (r.stop, r.start);
    r.step = h * normalize(r.stop - r.start);

    r.pos = r.start;

    float scalar, maxScalar = 0.0f;
    vec4 C = vec4(0.0f);

    //Render types:
    //  0: Maximum Intensity
    //  1: Isovalue
    //  2: Emission-Absorption Compositing Equation
    switch (renderType){
        case 0:
            //Max Intensity
            for(float i = 0; i <= r.length ; i += h){
                scalar = texture(voldataTex, r.pos).r;
                maxScalar = max(maxScalar, scalar);
                r.pos += r.step;
            }
            C.rgb = vec3(maxScalar);
            C.a = 1.0f;
            break;

        case 1:
            //iso surface
            for(float i = 0; i <= r.length ; i += h){
                scalar = texture(voldataTex, r.pos).r;
                if (scalar >= thresh){
                    maxScalar = scalar;
                    break;
                }
                r.pos += r.step;
            }
            C.rgb = vec3(maxScalar);
            C.a = 1.0f;
            break;

        case 2:
            vec4 raysamp;
            for(float i = 0; i <= r.length ; i += h){
                raysamp = texture(xferTex, texture(voldataTex, r.pos).r);

                C.rgb = C.rgb + (1 - C.a) * raysamp.a * raysamp.rgb;
                C.a = C.a + (1 - C.a) * raysamp.a;
                if (C.a >= 1.0)
                    break;
                r.pos += r.step;
            }
            break;
    }
    fragColor = C;
    //fragColor = vec4(1.0);
}