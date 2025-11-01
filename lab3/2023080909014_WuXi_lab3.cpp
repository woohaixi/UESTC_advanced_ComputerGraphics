/*
 * OpenGL Mipmap Demo - 纹理过滤模式演示
 *
 * 概述：
 * 这个程序是一个使用OpenGL和GLUT的简单3D渲染演示，渲染一个无限延伸的走廊（由重复的10单位段落组成），包括地板、墙壁和天花板，使用BMP纹理加载。
 * 支持Mipmap纹理过滤模式的动态切换，通过右键菜单选择不同过滤器（NEAREST, LINEAR, MIPMAP变体）。
 * 交互：键盘箭头键控制旋转和前进/后退；鼠标拖拽控制视角旋转和俯仰。
 *
 * 主要特性：
 * - BMP纹理加载：支持非2的幂纹理缩放至256x256，使用gluScaleImage和gluBuild2DMipmaps生成Mipmap。
 * - 纹理过滤：菜单切换MIN_FILTER（MAG始终为LINEAR），演示Mipmap在远距离的抗锯齿效果。
 * - 场景：走廊从z=-60到0，宽度/高度±10，纹理重复（GL_REPEAT）。
 * - 相机：透视投影（35.5° FOV），支持平移、旋转、鼠标控制。
 * - 资源：需要resource/目录下的ground.bmp, wall.bmp, ceiling.bmp文件。
 *
 * 编译与运行：
 * g++ -o mipmap_demo main.c -lGL -lGLU -lglut -lm（假设freeglut）
 * ./mipmap_demo
 *
 * 注意：窗口400x400；纹理加载失败时纹理ID为0；使用GL_BGR_EXT处理BMP格式。
 */

#define _CRT_SECURE_NO_WARNINGS  // 禁用Visual Studio的fopen等安全警告

#include <stdio.h>       // 标准I/O，如fopen, fread
#include <stdlib.h>      // 内存分配，如malloc, free
#include <Windows.h>     // Windows API（可能用于兼容，但非必需）
#include <GL/freeglut.h> // FreeGLUT库：窗口管理、事件处理、OpenGL辅助

 // 纹理ID：全局，用于绑定
GLuint groundTex;  // 地板纹理
GLuint wallTex;    // 墙壁纹理
GLuint ceilingTex; // 天花板纹理

// 纹理数组，用于统一管理（索引：0=ground, 1=wall, 2=ceiling）
GLuint textures[3];

static int isMenuOpen = 0;  // 菜单打开标志（防止鼠标交互冲突）

#define BMP_OFFSET 54  // BMP文件头大小（像素数据偏移）

// 全局变换变量
static GLfloat rotateAngle = 0.0f;   // Y轴旋转角度（键盘控制）
static GLfloat depthPos = 0.0f;      // Z轴平移（前进/后退）
static GLint prevMouseX, prevMouseY; // 上次鼠标位置（拖拽计算）
static GLfloat mouseRotate = 0.0f;   // 鼠标Y旋转
static GLfloat mouseDepth = 0.0f;    // 鼠标Z平移（未使用？代码中应用但无更新）
static GLfloat viewPitch = 0.0f;     // 俯仰角（X轴旋转）

// 判断是否为2的幂：用于纹理尺寸检查（OpenGL旧版要求）
int isPowerOfTwo(int num) {
    if (num <= 0) return 0;  // 无效输入
    return (num & (num - 1)) == 0;  // 位运算：2^n只有一个位为1
}

