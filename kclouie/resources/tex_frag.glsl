#version 330 core

in vec2 texCoord;
out vec4 color;
uniform sampler2D texBuf;

/* just pass through the texture color but add a little blue to anything on left side */
void main(){
   color = vec4(texture( texBuf, texCoord ).rgb, 1);

}
