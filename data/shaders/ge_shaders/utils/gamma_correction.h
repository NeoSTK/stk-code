vec4 gamma_correction(vec4 color)
{
#ifdef PBR_ENABLED
    return vec4(pow(color.rgb, vec3(0.4545)), color.a);
#else
    return color;
#endif
}