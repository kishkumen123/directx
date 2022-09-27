// SCALAR VARIABLES
//   bool, int, uint, half, and float
// VECTOR VARIABLES (max 4)
//   float1,2,3,4
// MATRIX VARIABLES (max 4x4)
//   float4x4

cbuffer constant_offset{ 
    float4x4 final_matrix;
    float4x4 rotation_matrix;
    float4 light_direction;
    float4 light_color;
    float4 ambient_color;
};

Texture2D Texture;
SamplerState ssDefault;

struct vs_in{ 
    float3 position : POSITION; 
    float4 normal : NORMAL;
    float2 texture_uv : TEXCOORD;
    //float4 color : COLOR;
};

struct vs_out{ 
    float4 position : SV_POSITION; 
    float4 color : COLOR;
    float2 texture_uv : TEXCOORD;
};

vs_out vs_main(vs_in input){
    vs_out output = (vs_out)0; // zero the memory first

    output.position = mul(final_matrix, float4(input.position, 1.0));

    output.color = ambient_color;

    // TODO: FIX THIS LIGHT STUFF, ROTATION_MATRIX ISNT BEING PASSED IN
    float4 normal = normalize(mul(rotation_matrix, input.normal));
    float diffuse_brightness = saturate(dot(normal, light_direction));
    output.color += (light_color * diffuse_brightness);

    output.texture_uv = input.texture_uv;

    return output;
};

float4 ps_main(vs_out input) : SV_TARGET{ 
    return (input.color * Texture.Sample(ssDefault, input.texture_uv));
};
