/*
 * CPU Ray Tracer - Cornell Box with Procedural Wood Grain
 *
 * 概述：
 * 这个程序是一个基于CPU的简单光线追踪渲染器，使用OpenGL和GLUT进行显示和交互。
 * 它渲染一个经典的Cornell Box场景，包括彩色墙壁、地板、天花板，以及三个球体（红色镜面球、玻璃球、粗糙黄金球）和一个带有程序化木纹纹理的木箱。
 *
 * 主要特性：
 * - 光线追踪核心：支持漫反射、镜面反射、折射（Fresnel效应）、阴影检测。
 * - 程序化纹理：使用Perlin噪声生成木箱的长条形木纹纹理，根据法线方向投影。
 * - 材质系统：支持金属/非金属、粗糙度（模糊反射使用Monte Carlo采样）。
 * - 相机：固定视角，FOV可调，支持伽马校正。
 * - 交互：按空格键重新渲染，按ESC退出。
 *
 * 场景布局：
 * - 盒子尺寸：x从-1.5到1.5，y从0到3.0，z从-1.5到0（后墙打开）。
 * - 光源：顶部中心点光源。
 * - 球体位置：左侧红色镜面、中间玻璃、右侧黄金。
 * - 木箱：角落位置，使用Box结构和纹理函数。
 *
 * 编译与运行：
 * g++ -o raytracer main.cpp -lGL -lGLU -lglut -lm
 * ./raytracer
 *
 * 注意：渲染时间较长（CPU渲染），窗口大小800x600。
 */

#define _USE_MATH_DEFINES  // 启用M_PI等数学常量定义

#include <GL/glut.h>  // GLUT库：处理窗口、事件和交互
#include <GL/glu.h>   // GLU库：提供更高层次的OpenGL功能，如纹理加载
#include <GL/gl.h>    // OpenGL核心库：图形渲染API
#include <cmath>      // 数学函数，如sqrt、sin、cos
#include <vector>     // 动态数组，用于存储球体和盒子
#include <iostream>   // 标准输入输出，用于进度输出
#include <chrono>     // 高精度时钟，用于测量渲染时间
#include <random>     // 随机数生成，用于Perlin噪声和Monte Carlo采样
#include <algorithm>  // 算法函数，如std::swap、std::min、std::max

 // 为了简化，使用 Mersenne Twister 引擎（高质量伪随机数生成器）
std::mt19937 rng(std::chrono::high_resolution_clock::now().time_since_epoch().count());
// 用于生成 -1.0 到 1.0 之间的浮点数（用于随机扰动）
std::uniform_real_distribution<float> dist_neg_pos_one(-1.0f, 1.0f);
// 用于生成 0.0 到 1.0 之间的浮点数（备用，未使用）
std::uniform_real_distribution<float> dist_zero_one(0.0f, 1.0f);

const int WIDTH = 800;   // 渲染窗口宽度（像素）
const int HEIGHT = 600;  // 渲染窗口高度（像素）
unsigned char* framebuffer;  // 帧缓冲区：存储RGB像素数据，用于OpenGL纹理显示

// Vector3结构体：3D向量和点，用于位置、法线、颜色等
struct Vector3 {
    float x, y, z;  // 坐标分量
    Vector3(float x = 0, float y = 0, float z = 0) : x(x), y(y), z(z) {}  // 构造函数，默认零向量
    // 向量加法
    Vector3 operator+(const Vector3& v) const { return Vector3(x + v.x, y + v.y, z + v.z); }
    // 向量减法
    Vector3 operator-(const Vector3& v) const { return Vector3(x - v.x, y - v.y, z - v.z); }
    // 标量乘法（向量 * 标量）
    Vector3 operator*(float s) const { return Vector3(x * s, y * s, z * s); }
    // 逐分量乘法（Hadamard积，用于颜色混合）
    Vector3 operator*(const Vector3& v) const { return Vector3(x * v.x, y * v.y, z * v.z); }
    // 友元函数：标量 * 向量（对称）
    friend Vector3 operator*(float s, const Vector3& v) { return v * s; }
    // 点积：用于计算角度、投影
    float dot(const Vector3& v) const { return x * v.x + y * v.y + z * v.z; }
    // 叉积：用于计算法线、垂直向量
    Vector3 cross(const Vector3& v) const {
        return Vector3(y * v.z - z * v.y, z * v.x - x * v.z, x * v.y - y * v.x);
    }
    // 归一化：返回单位向量
    Vector3 normalize() const {
        float len = std::sqrt(x * x + y * y + z * z);  // 计算长度
        return len > 0 ? *this * (1.0f / len) : *this;  // 避免除零
    }
    // 长度计算
    float length() const { return std::sqrt(x * x + y * y + z * z); }
};

