// Lambert model
float F_Schlick(float f0, float f90, float VdotH)
{
    // not using mix to keep the vec3 and float versions identical
    return f0 + (f90 - f0) * pow(1.0 - VdotH, 5.0);
}

vec3 DiffuseBRDF(vec3 normal, vec3 eyedir, vec3 lightdir, vec3 color, float roughness)
{
    vec3 H = normalize(eyedir + lightdir);
    float NdotV = max(dot(normal, eyedir), 0.0001);
    float NdotL = clamp(dot(normal, lightdir), 0.0, 1.0);
    float LdotH = clamp(dot(lightdir, H), 0.0, 1.0);

    // clamp perceptual roughness to prevent precision problems
    // According to Filament design 0.089 is recommended for mobile
    // Filament uses 0.045 for non-mobile
    roughness = clamp(roughness, 0.089, 1.0);
    roughness = roughness * roughness;
 

    float f90 = 0.5 + 2.0 * roughness * LdotH * LdotH;
    float lightScatter = F_Schlick(1.0, f90, NdotL);
    float viewScatter = F_Schlick(1.0, f90, NdotV);
    return color * lightScatter * viewScatter;
}
