cbuffer ConstantBuffer
{
    float4x4 Final;
    float4x4 rotation;    // the rotation matrix
    float4 lightvec;      // the light's vector
    float4 lightcol;      // the light's color
    float4 ambientcol;    // the ambient light's color
}

struct VOut
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

VOut VShader(float4 position : POSITION, float4 normal : NORMAL)
{
    VOut output;

    output.position = mul(Final, position);

    // set the ambient light
    output.color = ambientcol;

    // calculate the diffuse light and add it to the ambient light
    float4 norm = normalize(mul(rotation, normal));
    float diffusebrightness = saturate(dot(norm, lightvec));
    output.color += lightcol * diffusebrightness;

    return output;
}

float4 PShader(float4 position : POSITION, float4 color : COLOR) : SV_TARGET
{
    return color;
}