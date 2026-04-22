struct Pixel_shader_input {
    float4 position : SV_POSITION;
    float4 colour : COLOUR;
};

float4 main(Pixel_shader_input input) : SV_TARGET {
    return input.colour;
}