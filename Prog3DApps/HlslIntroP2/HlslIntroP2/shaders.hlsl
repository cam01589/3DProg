struct VOut
{
	float4 position : SV_POSITION;
	float4 color : COLOR;
};

VOut VShader( float4 position : POSITION, float4 color : COLOR )
{
	VOut output;

	output.position = position;
	output.color = color;	
	output.color.r = 1.0f;
	output.position.xy *= 0.7f;
	
	return output;
}

float4 PShader( float4 position : SV_POSITION, float4 color : COLOR ) : SV_TARGET
{
	return color;
}