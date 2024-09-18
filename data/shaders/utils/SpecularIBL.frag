#stk_include "utils/SpecularIBLDegraded.frag"

uniform samplerCube probe;

vec3 SpecularIBL(vec3 normal, vec3 V, vec3 color, float roughness, float metallic)
{
    vec3 sampleDirection = reflect(-V, normal);
    sampleDirection = (u_inverse_view_matrix * vec4(sampleDirection, 0.)).xyz;

     // Assume 8 level of lod (ie 256x256 texture)
    float lodval = 7. * roughness;
    return clamp(textureLod(probe, sampleDirection, lodval).rgb * color, 0., 1.)
        + SpecularIBLDegraded(normal, V, color, roughness, metallic);
}