// Ray结构体：光线，用于追踪
struct Ray {
    Vector3 origin, direction;  // 起点和方向
    Ray(Vector3 o, Vector3 d) : origin(o), direction(d.normalize()) {}  // 构造函数，自动归一化方向
};

// Material结构体：材质属性
struct Material {
    Vector3 color;           // 基础颜色
    float ka = 0.1f, kd = 0.8f, ks = 0.2f, kr = 0.0f;  // 环境反射、漫反射、镜面反射、镜面反射系数
    float shininess = 32.0f, eta = 1.0f;  // 光泽度（Phong模型）、折射率
    float roughness = 0.0f;  // 新增：粗糙度，0为完美光滑，1为完全漫反射（用于模糊反射）
    bool isRefractive = false;  // 是否为折射材质（如玻璃）
    bool isMetallic = false;    // 是否为金属材质（影响反射颜色）
};

// Sphere结构体：球体几何体
struct Sphere {
    Vector3 center;  // 中心点
    float radius;    // 半径
    Material mat;    // 材质
    Sphere(Vector3 c, float r, Material m) : center(c), radius(r), mat(m) {}  // 构造函数
    // 光线-球体相交测试：求解二次方程，返回最近交点距离t
    bool intersect(const Ray& ray, float& t) const {
        Vector3 oc = ray.origin - center;  // 原点到中心的向量
        float a = ray.direction.dot(ray.direction);  // 方向向量平方（应为1，但浮点精度）
        float b = 2 * oc.dot(ray.direction);         // 线性项
        float c = oc.dot(oc) - radius * radius;      // 常数项
        float discriminant = b * b - 4 * a * c;      // 判别式
        if (discriminant < 0) return false;          // 无实根
        t = (-b - std::sqrt(discriminant)) / (2 * a);  // 较近交点
        if (t > 0.001f) return true;                 // 有效交点（忽略自交）
        t = (-b + std::sqrt(discriminant)) / (2 * a);  // 较远交点
        return t > 0.001f;
    }
};

// Box结构体：轴对齐包围盒（AABB）
struct Box {
    Vector3 min, max;  // 最小/最大坐标
    Material mat;      // 材质
    Box(Vector3 mn, Vector3 mx, Material m) : min(mn), max(mx), mat(m) {}  // 构造函数
    // 光线-盒子相交测试：Slab方法，计算进入/离开每个轴的t值
    bool intersect(const Ray& ray, float& t, Vector3& normal) const {
        // 计算反方向（避免除零）
        Vector3 invDir(1 / ray.direction.x, 1 / ray.direction.y, 1 / ray.direction.z);
        // X轴Slab
        float tMin = (min.x - ray.origin.x) * invDir.x;
        float tMax = (max.x - ray.origin.x) * invDir.x;
        if (tMin > tMax) std::swap(tMin, tMax);  // 确保tMin < tMax

        // Y轴Slab
        float tyMin = (min.y - ray.origin.y) * invDir.y;
        float tyMax = (max.y - ray.origin.y) * invDir.y;
        if (tyMin > tyMax) std::swap(tyMin, tyMax);
        if (tMin > tyMax || tyMin > tMax) return false;  // 无交集
        if (tyMin > tMin) tMin = tyMin;
        if (tyMax < tMax) tMax = tyMax;

        // Z轴Slab
        float tzMin = (min.z - ray.origin.z) * invDir.z;
        float tzMax = (max.z - ray.origin.z) * invDir.z;
        if (tzMin > tzMax) std::swap(tzMin, tzMax);
        if (tMin > tzMax || tzMin > tMax) return false;  // 无交集
        if (tzMin > tMin) tMin = tzMin;

        t = tMin;  // 进入点距离
        if (t < 0.001f || t > 1000.0f) return false;  // 忽略无效交点

        // 计算击中点
        Vector3 hitPoint = ray.origin + ray.direction * t;
        float eps = 0.001f;  // 浮点精度阈值
        normal = Vector3(0, 0, 0);  // 初始化法线
        // 根据击中面和入射方向确定法线（外法线）
        if (std::abs(hitPoint.x - min.x) < eps && ray.direction.x < 0) normal = Vector3(-1, 0, 0);
        else if (std::abs(hitPoint.x - max.x) < eps && ray.direction.x > 0) normal = Vector3(1, 0, 0);
        else if (std::abs(hitPoint.y - min.y) < eps && ray.direction.y < 0) normal = Vector3(0, -1, 0);
        else if (std::abs(hitPoint.y - max.y) < eps && ray.direction.y > 0) normal = Vector3(0, 1, 0);
        else if (std::abs(hitPoint.z - min.z) < eps && ray.direction.z < 0) normal = Vector3(0, 0, -1);
        else if (std::abs(hitPoint.z - max.z) < eps && ray.direction.z > 0) normal = Vector3(0, 0, 1);

        // 如果无法精确确定（边缘情况），使用点到中心的近似法线
        if (normal.length() == 0) {
            Vector3 center = (min + max) * 0.5f;
            normal = (hitPoint - center).normalize();
        }
        return true;
    }
};

