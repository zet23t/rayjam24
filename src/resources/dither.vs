precision mediump float;                // Precision required for OpenGL ES2 (WebGL) (on some browsers)
attribute vec3 vertexPosition;
attribute vec2 vertexTexCoord;
attribute vec4 vertexColor;
varying vec2 fragTexCoord;
varying vec4 fragColor;
varying vec3 fragPosition;

uniform mat4 mvp;
void main() {
    fragTexCoord = vertexTexCoord;
    fragColor = vertexColor;
    gl_Position = mvp * vec4(vertexPosition, 1.0);
    fragPosition = gl_Position.xyz;
}
