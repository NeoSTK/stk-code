uniform sampler2D ntex;
#if defined(GL_ES) && defined(GL_FRAGMENT_PRECISION_HIGH)
uniform highp sampler2D dtex;
#else
uniform sampler2D dtex;
#endif
uniform sampler2D ctex;

flat in vec3 center;
flat in float energy;
flat in vec3 col;
flat in float radius;

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

void main()
{
    vec2 texc = gl_FragCoord.xy / u_screen;
    float z = texture(dtex, texc).x;
    vec3 norm = DecodeNormal(texture(ntex, texc).xy);
    float roughness = 1.0 - texture(ntex, texc).z;
    float metallic = texture(ntex, texc).w;

    vec4 xpos = getPosFromUVDepth(vec3(texc, z), u_inverse_projection_matrix);
    vec3 eyedir = -normalize(xpos.xyz);

    vec4 pseudocenter = u_view_matrix * vec4(center.xyz, 1.0);
    pseudocenter /= pseudocenter.w;
    vec3 light_pos = pseudocenter.xyz;
    vec3 light_col = col.xyz * energy;
    vec3 light_to_frag = light_pos - xpos.xyz;
    float d2 = dot(light_to_frag, light_to_frag);

    float inv_range_square = 1 / radius / radius;
    float factor = d2 * inv_range_square;
    float smoothFactor = clamp(1.0 - factor * factor, 0.0, 1.0);
    float att = smoothFactor * smoothFactor * 1.0 / max(d2, 0.0001);
    if (att <= 0.) discard;

    // Light Direction
    vec3 L = -normalize(xpos.xyz - light_pos);

    vec3 base_color = texture(ctex, texc).xyz;
    vec3 diffuse_color = (1.0 - metallic) * base_color;

    float NdotL = clamp(dot(norm, L), 0., 1.);
    vec3 Specular = SpecularBRDF(norm, eyedir, L, base_color, roughness, metallic);
    vec3 Diffuse = DiffuseBRDF(norm, eyedir, L, diffuse_color, roughness);

    Diff = vec4(light_col * Diffuse * NdotL * att, 1.);
    Spec = vec4(light_col * Specular * NdotL * att, 1.);
}