// ----------------------------------------------------
// 柏林噪声相关辅助函数
// 这是一个简化的1D和2D Perlin-like noise，用于生成木纹扰动
float interpolate(float a, float b, float t) {
    return (1 - t) * a + t * b;  // 线性插值
}

// Vector3版本的插值，用于颜色混合
Vector3 interpolate(const Vector3& a, const Vector3& b, float t) {
    return a * (1 - t) + b * t;
}

// 缓和曲线，用于平滑插值（Perlin噪声标准fade函数）
float fade(float t) {
    return t * t * t * (t * (t * 6 - 15) + 10);  // 5次多项式缓动
}

// 随机梯度向量（简化版，仅用于1D/2D）：基于hash计算梯度
float grad(int hash, float x, float y) {
    switch (hash & 0xF) {  // 使用低4位，选择8个方向
    case 0x0: return  x + y;
    case 0x1: return -x + y;
    case 0x2: return  x - y;
    case 0x3: return -x - y;
    case 0x4: return  x;
    case 0x5: return -x;
    case 0x6: return  y;
    case 0x7: return -y;
    default: return 0;  // 后备
    }
}

// Permutation table (256个随机数)：Perlin噪声的核心，看起来随机但可重复
int p[512];  // p[i] = p[i+256]，扩展到512以支持周期

// 初始化Perlin噪声：打乱置换表
void initPerlinNoise() {
    for (int i = 0; i < 256; ++i) {
        p[i] = i;  // 初始化为顺序
    }
    std::shuffle(p, p + 256, rng);  // 使用随机数生成器打乱
    for (int i = 0; i < 256; ++i) {
        p[i + 256] = p[i];  // 复制以实现周期性
    }
}

// 2D Perlin噪声计算：网格插值 + 梯度
float perlinNoise(float x, float y) {
    int X = (int)std::floor(x) & 255;  // 整数部分，模256
    int Y = (int)std::floor(y) & 255;

    x -= std::floor(x);  // 小数部分
    y -= std::floor(y);

    float u = fade(x);  // 缓动u
    float v = fade(y);  // 缓动v

    int A = p[X] + Y;      // 左下角hash
    int B = p[X + 1] + Y;  // 右下角hash

    // X方向插值，然后Y方向插值
    return interpolate(
        interpolate(grad(p[A], x, y), grad(p[B], x - 1, y), u),      // 下层插值
        interpolate(grad(p[A + 1], x, y - 1), grad(p[B + 1], x - 1, y - 1), u),  // 上层插值
        v
    );
}

// ----------------------------------------------------

// Scene结构体：场景管理，包括几何体、光源、相机、追踪函数
struct Scene {
    std::vector<Sphere*> spheres;  // 球体列表
    std::vector<Box*> boxes;       // 盒子列表
    Vector3 lightPos = Vector3(0, 2.9f, 0);     // 光源位置（顶部中心）
    Vector3 lightColor = Vector3(1.5f, 1.5f, 1.5f);  // 光源颜色（白色，强度1.5）
    Vector3 bgColor = Vector3(0.85f, 0.85f, 0.85f);  // 背景色（浅灰）
    Vector3 cameraPos = Vector3(0, 1.5f, 2.5f);  // 相机位置（前方）
    Vector3 lookAt = Vector3(0, 1.5f, 0.0f);     // 注视点（中心）
    float fov = 90.0f * M_PI / 180.0f;           // 视野角度（弧度，90度以填满窗口）

    // 析构函数：清理动态分配的几何体
    ~Scene() {
        for (auto s : spheres) delete s;
        for (auto b : boxes) delete b;
    }

    // 平面相交测试：无限平面
    bool planeIntersect(const Ray& ray, float& t, const Vector3& point, const Vector3& normal) {
        float denom = normal.dot(ray.direction);  // 法线·方向
        if (std::abs(denom) > 0.001f) {  // 非平行
            t = (point - ray.origin).dot(normal) / denom;  // t = (点 - 原点) · 法线 / denom
            return t > 0.001f;  // 正向交点
        }
        return false;
    }

