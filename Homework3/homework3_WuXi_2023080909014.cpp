/*
 * 3D场景渲染程序 - 光照与雾效演示
 *
 * 功能概述：
 * 本程序使用OpenGL和GLUT库创建了一个包含多个3D物体的场景，演示了动态光照和雾效的使用。
 * 场景中包含茶壶、球体、立方体、圆柱体和圆环五种基本几何体，用户可以通过交互控制两种颜色
 * 光源的开关和雾效的强度。
 *
 * 主要特性：
 * 1. 双光源系统：红色方向光和蓝色方向光，可独立开关
 * 2. 可调节的指数雾效，模拟大气透视效果
 * 3. 实时状态显示：光源状态和雾强度值
 * 4. 交互控制：鼠标控制光源开关，键盘调节雾强度
 *
 * 交互方式：
 * - 左键单击：切换红色光源开关
 * - 右键单击：切换蓝色光源开关
 * - 上方向键：增加雾强度
 * - 下方向键：减小雾强度
 *
 * 技术要点：
 * - 使用GL_LIGHT0和GL_LIGHT1实现多光源照明
 * - 采用GL_EXP模式的雾效实现深度感知
 * - 正交投影与透视投影的切换用于文字渲染
 * - 矩阵堆栈操作实现局部变换
 *
 * 文件结构：
 * - 初始化函数：setupLighting(), setupFog(), initialize()
 * - 渲染函数：renderScene(), drawText(), drawCylinder(), drawTorus()
 * - 回调函数：adjustViewport(), handleKeyboard(), handleMouse()
 * - 主函数：main()
 *
 * 编译要求：
 * - 需要OpenGL和GLUT库支持
 * - 在Windows环境下需定义_CRT_SECURE_NO_DEPRECATE
 *
 * 作者：吴熙
 * 日期：2025-10-10
 */

// 禁用Visual Studio中关于scanf等函数的安全警告
#define _CRT_SECURE_NO_DEPRECATE

// 引入OpenGL和GLUT头文件
#include <GL/glut.h>
#include <stdio.h>

// 全局变量定义
bool redLightOn = true;      // 红色光源开关状态
bool blueLightOn = true;     // 蓝色光源开关状态
float fogIntensity = 0.05f;  // 雾的密度值
int screenWidth = 800;       // 窗口初始宽度
int screenHeight = 600;      // 窗口初始高度

// 初始化光照系统
void setupLighting() {
    glEnable(GL_LIGHTING);  // 启用光照系统

    // 定义红色光源的颜色（RGBA格式）
    GLfloat redLightColor[] = { 1.0f, 0.0f, 0.0f, 1.0f };
    // 定义蓝色光源的颜色
    GLfloat blueLightColor[] = { 0.0f, 0.0f, 1.0f, 1.0f };

    // 定义红色光源位置（方向光，w=0表示无限远光源）
    GLfloat light0Pos[] = { -4.0f, 0.0f, 0.0f, 0.0f };
    // 定义蓝色光源位置
    GLfloat light1Pos[] = { 4.0f, 4.0f, 0.0f, 0.0f };

    // 设置光源0（红色光）的参数
    glLightfv(GL_LIGHT0, GL_POSITION, light0Pos);  // 位置
    glLightfv(GL_LIGHT0, GL_DIFFUSE, redLightColor); // 漫反射颜色

    // 设置光源1（蓝色光）的参数
    glLightfv(GL_LIGHT1, GL_POSITION, light1Pos);  // 位置
    glLightfv(GL_LIGHT1, GL_DIFFUSE, blueLightColor); // 漫反射颜色
}

// 初始化雾效
void setupFog() {
    glEnable(GL_FOG);  // 启用雾效

    // 设置雾的颜色为浅灰色
    GLfloat fogColor[] = { 0.6f, 0.6f, 0.6f, 1.0f };
    glFogfv(GL_FOG_COLOR, fogColor);  // 应用雾颜色

    glFogi(GL_FOG_MODE, GL_EXP);      // 设置雾模式为指数衰减
    glFogf(GL_FOG_DENSITY, fogIntensity); // 设置雾的密度
}

