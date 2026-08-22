#version 330 core
layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BrightColor; // Vẫn phải xuất ra 2 kênh vì FBO yêu cầu

in vec3 viewRay;

// Hàm băm (Hash) để tạo các vì sao sắc nét ngẫu nhiên
float hash(vec3 p) {
    p = fract(p * 0.3183099 + 0.1);
    p *= 17.0;
    return fract(p.x * p.y * p.z * (p.x + p.y + p.z));
}

// Hàm nhiễu 3D và FBM để làm tinh vân lờ mờ (Bỏ bớt chi tiết cho nhẹ GPU)
float noise(vec3 x) {
    vec3 i = floor(x); vec3 f = fract(x);
    f = f * f * (3.0 - 2.0 * f);
    return mix(mix(mix(hash(i), hash(i + vec3(1,0,0)), f.x),
                   mix(hash(i + vec3(0,1,0)), hash(i + vec3(1,1,0)), f.x), f.y),
               mix(mix(hash(i + vec3(0,0,1)), hash(i + vec3(1,0,1)), f.x),
                   mix(hash(i + vec3(0,1,1)), hash(i + vec3(1,1,1)), f.x), f.y), f.z);
}
float fbm(vec3 x) {
    float v = 0.0; float a = 0.5; vec3 shift = vec3(100.0);
    for (int i = 0; i < 3; ++i) { v += a * noise(x); x = x * 2.0 + shift; a *= 0.5; }
    return v;
}

void main() {
    vec3 dir = normalize(viewRay);

    // 1. SINH CÁC VÌ SAO LI TI (STARS)
    // Phóng to không gian lên 500 lần để tạo hạt nhỏ. Nếu hash > 0.995 thì thành sao, ngược lại là đen.
    float starVal = hash(dir * 500.0);
    float star = smoothstep(0.995, 1.0, starVal);

    // Làm một số ngôi sao sáng lấp lánh ngẫu nhiên
    star *= (0.3 + 0.7 * sin(dir.x * 123.0 + dir.y * 321.0));

    // 2. TẠO TINH VÂN KHÔNG GIAN SÂU (DEEP SPACE DUST)
    float dust = fbm(dir * 4.0);
    float dustMask = smoothstep(0.4, 0.8, dust);
    // Tinh vân mang màu xám/đen cực tối, đúng chất kinh dị viễn tưởng
    vec3 dustColor = vec3(0.04, 0.04, 0.04) * dustMask;

    // Kết hợp lại
    vec3 finalColor = vec3(star) + dustColor;

    FragColor = vec4(finalColor, 1.0);
    // Không đẩy ánh sáng sao vào kênh Bloom để giữ vẻ tĩnh mịch sắc lạnh
    BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
}