    // 修改：用于生成长条形木质条纹的函数（程序化纹理）
    Vector3 getWoodTextureColor(const Vector3& hitPoint, const Vector3& normal) {
        // 基础木材颜色：浅木和深木
        Vector3 lightWood = Vector3(0.65f, 0.45f, 0.25f);
        Vector3 darkWood = Vector3(0.45f, 0.25f, 0.1f);

        // 调整纹理频率和扰动强度
        float scale = 10.0f;             // 纹理缩放
        float stripeDensity = 0.3f;      // 条纹密度，较小值产生更宽的条纹
        float noiseStrength = 0.3f;      // 噪声扰动强度

        float noiseVal;                  // Perlin噪声值
        float stripeCoord;               // 条纹坐标

        // 根据法线方向选择条纹投影平面和方向（UV映射）
        if (std::abs(normal.y) > 0.9f) { // 顶部/底部面 (XZ平面投影，条纹沿Z方向)
            stripeCoord = hitPoint.z * scale; // 沿Z方向条纹
            noiseVal = perlinNoise(hitPoint.x * scale * 0.5f, hitPoint.z * scale * 0.5f); // 扰动沿X
        }
        else if (std::abs(normal.x) > 0.9f) { // 侧面X (YZ平面投影，条纹沿Y方向)
            stripeCoord = hitPoint.y * scale; // 沿Y方向条纹（垂直）
            noiseVal = perlinNoise(hitPoint.y * scale * 0.5f, hitPoint.z * scale * 0.5f); // 扰动沿Z
        }
        else { // 侧面Z (XY平面投影，条纹沿Y方向)
            stripeCoord = hitPoint.y * scale; // 沿Y方向条纹（垂直）
            noiseVal = perlinNoise(hitPoint.x * scale * 0.5f, hitPoint.y * scale * 0.5f); // 扰动沿X
        }

        // 生成条纹图案：使用sin函数创建周期性条纹，加上噪声扰动
        float stripePattern = std::sin(stripeCoord * 2.0f * M_PI / stripeDensity + noiseVal * noiseStrength * 10.0f);
        stripePattern = (stripePattern + 1.0f) * 0.5f; // 归一化到0-1
        stripePattern = std::abs(stripePattern - 0.5f) * 2.0f; // 创建尖锐的条纹边缘
        stripePattern = std::pow(stripePattern, 2.0f); // 锐化纹理，使条纹更明显

        // 根据pattern在两种木色之间插值
        return interpolate(lightWood, darkWood, stripePattern);
    }

