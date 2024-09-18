uniform sampler2D ntex;
#if defined(GL_ES) && defined(GL_FRAGMENT_PRECISION_HIGH)
uniform highp sampler2D dtex;
#else
uniform sampler2D dtex;
#endif
uniform sampler2D ctex;

uniform vec3 sundirection;
uniform vec3 sun_color;

#ifdef GL_ES
layout (location = 0) out vec4 Diff;
layout (location = 1) out vec4 Spec;
#else
out vec4 Diff;
out vec4 Spec;
#endif

#stk_include "utils/decodeNormal.frag"
#stk_include "utils/SpecularBRDF.frag"
#stk_include "utils/DiffuseBRDF.frag"
#stk_include "utils/getPosFromUVDepth.frag"
#stk_include "utils/SunMRP.frag"

void main() {
    vec2 uv = gl_FragCoord.xy / u_screen;
    float z = texture(dtex, uv).x;
    vec4 xpos = getPosFromUVDepth(vec3(uv, z), u_inverse_projection_matrix);

    vec3 norm = DecodeNormal(texture(ntex, uv).xy);
    float roughness = 1.0 - texture(ntex, uv).z;
    float metallic = texture(ntex, uv).w;
    vec3 eyedir = -normalize(xpos.xyz);

    vec3 Lightdir = SunMRP(norm, eyedir);
    float NdotL = clamp(dot(norm, Lightdir), 0., 1.);
    
    vec3 base_color = texture(ctex, uv).xyz;
    vec3 diffuse_color = (1.0 - metallic) * base_color;

    vec3 Specular = SpecularBRDF(norm, eyedir, Lightdir, base_color, roughness, metallic);
    vec3 Diffuse = DiffuseBRDF(norm, eyedir, Lightdir, diffuse_color, roughness);

    Diff = vec4(sun_color * NdotL * Diffuse, 1.);
    Spec = vec4(sun_color * NdotL * Specular, 1.);
}