/* 加载BMP纹理：从文件读取、缩放、生成Mipmap，返回纹理ID */
GLuint loadBmpTexture(const char* filename) {
    GLint imgWidth, imgHeight, dataSize;  // 图像尺寸和数据大小
    GLubyte* pixelData = NULL;            // 像素数据缓冲
    GLuint texID = 0;                     // 新纹理ID

    // 打开BMP文件（二进制模式）
    FILE* file = fopen(filename, "rb");
    if (!file) return 0;  // 文件打开失败

    // 读取宽度（文件偏移0x0012）
    fseek(file, 0x0012, SEEK_SET);
    fread(&imgWidth, 4, 1, file);
    // 读取高度（偏移0x0016）
    fread(&imgHeight, 4, 1, file);
    // 跳到像素数据（偏移54）
    fseek(file, BMP_OFFSET, SEEK_SET);

    // 计算行字节（RGB=3字节/像素，4字节对齐）
    GLint rowBytes = imgWidth * 3;
    while (rowBytes % 4 != 0) ++rowBytes;  // 填充到4的倍数
    dataSize = rowBytes * imgHeight;       // 总数据大小

    // 分配像素缓冲
    pixelData = (GLubyte*)malloc(dataSize);
    if (!pixelData) {
        fclose(file);
        return 0;  // 内存分配失败
    }

    // 读取像素数据（BGR格式）
    if (fread(pixelData, dataSize, 1, file) <= 0) {
        free(pixelData);
        fclose(file);
        return 0;  // 读取失败
    }

    // 检查纹理尺寸：必须2的幂且不超过最大纹理大小
    GLint maxTexSize;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTexSize);
    if (!isPowerOfTwo(imgWidth) || !isPowerOfTwo(imgHeight) ||
        imgWidth > maxTexSize || imgHeight > maxTexSize) {
        const GLint newSize = 256;  // 缩放至256x256
        GLint newRowBytes = newSize * 3;
        while (newRowBytes % 4 != 0) ++newRowBytes;  // 4字节对齐
        GLint newDataSize = newRowBytes * newSize;

        // 分配新缓冲
        GLubyte* newPixelData = (GLubyte*)malloc(newDataSize);
        if (!newPixelData) {
            free(pixelData);
            fclose(file);
            return 0;
        }

        // 使用GLU缩放图像（双线性插值）
        gluScaleImage(GL_RGB, imgWidth, imgHeight, GL_UNSIGNED_BYTE, pixelData,
            newSize, newSize, GL_UNSIGNED_BYTE, newPixelData);

        free(pixelData);         // 释放旧数据
        pixelData = newPixelData;  // 使用新数据
        imgWidth = imgHeight = newSize;  // 更新尺寸
    }

    // 生成纹理ID
    glGenTextures(1, &texID);
    if (texID == 0) {
        free(pixelData);
        fclose(file);
        return 0;  // 生成失败
    }

    // 保存当前绑定纹理
    GLuint prevTex;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, (GLint*)&prevTex);
    glBindTexture(GL_TEXTURE_2D, texID);  // 绑定新纹理

    // 设置默认过滤和包裹模式
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);  // 三线性Mipmap
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);                // 放大线性
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);                    // S方向重复
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);                    // T方向重复

    // 纹理环境：替换模式（纹理覆盖几何颜色）
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

    // 生成Mipmap（GL_BGR_EXT处理BMP的BGR顺序）
    gluBuild2DMipmaps(GL_TEXTURE_2D, 3, imgWidth, imgHeight, GL_BGR_EXT, GL_UNSIGNED_BYTE, pixelData);

    // 恢复之前绑定
    glBindTexture(GL_TEXTURE_2D, prevTex);
    free(pixelData);  // 释放数据
    fclose(file);     // 关闭文件

    return texID;  // 返回纹理ID
}

// 重塑回调：调整视口和投影矩阵
void onReshape(GLsizei width, GLsizei height) {
    glViewport(0, 0, width, height);  // 设置视口

    GLfloat ratio = (GLfloat)width / (GLfloat)height;  // 宽高比

    glMatrixMode(GL_PROJECTION);  // 投影矩阵模式
    glLoadIdentity();             // 重置

    // 透视投影：35.5° FOV，近裁剪1.0，远裁剪150.0
    gluPerspective(35.5, ratio, 1.0, 150.0);

    glMatrixMode(GL_MODELVIEW);   // 模型视图模式
    glLoadIdentity();             // 重置

    glutPostRedisplay();  // 刷新显示
}