    // 核心追踪函数：递归光线追踪，返回像素颜色
    Vector3 trace(const Ray& ray, int depth = 0) {
        if (depth > 6) return bgColor;  // 限制递归深度，避免无限循环

        float tMin = 100000.0f;  // 最近交点距离
        Material hitMat;         // 击中材质
        Vector3 hitPoint, hitNormal;  // 击中点和法线
        bool hasHit = false;     // 是否有交点
        bool isBox = false;      // 标记是否击中盒子（用于纹理）

        // 球体相交：遍历所有球体，更新最近交点
        for (auto sphere : spheres) {
            float t;
            if (sphere->intersect(ray, t) && t < tMin) {
                tMin = t;
                hitPoint = ray.origin + ray.direction * t;
                hitNormal = (hitPoint - sphere->center).normalize();  // 球体法线：径向
                hitMat = sphere->mat;
                hasHit = true;
                isBox = false;
            }
        }

        // 盒子相交：遍历所有盒子
        for (auto box : boxes) {
            float t;
            Vector3 normal;
            if (box->intersect(ray, t, normal) && t < tMin) {
                tMin = t;
                hitPoint = ray.origin + ray.direction * t;
                hitNormal = normal;
                hitMat = box->mat;  // 先获取基础材质
                hasHit = true;
                isBox = true;
            }
        }

        // 地板 y=0 (精细木纹)：平面交点 + 边界检查
        float tPlane;
        if (planeIntersect(ray, tPlane, Vector3(0, 0, 0), Vector3(0, 1, 0)) && tPlane < tMin) {
            Vector3 p = ray.origin + ray.direction * tPlane;
            if (p.x >= -1.5f && p.x <= 1.5f && p.z >= -1.5f && p.z <= 1.5f) {  // 盒子边界
                tMin = tPlane;
                hitPoint = p;
                hitNormal = Vector3(0, 1, 0);  // 上法线

                // 简单木纹：sin波纹 + 条纹
                float woodX = p.x * 15.0f;
                float woodZ = p.z * 8.0f;
                float pattern = std::sin(woodX) * 0.5f + 0.5f;  // X方向波纹
                int stripe = static_cast<int>(woodZ * 5) % 2;    // Z方向条纹
                Vector3 baseColor = stripe ? Vector3(0.55f, 0.35f, 0.15f) : Vector3(0.45f, 0.25f, 0.1f);
                hitMat.color = baseColor * (0.8f + pattern * 0.2f);  // 调制颜色
                hitMat.ka = 0.15f; hitMat.kd = 0.75f; hitMat.ks = 0.15f;  // 材质属性
                hitMat.kr = 0.0f; hitMat.shininess = 20.0f;
                hitMat.isMetallic = false;
                hitMat.roughness = 0.0f;  // 地板光滑
                hasHit = true;
                isBox = false;
            }
        }

        // 左墙 x=-1.5 (红)：平面 + 边界
        if (planeIntersect(ray, tPlane, Vector3(-1.5f, 0, 0), Vector3(1, 0, 0)) && tPlane < tMin) {
            Vector3 p = ray.origin + ray.direction * tPlane;
            if (p.y >= 0 && p.y <= 3.0f && p.z >= -1.5f && p.z <= 1.5f) {
                tMin = tPlane; hitPoint = p; hitNormal = Vector3(1, 0, 0);  // 内法线
                hitMat.color = Vector3(0.75f, 0.1f, 0.1f);  // 红色
                hitMat.ka = 0.1f; hitMat.kd = 0.8f; hitMat.ks = 0.05f; hitMat.kr = 0.0f;
                hitMat.isMetallic = false;
                hitMat.roughness = 0.0f;  // 光滑
                hasHit = true;
                isBox = false;
            }
        }

        // 右墙 x=1.5 (绿)
        if (planeIntersect(ray, tPlane, Vector3(1.5f, 0, 0), Vector3(-1, 0, 0)) && tPlane < tMin) {
            Vector3 p = ray.origin + ray.direction * tPlane;
            if (p.y >= 0 && p.y <= 3.0f && p.z >= -1.5f && p.z <= 1.5f) {
                tMin = tPlane; hitPoint = p; hitNormal = Vector3(-1, 0, 0);
                hitMat.color = Vector3(0.1f, 0.75f, 0.1f);  // 绿色
                hitMat.ka = 0.1f; hitMat.kd = 0.8f; hitMat.ks = 0.05f; hitMat.kr = 0.0f;
                hitMat.isMetallic = false;
                hitMat.roughness = 0.0f;
                hasHit = true;
                isBox = false;
            }
        }

        // 后墙 z=-1.5 (白)
        if (planeIntersect(ray, tPlane, Vector3(0, 0, -1.5f), Vector3(0, 0, 1)) && tPlane < tMin) {
            Vector3 p = ray.origin + ray.direction * tPlane;
            if (p.x >= -1.5f && p.x <= 1.5f && p.y >= 0 && p.y <= 3.0f) {
                tMin = tPlane; hitPoint = p; hitNormal = Vector3(0, 0, 1);
                hitMat.color = Vector3(0.85f, 0.85f, 0.85f);  // 白色
                hitMat.ka = 0.1f; hitMat.kd = 0.8f; hitMat.ks = 0.05f; hitMat.kr = 0.0f;
                hitMat.isMetallic = false;
                hitMat.roughness = 0.0f;
                hasHit = true;
                isBox = false;
            }
        }

        // 天花板 y=3.0 (白)
        if (planeIntersect(ray, tPlane, Vector3(0, 3.0f, 0), Vector3(0, -1, 0)) && tPlane < tMin) {
            Vector3 p = ray.origin + ray.direction * tPlane;
            if (p.x >= -1.5f && p.x <= 1.5f && p.z >= -1.5f && p.z <= 1.5f) {
                tMin = tPlane; hitPoint = p; hitNormal = Vector3(0, -1, 0);
                hitMat.color = Vector3(0.85f, 0.85f, 0.85f);
                hitMat.ka = 0.1f; hitMat.kd = 0.8f; hitMat.ks = 0.05f; hitMat.kr = 0.0f;
                hitMat.isMetallic = false;
                hitMat.roughness = 0.0f;
                hasHit = true;
                isBox = false;
            }
        }

        if (!hasHit) return bgColor;  // 无交点，返回背景

        // 如果是木箱，更新材质颜色为长条形木纹颜色
        if (isBox) {
            hitMat.color = getWoodTextureColor(hitPoint, hitNormal);
        }

        Vector3 finalColor = Vector3(0, 0, 0);  // 最终颜色初始化

        // 处理折射材质（优先级最高，因为折射会改变光线路径）
        if (hitMat.isRefractive&& depth < 6) {
            float cosI = -ray.direction.dot(hitNormal);  // 入射角余弦
            float eta = hitMat.eta;  // 折射率
            Vector3 n = hitNormal;   // 法线

            bool entering = cosI > 0;  // 是否进入介质
            if (!entering) {  // 从内部射出
                cosI = -cosI;
                n = hitNormal * -1;  // 翻转法线
                eta = 1.0f / eta;    // 倒数折射率
            }

            float sinT2 = eta * eta * (1 - cosI * cosI);  // Snell定律
            if (sinT2 < 1.0f) {  // 发生折射（无全反射）
                float cosT = std::sqrt(1 - sinT2);  // 折射角余弦
                // 折射方向公式
                Vector3 refractDir = (ray.direction * eta + n * (eta * cosI - cosT)).normalize();
                Ray refractRay(hitPoint - n * 0.001f, refractDir);  // 偏移避免自交
                Vector3 refractColor = trace(refractRay, depth + 1);  // 递归追踪

                // Fresnel效应：反射/透射比例
                float R0 = ((eta - 1) * (eta - 1)) / ((eta + 1) * (eta + 1));  // 垂直入射反射率
                float fresnel = R0 + (1 - R0) * std::pow(1 - cosI, 5);  // Schlick近似

                // 反射部分
                Vector3 reflectDir = (ray.direction - hitNormal * 2 * ray.direction.dot(hitNormal)).normalize();
                Ray reflectRay(hitPoint + hitNormal * 0.001f, reflectDir);
                Vector3 reflectColor = trace(reflectRay, depth + 1);

                finalColor = refractColor * (1 - fresnel) + reflectColor * fresnel;  // 混合
            }
            else {  // 全反射
                Vector3 reflectDir = (ray.direction - n * 2 * ray.direction.dot(n)).normalize();
                Ray reflectRay(hitPoint + n * 0.001f, reflectDir);
                finalColor = trace(reflectRay, depth + 1);
            }
            return finalColor;  // 折射材质直接返回
        }

        // 标准光照计算 (漫反射 + 镜面高光，Phong模型)
        Vector3 localColor = Vector3(0, 0, 0);  // 本地照明颜色
        Vector3 lightDir = (lightPos - hitPoint).normalize();  // 光线方向
        float lightDist = (lightPos - hitPoint).length();      // 光源距离

        // 阴影检测：从击中点向光源发射影子光线
        Ray shadowRay(hitPoint + hitNormal * 0.001f, lightDir);  // 偏移
        bool inShadow = false;

        // 检查球体遮挡
        for (auto s : spheres) {
            float st;
            if (s->intersect(shadowRay, st) && st < lightDist - 0.001f) {
                inShadow = true; break;
            }
        }

        // 检查盒子遮挡
        if (!inShadow) {
            for (auto b : boxes) {
                float bt; Vector3 bn;
                if (b->intersect(shadowRay, bt, bn) && bt < lightDist - 0.001f) {
                    inShadow = true; break;
                }
            }
        }

        if (!inShadow) {  // 无阴影
            // 环境光：恒定
            localColor = localColor + hitMat.ka * hitMat.color;

            // 漫反射：Lambert模型
            float diff = std::max(0.0f, hitNormal.dot(lightDir));  // cos theta
            float attenuation = 1.0f / (1.0f + 0.05f * lightDist * lightDist);  // 距离衰减

            if (hitMat.isMetallic) {
                // 金属：减少漫反射，并用材质色调制光色
                localColor = localColor + hitMat.kd * (hitMat.color * lightColor) * diff * attenuation * 0.1f;
            }
            else {
                localColor = localColor + hitMat.kd * (hitMat.color * lightColor) * diff * attenuation;
            }

            // 镜面高光：Phong模型
            Vector3 viewDir = (cameraPos - hitPoint).normalize();  // 视线方向
            Vector3 reflectDir = (hitNormal * (2 * hitNormal.dot(lightDir)) - lightDir).normalize();  // 反射方向
            float spec = std::pow(std::max(0.0f, viewDir.dot(reflectDir)), hitMat.shininess);  // 高光强度
            // 金属高光用材质色，非金属用光色
            localColor = localColor + hitMat.ks * (hitMat.isMetallic ? hitMat.color : lightColor) * spec * attenuation;
        }
        else {
            // 阴影中：仅环境光，减半
            localColor = hitMat.ka * hitMat.color * 0.5f;
        }

        finalColor = localColor;  // 本地照明作为基础

        // 处理反射 (对于所有材质，通过kr参数控制)
        if (hitMat.kr > 0.0f && depth < 6) {
            Vector3 reflectDir = (ray.direction - hitNormal * 2 * ray.direction.dot(hitNormal)).normalize();  // 完美反射方向

            // 引入Monte Carlo模糊反射（粗糙表面）
            Vector3 glossyReflectColor = Vector3(0, 0, 0);
            int SAMPLES_PER_GLOSSY_RAY = 1;  // 默认1条光线
            if (hitMat.roughness > 0.001f) {
                SAMPLES_PER_GLOSSY_RAY = 16;  // 粗糙时增加采样（噪声减少）
            }

            // 多采样循环
            for (int i = 0; i < SAMPLES_PER_GLOSSY_RAY; ++i) {
                Vector3 perturbedReflectDir = reflectDir;  // 起始为完美反射

                if (hitMat.roughness > 0.001f) {
                    // 简单随机扰动：生成随机单位向量
                    float r1 = dist_neg_pos_one(rng);
                    float r2 = dist_neg_pos_one(rng);
                    float r3 = dist_neg_pos_one(rng);
                    Vector3 randomVec = Vector3(r1, r2, r3).normalize();

                    // 混合：扰动强度与roughness成正比
                    perturbedReflectDir = (reflectDir + randomVec * hitMat.roughness).normalize();

                    // 确保在法线半球（避免背面）
                    if (perturbedReflectDir.dot(hitNormal) < 0) {
                        perturbedReflectDir = reflectDir;
                    }
                }

                Ray reflectRay(hitPoint + hitNormal * 0.001f, perturbedReflectDir);  // 偏移
                glossyReflectColor = glossyReflectColor + trace(reflectRay, depth + 1);  // 递归
            }
            glossyReflectColor = glossyReflectColor * (1.0f / SAMPLES_PER_GLOSSY_RAY);  // 平均

            // 混合本地光和反射光
            if (hitMat.isMetallic) {
                // 金属：反射色 * 材质色
                Vector3 metalReflectColor = glossyReflectColor * hitMat.color;
                finalColor = finalColor * (1.0f - hitMat.kr) + metalReflectColor * hitMat.kr;
            }
            else {
                // 非金属：直接混合
                finalColor = finalColor * (1.0f - hitMat.kr) + glossyReflectColor * hitMat.kr;
            }
        }

        return finalColor;  // 返回最终颜色
    }