// 在指定位置绘制文本
void drawText(float x, float y, const char* text) {
    glRasterPos2f(x, y);  // 设置文本起始位置

    // 逐个字符绘制文本
    for (const char* c = text; *c; ++c) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c); // 使用12点Helvetica字体
    }
}

// 绘制圆柱体函数
void drawCylinder(float baseRadius, float topRadius, float height, int slices, int stacks) {
    // 创建二次曲面对象
    GLUquadricObj* quadric = gluNewQuadric();
    gluQuadricDrawStyle(quadric, GLU_FILL);    // 设置绘制模式为填充
    gluQuadricNormals(quadric, GLU_SMOOTH);    // 设置法线为平滑模式
    gluCylinder(quadric, baseRadius, topRadius, height, slices, stacks); // 绘制圆柱体
    gluDeleteQuadric(quadric);  // 删除二次曲面对象，释放内存
}

// 绘制圆环体函数
void drawTorus(float innerRadius, float outerRadius, int sides, int rings) {
    glutSolidTorus(innerRadius, outerRadius, sides, rings); // 使用GLUT内置函数绘制实心圆环
}

// 主渲染函数，绘制整个场景
void renderScene() {
    // 清除颜色缓冲区和深度缓冲区
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // 切换到模型视图矩阵模式
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();  // 重置模型视图矩阵

    // 设置相机位置和视角
    gluLookAt(12.0, 10.0, 12.0,  // 相机位置 (x,y,z)
        0.0, 0.0, 0.0,     // 观察点位置
        0.0, 1.0, 0.0);    // 相机的上方向向量

    // 根据开关状态控制红色光源
    glEnable(GL_LIGHT0);
    if (!redLightOn) glDisable(GL_LIGHT0);

    // 根据开关状态控制蓝色光源
    glEnable(GL_LIGHT1);
    if (!blueLightOn) glDisable(GL_LIGHT1);

    // 绘制茶壶（右上角）
    glPushMatrix();                     // 保存当前矩阵状态
    glTranslatef(4.0f, 0.0f, 4.0f);    // 平移到指定位置
    glutSolidTeapot(1.5f);             // 绘制大小为1.5的茶壶
    glPopMatrix();                      // 恢复之前保存的矩阵状态

    // 绘制球体（左上角）
    glPushMatrix();
    glTranslatef(-4.0f, 0.0f, 4.0f);
    glutSolidSphere(1.8f, 30, 30);     // 绘制球体，30经度分段，30纬度分段
    glPopMatrix();

    // 绘制立方体（后方中间位置）
    glPushMatrix();
    glTranslatef(0.0f, 0.0f, 9.0f);    // 移动到Z轴更远的位置
    glutSolidCube(2.2f);               // 绘制边长为2.2的立方体
    glPopMatrix();

    // 绘制圆柱体（左下角）
    glPushMatrix();
    glTranslatef(-4.0f, 0.0f, -4.0f);
    glRotatef(-90.0f, 1.0f, 0.0f, 0.0f); // 绕X轴旋转-90度，使圆柱体直立
    drawCylinder(1.2f, 1.2f, 2.5f, 30, 15); // 绘制圆柱体（底面半径，顶面半径，高度，分段数）
    glPopMatrix();

    // 绘制圆环体（右下角）
    glPushMatrix();
    glTranslatef(4.0f, 0.0f, -4.0f);
    glRotatef(45.0f, 1.0f, 0.0f, 0.0f); // 绕X轴旋转45度
    drawTorus(0.6f, 1.6f, 25, 35);     // 绘制圆环（内半径，外半径，边数，环数）
    glPopMatrix();

    // 绘制状态信息文字
    glDisable(GL_LIGHTING);  // 暂时禁用光照，确保文字清晰可见

    // 切换到投影矩阵模式以设置2D正交投影
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();          // 保存当前投影矩阵
    glLoadIdentity();        // 重置投影矩阵
    gluOrtho2D(0, screenWidth, 0, screenHeight); // 设置2D正交投影

    // 切换回模型视图矩阵
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();          // 保存当前模型视图矩阵
    glLoadIdentity();        // 重置模型视图矩阵

    // 设置文字颜色为白色
    glColor3f(1.0f, 1.0f, 1.0f);

    // 格式化并绘制状态信息
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "Red Light: %s", redLightOn ? "On" : "Off");
    drawText(screenWidth - 120, 60, buffer);  // 在屏幕右上角绘制红色光源状态

    snprintf(buffer, sizeof(buffer), "Blue Light: %s", blueLightOn ? "On" : "Off");
    drawText(screenWidth - 120, 40, buffer);  // 绘制蓝色光源状态

    snprintf(buffer, sizeof(buffer), "Fog Intensity: %.2f", fogIntensity);
    drawText(screenWidth - 120, 20, buffer);  // 绘制雾强度值

    // 恢复之前的矩阵状态
    glPopMatrix();              // 恢复模型视图矩阵
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();              // 恢复投影矩阵
    glMatrixMode(GL_MODELVIEW);

    glEnable(GL_LIGHTING);      // 重新启用光照

    // 交换前后缓冲区，显示渲染结果
    glutSwapBuffers();
}

