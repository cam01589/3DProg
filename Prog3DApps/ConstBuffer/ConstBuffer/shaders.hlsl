cbuffer ConstantBuffer
{
	float3 Offset;
}


struct VOut
{
	float4 position : SV_POSITION;
	float4 color : COLOR;
};

VOut VShader( float4 position : POSITION, float4 color : COLOR )
{
	VOut output;

	output.position = position;
	output.position.x += Offset.x;
	output.position.y += Offset.y;
	output.position.z += Offset.z;
	
	output.color = color;	
		
	return output;
}

float4 PShader( float4 position : SV_POSITION, float4 color : COLOR ) : SV_TARGET
{
	return color;
}