    // 渲染函数：生成帧缓冲区
    void render() {
        auto start = std::chrono::high_resolution_clock::now();  // 开始计时
        Vector3 forward = (lookAt - cameraPos).normalize();  // 前向向量
        Vector3 right = forward.cross(Vector3(0, 1, 0)).normalize();  // 右向量（世界Y上）
        Vector3 up = right.cross(forward).normalize();       // 上向量

        // 像素循环：生成主光线
        for (int y = 0; y < HEIGHT; ++y) {
            for (int x = 0; x < WIDTH; ++x) {
                // 屏幕坐标到方向：透视投影
                float u = (2.0f * x / WIDTH - 1.0f) * std::tan(fov / 2) * (WIDTH / float(HEIGHT));  // X偏移，宽高比校正
                float v = (1.0f - 2.0f * y / HEIGHT) * std::tan(fov / 2);  // Y偏移（翻转Y）
                Vector3 dir = (forward + right * u + up * v).normalize();  // 光线方向
                Ray ray(cameraPos, dir);
                Vector3 col = trace(ray);  // 追踪颜色

                // 伽马校正：sRGB到线性（近似2.2的逆）
                col.x = std::pow(std::max(0.0f, std::min(col.x, 1.0f)), 0.454f);  // 1/2.2 ≈ 0.454
                col.y = std::pow(std::max(0.0f, std::min(col.y, 1.0f)), 0.454f);
                col.z = std::pow(std::max(0.0f, std::min(col.z, 1.0f)), 0.454f);

                // 写入帧缓冲：RGB字节
                int idx = (y * WIDTH + x) * 3;
                framebuffer[idx] = static_cast<unsigned char>(std::min(255.0f, col.x * 255));
                framebuffer[idx + 1] = static_cast<unsigned char>(std::min(255.0f, col.y * 255));
                framebuffer[idx + 2] = static_cast<unsigned char>(std::min(255.0f, col.z * 255));
            }
            if (y % 10 == 0) std::cout << "进度: " << (y * 100 / HEIGHT) << "%" << std::endl;  // 进度输出
        }

        auto end = std::chrono::high_resolution_clock::now();  // 结束计时
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        std::cout << "渲染完成! 时间: " << duration.count() / 1000.0f << " 秒" << std::endl;  // 输出时间
    }
};

