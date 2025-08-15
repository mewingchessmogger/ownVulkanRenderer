#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(set = 2, binding = 0) uniform sampler2D texSampler;

layout(location = 0) out vec4 outColor;

void main() {

   // outColor = vec4(fragTexCoord,0.0,1.0);

     vec4 tex =  texture(texSampler, vec2(fragTexCoord.x,fragTexCoord.y));
    
     outColor =  mix(tex,vec4(fragColor,1.0f),0.3);
    
    
    //outColor = texture(texSampler, fragTexCoord * 2.0);    
    
}