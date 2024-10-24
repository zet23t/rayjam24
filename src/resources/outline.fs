#extension GL_OES_standard_derivatives : enable
// outline shader;
// sample 4 neighboring pixels and determine if the current pixel is on the edge.
// edge pixels are identified by the following conditions:
// - the uv.y value differs - since the object's texture is simple, the color is selected in rows.
//   Due to this, we can use the y value as distinction indicator.
// - Z value is used to prioritize which side the edge is drawn on.
// - if the uv.y value is greater than 1.0, no edge detection is run, IF the current pixel is behind it.
//   This is done to have no outline behind FX objects but still having the outline when in front.
// The texture is 32bit float, which allows us to store the color in a single float.
precision highp float;                // Precision required for OpenGL ES2 (WebGL)
varying vec2 fragTexCoord;
varying vec4 fragColor;
uniform sampler2D texture0;
uniform vec4 colDiffuse;
uniform vec2 resolution;
void main() {
    vec4 texelColor = texture2D(texture0, fragTexCoord.xy);
    // gl_FragColor = vec4(texelColor.bbb*10.0, 1.0);
    vec4 texelColorN = texture2D(texture0, fragTexCoord.xy + vec2(0.0, 1.0 / resolution.y));
    vec4 texelColorE = texture2D(texture0, fragTexCoord.xy + vec2(1.0 / resolution.x, 0.0));
    vec4 texelColorS = texture2D(texture0, fragTexCoord.xy - vec2(0.0, 1.0 / resolution.y));
    vec4 texelColorW = texture2D(texture0, fragTexCoord.xy - vec2(1.0 / resolution.x, 0.0));
    
    float z = texelColor.b + 0.0001;
    float zN = texelColorN.b;
    float zE = texelColorE.b;
    float zS = texelColorS.b;
    float zW = texelColorW.b;
    
    // unpack color from 32bit red channel
    float f = texelColor.r;
    vec3 color = vec3(0.0);
    color.b = f / 256.0 / 256.0;
    color.g = fract((f - color.b * 256.0) / 256.0);
    color.r = fract(f);

    if (texelColor == vec4(1.0, 1.0, 1.0, 1.0))
    {
        color = vec3(1.0);
    }

    if (texelColor.g >= 1.0 || (texelColorW.g >= 1.0 && z > zW))
    {
        gl_FragColor = vec4(color, 1.0);
        return;
    }
    // gl_FragColor = vec4(screenPos.x * 1000.0, screenPos.y * 0.001, 0.0, 1.0);
    float diff = 1.0;
    float v = texelColor.g;
    float vN = texelColorN.g;
    float vE = texelColorE.g;
    float vS = texelColorS.g;
    float vW = texelColorW.g;
    if ((z < zE && abs(v - vE) > 0.01) || 
        (z < zW && abs(v - vW) > 0.01) || 
        (z < zS && abs(v - vS) > 0.01) || 
        (z < zN && abs(v - vN) > 0.01) ||
        (z + 0.0004 < zE) ||
        (z + 0.0004 < zW) ||
        (z + 0.0004 < zS) ||
        (z + 0.0004 < zN)
        )
    {
        diff = 0.0;
    }
    else {
        z -= 0.0002;
        if ((v - vW <= -0.000001 && z <= zW) ||
            (v - vS <= -0.000001 && z <= zS) ||
            (v - vN <= -0.000001 && z <= zN) ||
            (v - vE <= -0.000001 && z <= zE))
        {
            diff = 0.0;
        }
    }
    gl_FragColor = vec4(diff * color, 1.0);
}