Scene* scene;  // 全局场景指针
GLuint texture;  // OpenGL纹理ID

// 初始化函数：设置场景几何体和OpenGL
void init() {
    initPerlinNoise();  // 初始化噪声置换表
    framebuffer = new unsigned char[WIDTH * HEIGHT * 3];  // 分配帧缓冲
    glClearColor(0, 0, 0, 1);  // 清屏色黑
    glEnable(GL_TEXTURE_2D);   // 启用2D纹理
    glGenTextures(1, &texture);  // 生成纹理ID
    glBindTexture(GL_TEXTURE_2D, texture);  // 绑定
    // 纹理过滤：线性插值
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // === 左侧红色镜面球 ===
    Material redMirror;
    redMirror.color = Vector3(0.9f, 0.1f, 0.1f);  // 红色
    redMirror.ka = 0.05f; redMirror.kd = 0.0f; redMirror.ks = 0.9f;  // 无漫反射，高镜面
    redMirror.kr = 1.0f;  // 完美反射
    redMirror.shininess = 100.0f;  // 高光泽
    redMirror.roughness = 0.0f;  // 光滑
    redMirror.isMetallic = false;
    scene->spheres.push_back(new Sphere(Vector3(-1.0f, 0.4f, 0.5f), 0.4f, redMirror));  // 添加球体

    // === 中间玻璃球 ===
    Material glass;
    glass.color = Vector3(0.95f, 0.95f, 0.95f);  // 近白（透明）
    glass.ka = 0.0f; glass.kd = 0.0f; glass.ks = 0.1f;  // 无漫反射
    glass.kr = 0.0f;  // 反射由Fresnel处理
    glass.eta = 1.5f;  // 玻璃折射率
    glass.isRefractive = true;
    glass.isMetallic = false;
    glass.roughness = 0.0f;
    scene->spheres.push_back(new Sphere(Vector3(0.0f, 0.4f, -0.2f), 0.4f, glass));

    // === 右侧黄金球 —— 完全不透明的闪耀黄金！===
    Material gold;
    gold.color = Vector3(1.0f, 0.76f, 0.33f);   // 黄金颜色
    gold.ka = 0.1f;          // 环境
    gold.kd = 0.05f;         // 少量漫反射
    gold.ks = 1.0f;          // 强高光
    gold.kr = 0.9f;          // 强反射
    gold.shininess = 200.0f; // 高光泽
    gold.roughness = 0.2f;   // 粗糙度（模糊反射）
    gold.isRefractive = false;
    gold.isMetallic = true;
    gold.eta = 1.0f;
    scene->spheres.push_back(new Sphere(Vector3(0.85f, 0.25f, 0.6f), 0.25f, gold));  // 小球

    // === 木箱（角落）===
    Material woodBox;
    woodBox.color = Vector3(0.5f, 0.3f, 0.15f);  // 基础木色（将被纹理覆盖）
    woodBox.ka = 0.1f; woodBox.kd = 0.75f; woodBox.ks = 0.1f;  // 木材质地
    woodBox.kr = 0.0f; woodBox.shininess = 15.0f;
    woodBox.isMetallic = false;
    woodBox.roughness = 0.05f;  // 轻微粗糙
    scene->boxes.push_back(new Box(Vector3(0.5f, 0, -1.3f), Vector3(1.3f, 1.0f, -0.5f), woodBox));  // 盒子位置
}