// 显示回调：渲染场景
void onDisplay() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);  // 清色深缓冲
    glPushMatrix();  // 保存矩阵栈

    // 应用全局变换：平移、旋转（顺序重要）
    glTranslatef(0.0f, 0.0f, depthPos);  // Z平移
    glRotatef(rotateAngle, 0.0f, 1.0f, 0.0f);  // Y旋转

    glTranslatef(0.0f, 0.0f, mouseDepth);  // 鼠标Z（未更新）
    glRotatef(viewPitch, 1.0f, 0.0f, 0.0f);  // X俯仰
    glRotatef(mouseRotate, 0.0f, 1.0f, 0.0f);  // Y鼠标旋转

    // 绘制走廊段落：从z=-60到0，每10单位一个段落（重复纹理）
    for (GLfloat zStart = -60.0f; zStart <= 0.0f; zStart += 10.0f) {
        GLfloat zEnd = zStart + 10.0f;  // 段落结束Z

        // 地板：绑定纹理，绘制四边形（纹理坐标0-1）
        glBindTexture(GL_TEXTURE_2D, groundTex);
        glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f); glVertex3f(-10.0f, -10.0f, zStart);  // 左后
        glTexCoord2f(1.0f, 0.0f); glVertex3f(-10.0f, -10.0f, zEnd);    // 左前
        glTexCoord2f(1.0f, 1.0f); glVertex3f(10.0f, -10.0f, zEnd);     // 右前
        glTexCoord2f(0.0f, 1.0f); glVertex3f(10.0f, -10.0f, zStart);   // 右后
        glEnd();

        // 天花板：类似地板，但Y=10
        glBindTexture(GL_TEXTURE_2D, ceilingTex);
        glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f); glVertex3f(-10.0f, 10.0f, zStart);
        glTexCoord2f(1.0f, 0.0f); glVertex3f(-10.0f, 10.0f, zEnd);
        glTexCoord2f(1.0f, 1.0f); glVertex3f(10.0f, 10.0f, zEnd);
        glTexCoord2f(0.0f, 1.0f); glVertex3f(10.0f, 10.0f, zStart);
        glEnd();

        // 墙壁：绑定墙纹理，绘制左/右墙（两个四边形）
        glBindTexture(GL_TEXTURE_2D, wallTex);
        glBegin(GL_QUADS);
        // 左墙（X=-10）
        glTexCoord2f(0.0f, 0.0f); glVertex3f(-10.0f, -10.0f, zStart);  // 下后
        glTexCoord2f(1.0f, 0.0f); glVertex3f(-10.0f, 10.0f, zStart);   // 上后
        glTexCoord2f(1.0f, 1.0f); glVertex3f(-10.0f, 10.0f, zEnd);     // 上前
        glTexCoord2f(0.0f, 1.0f); glVertex3f(-10.0f, -10.0f, zEnd);    // 下前
        // 右墙（X=10）
        glTexCoord2f(0.0f, 0.0f); glVertex3f(10.0f, -10.0f, zStart);
        glTexCoord2f(1.0f, 0.0f); glVertex3f(10.0f, 10.0f, zStart);
        glTexCoord2f(1.0f, 1.0f); glVertex3f(10.0f, 10.0f, zEnd);
        glTexCoord2f(0.0f, 1.0f); glVertex3f(10.0f, -10.0f, zEnd);
        glEnd();
    }

    glPopMatrix();       // 恢复矩阵
    glutSwapBuffers();   // 双缓冲交换
}

// 特殊键回调：箭头键控制
void onSpecialKey(GLint key, GLint x, GLint y) {
    if (key == GLUT_KEY_UP) depthPos += 0.5f;       // 上：前进
    if (key == GLUT_KEY_DOWN) depthPos -= 0.5f;     // 下：后退
    if (key == GLUT_KEY_LEFT) rotateAngle -= 0.5f;  // 左：左转
    if (key == GLUT_KEY_RIGHT) rotateAngle += 0.5f; // 右：右转

    if (rotateAngle > 360.0f) rotateAngle = 0.0f;   // 角度归一化

    glutPostRedisplay();  // 刷新
}

// 鼠标按键回调：记录起始位置
void onMouse(int button, int state, int x, int y) {
    if (isMenuOpen) return;  // 菜单打开时忽略
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {  // 左键按下
        prevMouseX = x;
        prevMouseY = y;
    }
}

