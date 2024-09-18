// From Bevy
// https://github.com/bevyengine/bevy/blob/main/crates/bevy_pbr/src/render/pbr_lighting.wgsl

vec2 F_AB(float perceptual_roughness, float NdotV) 
{
    vec4 c0 = vec4(-1.0, -0.0275, -0.572, 0.022);
    vec4 c1 = vec4(1.0, 0.0425, 1.04, -0.04);
    vec4 r = perceptual_roughness * c0 + c1;
    float a004 = min(r.x * r.x, pow(2.0, -9.28 * NdotV)) * r.x + r.y;
    return vec2(-1.04, 1.04) * a004 + r.zw;
}

// Specular BRDF
// https://google.github.io/filament/Filament.html#materialsystem/specularbrdf
// Cook-Torrance approximation of the microfacet model integration using Fresnel law F to model f_m
// f_r(v,l) = { D(h,α) G(v,l,α) F(v,h,f0) } / { 4 (n⋅v) (n⋅l) }
vec3 SpecularBRDF(vec3 normal, vec3 eyedir, vec3 lightdir, vec3 color, float roughness, float metallic)
{
    float NdotV = max(dot(normal, eyedir), 0.0001);
    vec3 F0 = mix(vec3(0.04), color, metallic);
    vec2 F_ab = F_AB(roughness, NdotV);
    vec3 H = normalize(eyedir + lightdir);
    float NdotL = clamp(dot(normal, lightdir), 0.0, 1.0);
    float NdotH = clamp(dot(normal, H), 0.0, 1.0);
    float LdotH = clamp(dot(lightdir, H), 0.0, 1.0);

    // clamp perceptual roughness to prevent precision problems
    // According to Filament design 0.089 is recommended for mobile
    // Filament uses 0.045 for non-mobile
    roughness = clamp(roughness, 0.089, 1.0);
    roughness = roughness * roughness;
 
    // Calculate distribution.
    // Based on https://google.github.io/filament/Filament.html#citation-walter07
    // D_GGX(h,α) = α^2 / { π ((n⋅h)^2 (α2−1) + 1)^2 }
    // Simple implementation, has precision problems when using fp16 instead of fp32
    // see https://google.github.io/filament/Filament.html#listing_speculardfp16
    float oneMinusNdotHSquared = 1.0 - NdotH * NdotH;
    float a = NdotH * roughness;
    float k = roughness / (oneMinusNdotHSquared + a * a);
    float D = k * k;

    // Calculate visibility.
    // V(v,l,a) = G(v,l,α) / { 4 (n⋅v) (n⋅l) }
    // such that f_r becomes
    // f_r(v,l) = D(h,α) V(v,l,α) F(v,h,f0)
    // where
    // V(v,l,α) = 0.5 / { n⋅l sqrt((n⋅v)^2 (1−α2) + α2) + n⋅v sqrt((n⋅l)^2 (1−α2) + α2) }
    // Note the two sqrt's, that may be slow on mobile, 
    // see https://google.github.io/filament/Filament.html#listing_approximatedspecularv
    float a2 = roughness * roughness;
    float lambdaV = NdotL * sqrt((NdotV - a2 * NdotV) * NdotV + a2);
    float lambdaL = NdotV * sqrt((NdotL - a2 * NdotL) * NdotL + a2);
    float V = 0.5 / (lambdaV + lambdaL);

    // Calculate the Fresnel term.
    // f_90 suitable for ambient occlusion
    // see https://google.github.io/filament/Filament.html#lighting/occlusion
    float f90 = clamp(dot(F0, vec3(50.0 * 0.33)), 0.0, 1.0);
    // Fresnel function
    // see https://google.github.io/filament/Filament.html#citation-schlick94
    // F_Schlick(v,h,f_0,f_90) = f_0 + (f_90 − f_0) (1 − v⋅h)^5
    vec3 F = F0 + (f90 - F0) * pow(1.0 - LdotH, 5.0);

    // Calculate the specular light.
    vec3 Fr = D * V * F * (1.0 + F0 * (1.0 / F_ab.x - 1.0));
    return Fr;
}