uniform sampler2D ntex;
#if defined(GL_ES) && defined(GL_FRAGMENT_PRECISION_HIGH)
uniform highp sampler2D dtex;
#else
uniform sampler2D dtex;
#endif
uniform sampler2D ctex;

uniform vec3 ambient_color;

#ifdef GL_ES
layout (location = 0) out vec4 Diff;
layout (location = 1) out vec4 Spec;
#else
out vec4 Diff;
out vec4 Spec;
#endif

#stk_include "utils/decodeNormal.frag"
#stk_include "utils/getPosFromUVDepth.frag"
#stk_include "utils/DiffuseIBL.frag"
#stk_include "utils/SpecularIBLDegraded.frag"

void main(void)
{
    vec2 uv = gl_FragCoord.xy / u_screen;
    vec3 normal = DecodeNormal(texture(ntex, uv).xy);

    float z = texture(dtex, uv).x;
    vec4 xpos = getPosFromUVDepth(vec3(uv, z), u_inverse_projection_matrix);
    vec3 eyedir = -normalize(xpos.xyz);
    float roughness = 1.0 - texture(ntex, uv).z;
    float metallic = texture(ntex, uv).w;

    vec3 base_color = texture(ctex, uv).xyz;
    vec3 diffuse_color = (1.0 - metallic) * base_color;

    Diff = vec4(DiffuseIBL(normal, eyedir, diffuse_color, roughness) * ambient_color, 1.);
    Spec = vec4(SpecularIBLDegraded(normal, eyedir, base_color, roughness, metallic) * ambient_color, 1.);
}