// 鼠标拖拽回调：计算旋转
void onMotion(int x, int y) {
    GLfloat dx = (GLfloat)(x - prevMouseX);  // X偏移
    GLfloat dy = (GLfloat)(y - prevMouseY);  // Y偏移

    if (dx != 0.0f || dy != 0.0f) {  // 有移动
        mouseRotate += dx * 0.1f;    // Y旋转（灵敏度0.1）
        viewPitch += dy * 0.1f;      // X俯仰

        // 限制俯仰角（避免翻转）
        if (viewPitch > 89.0f) viewPitch = 89.0f;
        if (viewPitch < -89.0f) viewPitch = -89.0f;

        prevMouseX = x;  // 更新位置
        prevMouseY = y;
        glutPostRedisplay();  // 刷新
    }
}

// 菜单选择回调：切换过滤模式
void onMenuSelect(int option) {
    isMenuOpen = 1;  // 锁定交互

    // 定义MIN_FILTER选项（10-15对应数组索引0-5）
    GLenum minFilters[] = {
        GL_NEAREST,                    // 10: 最近邻
        GL_LINEAR,                     // 11: 线性
        GL_NEAREST_MIPMAP_NEAREST,     // 12: Mipmap最近
        GL_LINEAR_MIPMAP_NEAREST,      // 13: 线性Mipmap最近
        GL_NEAREST_MIPMAP_LINEAR,      // 14: Mipmap线性
        GL_LINEAR_MIPMAP_LINEAR        // 15: 三线性Mipmap
    };

    GLenum selectedFilter = minFilters[option - 10];  // 获取选择过滤器

    // 应用到所有纹理（MAG固定LINEAR）
    for (int i = 0; i < 3; i++) {
        if (textures[i] != 0) {  // 有效纹理
            glBindTexture(GL_TEXTURE_2D, textures[i]);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, selectedFilter);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        }
    }

    isMenuOpen = 0;  // 解锁
    glutPostRedisplay();  // 刷新
}

// 初始化纹理和OpenGL状态
void initTextures() {
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);  // 清屏黑色
    glEnable(GL_DEPTH_TEST);               // 启用深度测试
    glEnable(GL_TEXTURE_2D);               // 启用2D纹理

    // 加载纹理文件（假设resource/目录）
    textures[0] = loadBmpTexture("resource/ground.bmp");
    textures[1] = loadBmpTexture("resource/wall.bmp");
    textures[2] = loadBmpTexture("resource/ceiling.bmp");

    // 赋值全局ID
    groundTex = textures[0];
    wallTex = textures[1];
    ceilingTex = textures[2];
}

// 主函数：GLUT初始化和循环
int main(int argc, char* argv[]) {
    glutInit(&argc, argv);                   // GLUT初始化
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);  // 双缓冲 + RGBA
    glutInitWindowPosition(100, 100);        // 窗口位置
    glutInitWindowSize(400, 400);            // 窗口大小
    glutCreateWindow("OpenGL Mipmap Demo");  // 创建窗口

    initTextures();  // 初始化纹理

    // 注册回调
    glutDisplayFunc(onDisplay);   // 显示
    glutReshapeFunc(onReshape);   // 重塑
    glutSpecialFunc(onSpecialKey); // 特殊键
    glutMouseFunc(onMouse);       // 鼠标按键
    glutMotionFunc(onMotion);     // 鼠标拖拽

    // 创建右键弹出菜单
    glutCreateMenu(onMenuSelect);
    glutAddMenuEntry("GL_NEAREST", 10);
    glutAddMenuEntry("GL_LINEAR", 11);
    glutAddMenuEntry("GL_NEAREST_MIPMAP_NEAREST", 12);
    glutAddMenuEntry("GL_LINEAR_MIPMAP_NEAREST", 13);
    glutAddMenuEntry("GL_NEAREST_MIPMAP_LINEAR", 14);
    glutAddMenuEntry("GL_LINEAR_MIPMAP_LINEAR", 15);
    glutAttachMenu(GLUT_RIGHT_BUTTON);  // 绑定右键

    glutMainLoop();  // 进入事件循环
    return 0;
}