// 显示回调：将帧缓冲渲染为全屏四边形纹理
void display() {
    glClear(GL_COLOR_BUFFER_BIT);  // 清屏
    glBindTexture(GL_TEXTURE_2D, texture);  // 绑定纹理
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, WIDTH, HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, framebuffer);  // 更新纹理数据
    glBegin(GL_QUADS);  // 绘制全屏四边形
    glTexCoord2f(0, 1); glVertex2f(-1, -1);  // 左下
    glTexCoord2f(1, 1); glVertex2f(1, -1);   // 右下
    glTexCoord2f(1, 0); glVertex2f(1, 1);    // 右上
    glTexCoord2f(0, 0); glVertex2f(-1, 1);   // 左上
    glEnd();
    glutSwapBuffers();  // 双缓冲交换
}

// 键盘回调：处理输入
void keyboard(unsigned char key, int x, int y) {
    if (key == 27) exit(0);  // ESC退出
    if (key == ' ') {  // 空格重新渲染
        std::cout << "\n开始重新渲染..." << std::endl;
        scene->render();
        glutPostRedisplay();  // 刷新显示
    }
}

// 主函数：初始化GLUT和场景
int main(int argc, char** argv) {
    scene = new Scene();  // 创建场景
    glutInit(&argc, argv);  // GLUT初始化
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);  // 双缓冲 + RGBA
    glutInitWindowSize(WIDTH, HEIGHT);  // 窗口大小
    glutCreateWindow("CPU Ray Tracer - Cornell Box with Wood Grain");  // 窗口标题

    init();  // 初始化场景和OpenGL
    scene->render();  // 首次渲染

    glutDisplayFunc(display);  // 显示回调
    glutKeyboardFunc(keyboard);  // 键盘回调
    glutMainLoop();  // 进入事件循环

    // 清理
    delete[] framebuffer;
    delete scene;
    return 0;
}