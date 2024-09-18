// Must include utils/DiffuseIBL before it

vec3 SpecularIBLDegraded(vec3 normal, vec3 eyedir, vec3 color, float roughness, float metallic)
{
    float NdotV = max(dot(normal, eyedir), 0.0001);
    vec3 F0 = mix(vec3(0.04), color, metallic);

    vec3 specular_ambient = EnvBRDFApprox(F0, F_AB(roughness, NdotV));

    // No real world material has specular values under 0.02, so we use this range as a
    // "pre-baked specular occlusion" that extinguishes the fresnel term, for artistic control.
    // See: https://google.github.io/filament/Filament.html#specularocclusion
    float F90 = clamp(dot(F0, vec3(50.0 * 0.33)), 0.0, 1.0);

    return specular_ambient * F90;
}