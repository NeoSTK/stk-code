uniform sampler2D ntex;
uniform sampler2D dtex;
uniform sampler2D albedo;

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
#stk_include "utils/SpecularIBL.frag"


float distanceSquared(vec2 a, vec2 b)
{
    a -= b;
    return dot(a, a);
}

// Fade out edges of screen buffer tex
// 1 means full render tex, 0 means full IBL tex
float GetEdgeFade(vec2 coords)
{
    float gradL = smoothstep(0.0, 0.4, coords.x);
    float gradR = 1.0 - smoothstep(0.6, 1.0, coords.x);
    float gradT = smoothstep(0.0, 0.4, coords.y);
    float gradB = 1.0 - smoothstep(0.6, 1.0, coords.y);
    return min(min(gradL, gradR), min(gradT, gradB));
}

vec2 RayCast(vec3 dir, vec3 hitCoord, out bool ishit)
{
    vec3 endCoord = hitCoord + dir * 10000.0f;

    vec3 v0 = hitCoord;
    vec3 v1 = endCoord;

    vec4 h0 = vec4(v0, 1.0) * u_projection_matrix;
    vec4 h1 = vec4(v1, 1.0) * u_projection_matrix;

    float k0 = 1.0 / h0.w;
    float k1 = 1.0 / h1.w;

    vec3 q0 = v0 * k0;
    vec3 q1 = v1 * k1;

    vec2 p0 = (h0.xy * k0 + 1) / 2 * u_screen;
    vec2 p1 = (h1.xy * k1 + 1) / 2 * u_screen;

    p1 += vec2((distanceSquared(p0, p1) < 0.0001) ? 0.01 : 0.0);

    vec2 delta = p1 - p0;

    bool permute = false;
    if (abs(delta.x) < abs(delta.y))
    {
        permute = true;
        delta = delta.yx;
        p0 = p0.yx;
        p1 = p1.yx;
    }

    float stepdir = sign(delta.x);
    float invdx = stepdir / delta.x;
    vec3 dq = (q1 - q0) * invdx;
    float dk = (k1 - k0) * invdx;
    vec2 dp = vec2(stepdir, delta.y * invdx);
    float stride = 1.0;
    float jitter = 1.0;

    dp *= stride;
    dq *= stride;
    dk *= stride;

    p0 += jitter * dp;
    q0 += jitter * dq;
    k0 += jitter * dk;

    int curstep = 0;
    int maxstep = 5000;
    vec2 p = p0;
    float k = k0;
    vec3 q = q0;
    float prevz = v0.z;
    vec2 uv = vec2(0.0);
    vec2 depths = vec2(prevz);

    while (curstep < maxstep && p.x * stepdir < p1.x * stepdir)
    {
        uv = permute ? 1 - p.yx / u_screen : 1 - p / u_screen;
        depths.x = prevz;
        depths.y = (dq.z * 0.5 + q.z) / (dk * 0.5 + k);
        prevz = depths.y;
        if (depths.x < depths.y);
            depths.xy = depths.yx;
        if (uv.x > u_screen.x || uv.y > u_screen.y || uv.x < 0 || uv.y < 0)
            break;
        float depth = getPosFromUVDepth(vec3(uv, texture(dtex, uv).x), u_inverse_projection_matrix).z;
        ishit = depths.y < depth && depths.x > depth;
        if (ishit)
            break;
        
        p += dp;
        q.z += dq.z;
        k += dk;
        curstep++;
    }
    if (ishit)
    {
        float l = 0, r = 1.0;
        while (r - l > 1.0 / stride)
        {
            float mid = (l + r) * 0.5;
            uv = permute ? 1 - (p.yx + dp.yx * mid) / u_screen : 1 - (p + dp * mid) / u_screen;
            float depths = (dq.z * (mid - 0.5) + q.z) / (dk * (mid - 0.5) + k);
            float depth = getPosFromUVDepth(vec3(uv, texture(dtex, uv).x), u_inverse_projection_matrix).z;
            if (depth > depths) l = mid;
            else r = mid;
        }
    }
    return uv;
}

// Main ===================================================================

void main(void)
{
    vec2 uv = gl_FragCoord.xy / u_screen;
    vec3 normal = normalize(DecodeNormal(texture(ntex, uv).xy));

    Diff = vec4(0.25 * DiffuseIBL(normal), 1.);

    float z = texture(dtex, uv).x;

    vec4 xpos = getPosFromUVDepth(vec3(uv, z), u_inverse_projection_matrix);
    vec3 eyedir = -normalize(xpos.xyz);
    // Extract roughness
    float specval = texture(ntex, uv).z;

#ifdef GL_ES
    Spec = vec4(.25 * SpecularIBL(normal, eyedir, specval), 1.);
#else
    // :::::::: Compute Space Screen Reflection ::::::::::::::::::::::::::::::::::::

    // Output color
    vec3 outColor;

    // Fallback (if the ray can't find an intersection we display the sky)
    vec3 fallback = .25 * SpecularIBL(normal, eyedir, specval);

    // Only calculate reflections if the reflectivity value is high enough,
    // otherwise just use specular IBL
    if (specval > 0.5)
    {
        // Reflection vector
        vec3 reflected = reflect(-eyedir, normal);
        bool ishit = false;

        vec2 coords = RayCast(reflected, xpos.xyz, ishit);

        if (!ishit) {
            outColor = fallback;
        } else {
            // FIXME We need to generate mipmap to take into account the gloss map
            outColor = textureLod(albedo, coords, 0.f).rgb;
            outColor = mix(fallback, outColor, GetEdgeFade(coords));
            // TODO temporary measure the lack of mipmapping for RTT albedo
            // Implement it in proper way
            // Use (specval - 0.5) * 2.0 to bring specval from 0.5-1.0 range to 0.0-1.0 range
            outColor = mix(fallback, outColor, (specval - 0.5) * 2.0);
        }
    }
    else
    {
        outColor = fallback;
    }

    Spec = vec4(outColor.rgb, 1.0);
#endif

}
