// From Bevy
vec2 F_AB(float perceptual_roughness, float NdotV) 
{
    vec4 c0 = vec4(-1.0, -0.0275, -0.572, 0.022);
    vec4 c1 = vec4(1.0, 0.0425, 1.04, -0.04);
    vec4 r = perceptual_roughness * c0 + c1;
    float a004 = min(r.x * r.x, pow(2.0, -9.28 * NdotV)) * r.x + r.y;
    return vec2(-1.04, 1.04) * a004 + r.zw;
}

vec3 EnvBRDFApprox(vec3 F0, vec2 F_ab)
{
    return F0 * F_ab.x + F_ab.y;
}

vec3 DiffuseIBL(vec3 normal, vec3 eyedir, vec3 color, float roughness)
{
    float NdotV = max(dot(normal, eyedir), 0.0001);

    vec3 diffuse_ambient = EnvBRDFApprox(color, F_AB(1.0, NdotV));

    return diffuse_ambient;
}