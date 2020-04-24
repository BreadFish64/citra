//? #version 320 es

// from https://github.com/BreadFish64/ScaleFish/tree/master/scale_force
// shader adapted to GLSL 320 es and debugging outputs stripped

// MIT License
//
// Copyright (c) 2020 BreadFish64
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

precision mediump float;

in vec2 tex_coord;

out vec4 frag_color;

uniform sampler2D input_texture;

vec2 tex_size;
vec2 inv_tex_size;

vec4 cubic(float v) {
    vec4 n = vec4(1.0, 2.0, 3.0, 4.0) - v;
    vec4 s = n * n * n;
    float x = s.x;
    float y = s.y - 4.0 * s.x;
    float z = s.z - 4.0 * s.y + 6.0 * s.x;
    float w = 6.0 - x - y - z;
    return vec4(x, y, z, w) / 6.0;
}

// Bicubic interpolation
vec4 textureBicubic(vec2 tex_coords) {
    tex_coords = tex_coords * tex_size - 0.5;

    vec2 fxy = fract(tex_coords);
    tex_coords -= fxy;

    vec4 xcubic = cubic(fxy.x);
    vec4 ycubic = cubic(fxy.y);

    vec4 c = tex_coords.xxyy + vec2(-0.5, +1.5).xyxy;

    vec4 s = vec4(xcubic.xz + xcubic.yw, ycubic.xz + ycubic.yw);
    vec4 offset = c + vec4(xcubic.yw, ycubic.yw) / s;

    offset *= inv_tex_size.xxyy;

    vec4 sample0 = textureLod(input_texture, offset.xz, 0.0);
    vec4 sample1 = textureLod(input_texture, offset.yz, 0.0);
    vec4 sample2 = textureLod(input_texture, offset.xw, 0.0);
    vec4 sample3 = textureLod(input_texture, offset.yw, 0.0);

    float sx = s.x / (s.x + s.y);
    float sy = s.z / (s.z + s.w);

    return mix(mix(sample3, sample2, sx), mix(sample1, sample0, sx), sy);
}

// Finds the distance between colors in YCbCr space,
// with a higher priority given to tone rather than luminance.
// Also handles the alpha channel
float ColorDist(vec4 a, vec4 b) {
    // https://en.wikipedia.org/wiki/YCbCr#ITU-R_BT.2020_conversion
    const vec3 K = vec3(0.2627, 0.6780, 0.0593);
    const float LUMINANCE_WEIGHT = .6;
    const mat3 MATRIX = mat3(K * LUMINANCE_WEIGHT, -.5 * K.r / (1.0 - K.b), -.5 * K.g / (1.0 - K.b),
                             .5, .5, -.5 * K.g / (1.0 - K.r), -.5 * K.b / (1.0 - K.r));
    const float LENGTH_ADJUSTMENT = length(vec3(1.0)) / length(vec3(LUMINANCE_WEIGHT, 1.0, 1.0));
    vec4 diff = abs(a - b);
    vec2 alpha_product = vec2(a.a * b.a);
    vec3 YCbCr = diff.rgb * MATRIX;
    float d = dot(YCbCr, YCbCr) * (LENGTH_ADJUSTMENT * LENGTH_ADJUSTMENT);
    return sqrt(dot(vec2(d + diff.a), alpha_product));
}

// Regular Bilinear interpolated texel at tex_coord.
vec4 center_texel;

// Calculates the effect of the surrounding texels on the final texel's coordinate.
#define ColorDiff(x, y) {                                                                         \
    const vec2 offset = vec2(x, y);                                                               \
    const float weight = pow(length(offset), -length(offset));                                    \
    vec4 texel = textureLodOffset(input_texture, tex_coord, 0.0, ivec2(x, y));                    \
    total_offset += vec3(ColorDist(texel, center_texel) * weight) * vec3(offset, 1.0);            \
}

void main() {
    center_texel = textureLod(input_texture, tex_coord, 0.0);
    tex_size = vec2(textureSize(input_texture, 0));
    inv_tex_size = 1.0 / tex_size;

    // x and y are the calculated location of the final texel.
    // z is the sum of all the weighted color differences from surrounding pixels.
    vec3 total_offset = vec3(0.0);

    ColorDiff(-1, -2);
    ColorDiff(0, -2);
    ColorDiff(1, -2);

    ColorDiff(-2, -1);
    ColorDiff(-1, -1);
    ColorDiff(0, -1);
    ColorDiff(1, -1);
    ColorDiff(2, -1);

    ColorDiff(-2, 0);
    ColorDiff(-1, 0);
    // center_tex
    ColorDiff(1, 0);
    ColorDiff(2, 0);

    ColorDiff(-2, 1);
    ColorDiff(-1, 1);
    ColorDiff(0, 1);
    ColorDiff(1, 1);
    ColorDiff(2, 1);

    ColorDiff(-1, 2);
    ColorDiff(0, 2);
    ColorDiff(1, 2);

    if(total_offset.z == 0.0){
        // Doing bicubic filtering just past the edges where the offset is 0 causes black floaters
        // and it doesn't really matter which filter is used when the colors aren't changing.
        frag_color = center_texel;
    } else {
        // When the image has thin points, they tend to split apart.
        // This is because the texels all around are different
        // and total_offset reaches into clear areas.
        // This works pretty well to keep the offset in bounds for these cases.
        float clamp_val = length(total_offset.xy) / total_offset.z;
        vec2 final_offset = clamp(total_offset.xy, -clamp_val, clamp_val);

        final_offset /= vec2(textureSize(input_texture, 0));
        frag_color = textureBicubic(tex_coord - final_offset);
    }
}
