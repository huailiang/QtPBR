#version 330 core
out vec4 FragColor;

in vec3 WorldPos;
in vec2 TexCoord;
in mat3 TBN;

uniform vec3 camPos;
uniform vec3 albedo;
uniform float metallic;
uniform float roughness;
uniform bool useAlbedoMap;
uniform sampler2D albedoMap;
uniform bool useNormalMap;
uniform sampler2D normalMap;
uniform bool useRmacMap;
uniform sampler2D rmacMap;
uniform vec3 lightPositions[1];
uniform vec3 lightColors[1];

const float PI = 3.14159265359;

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;
    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    return nom / max(denom, 0.001);
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;
    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    return nom / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);
    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(max(1.0 - cosTheta, 0.0), 5.0);
}

void main()
{
    vec3 N;
    if (useNormalMap) {
        vec3 tangentNormal = texture(normalMap, TexCoord).rgb;
        tangentNormal = normalize(tangentNormal * 2.0 - 1.0); // 映射到 [-1,1]
        N = normalize(TBN * tangentNormal);
    } else {
        N = normalize(TBN[2]); // 使用顶点法线（TBN 的第三列）
    }

    float M = metallic;
    float R = roughness;
    float AO = 1.0;
    if(useRmacMap) {
        vec3 tex = texture(rmacMap, TexCoord).rgb;
        R = tex.r;
        M = tex.g;
        AO = tex.b;
    }
    vec3 V = normalize(camPos - WorldPos);
    vec3 albedoColor = albedo;
    if (useAlbedoMap) {
        albedoColor = texture(albedoMap, TexCoord).rgb;
    }
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedoColor, M);
    vec3 Lo = vec3(0.0);
    for(int i = 0; i < 1; ++i)
    {
        vec3 L = normalize(lightPositions[i] - WorldPos);
        vec3 H = normalize(V + L);
        float distance = length(lightPositions[i] - WorldPos);
        float attenuation = 1.0 / (distance * distance);
        vec3 radiance = lightColors[i] * attenuation * 0.1;
        float NDF = DistributionGGX(N, H, R);
        float G = GeometrySmith(N, V, L, R);
        vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);
        vec3 numerator = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.001;
        vec3 specular = numerator / denominator;
        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - M;
        float NdotL = max(dot(N, L), 0.0);
        Lo += specular * NdotL;
    }
    vec3 ambient = vec3(0.03) * albedoColor;
    vec3 color = (ambient + Lo) * AO;

    // HDR tonemapping & gamma correction
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0/2.2));

    FragColor = vec4(color, 1.0);
}