// 窗口大小调整回调函数
void adjustViewport(int width, int height) {
    // 更新全局窗口尺寸变量
    screenWidth = width;
    screenHeight = height ? height : 1;  // 防止除零错误

    // 设置视口为整个窗口
    glViewport(0, 0, width, height);

    // 切换到投影矩阵模式
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();  // 重置投影矩阵

    // 设置透视投影
    gluPerspective(50.0, (float)width / height, 0.5, 80.0);

    // 切换回模型视图矩阵模式
    glMatrixMode(GL_MODELVIEW);
}

// 特殊键盘按键处理函数（方向键等）
void handleKeyboard(int key, int x, int y) {
    switch (key) {
    case GLUT_KEY_UP:    // 上方向键：增加雾强度
        fogIntensity = fogIntensity + 0.02f > 1.0f ? 1.0f : fogIntensity + 0.02f;
        glFogf(GL_FOG_DENSITY, fogIntensity);  // 更新雾密度
        break;
    case GLUT_KEY_DOWN:  // 下方向键：减小雾强度
        fogIntensity = fogIntensity - 0.02f < 0.0f ? 0.0f : fogIntensity - 0.02f;
        glFogf(GL_FOG_DENSITY, fogIntensity);  // 更新雾密度
        break;
    }
    glutPostRedisplay();  // 标记需要重新绘制场景
}

// 鼠标按键处理函数
void handleMouse(int button, int state, int x, int y) {
    // 只处理鼠标按下事件
    if (state == GLUT_DOWN) {
        if (button == GLUT_LEFT_BUTTON) {    // 左键切换红色光源
            redLightOn = !redLightOn;
        }
        else if (button == GLUT_RIGHT_BUTTON) { // 右键切换蓝色光源
            blueLightOn = !blueLightOn;
        }
        glutPostRedisplay();  // 标记需要重新绘制场景
    }
}

// 初始化OpenGL状态
void initialize() {
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);  // 设置背景颜色为深灰色
    glEnable(GL_DEPTH_TEST);               // 启用深度测试
    glShadeModel(GL_SMOOTH);               // 设置平滑着色模式
    setupLighting();                       // 初始化光照
    setupFog();                            // 初始化雾效
}

// 程序主函数
int main(int argc, char** argv) {
    // 初始化GLUT库
    glutInit(&argc, argv);

    // 设置显示模式：双缓冲区、RGB颜色模式、深度缓冲区
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);

    // 设置窗口初始大小
    glutInitWindowSize(screenWidth, screenHeight);

    // 创建窗口并设置标题
    glutCreateWindow("3D Scene with Lighting and Fog");

    // 执行初始化操作
    initialize();

    // 注册回调函数
    glutDisplayFunc(renderScene);      // 显示回调
    glutReshapeFunc(adjustViewport);   // 窗口调整回调
    glutSpecialFunc(handleKeyboard);   // 特殊键盘按键回调
    glutMouseFunc(handleMouse);        // 鼠标按键回调

    // 进入GLUT主事件循环
    glutMainLoop();

    return 0;
}