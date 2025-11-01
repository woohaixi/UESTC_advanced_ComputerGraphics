#include <GL/glut.h>

// 全局变量
bool isRotating = false; // 控制场景是否旋转，通过按键 'r'（开始旋转）或 's'（停止旋转）切换
float rotationAngle = 0.0f; // 记录围绕 Y 轴的当前旋转角度
int projectionMode = 1; // 控制投影模式：1 表示透视投影，0 表示正交投影

// 初始化 OpenGL 环境
void initializeGL() {
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f); // 设置清屏颜色为深灰色（RGBA：0.1, 0.1, 0.1，完全不透明）
    glEnable(GL_DEPTH_TEST); // 启用深度测试，确保正确渲染 3D 对象的深度关系
}

// 绘制坐标轴（X、Y、Z），作为 3D 场景的参考
void drawCoordinateAxes() {
    glBegin(GL_LINES); // 开始绘制线段模式
    glColor3f(1.0f, 0.0f, 0.0f); // 设置颜色为红色（X 轴）
    glVertex3f(0.0f, 0.0f, 0.0f); glVertex3f(6.0f, 0.0f, 0.0f); // 绘制 X 轴（从原点到 (6, 0, 0)）
    glVertex3f(0.0f, 0.0f, 0.0f); glVertex3f(0.0f, 6.0f, 0.0f); // 绘制 Y 轴（从原点到 (0, 6, 0)）
    glVertex3f(0.0f, 0.0f, 0.0f); glVertex3f(0.0f, 0.0f, 6.0f); // 绘制 Z 轴（从原点到 (0, 0, 6)）
    glEnd(); // 结束线段绘制
}

// 绘制场景中的 3D 对象（立方体、球体、茶壶）
void drawObjects() {
    // 绘制第一个立方体
    glPushMatrix(); // 保存当前模型矩阵
    glColor3f(0.0f, 0.0f, 1.0f); // 设置颜色为蓝色
    glTranslatef(-2.0f, 0.0f, -4.0f); // 将立方体平移到 (-2, 0, -4)
    glutSolidCube(1.0f); // 绘制边长为 1 的实心立方体
    glPopMatrix(); // 恢复模型矩阵

    // 绘制第二个立方体
    glPushMatrix();
    glColor3f(1.0f, 0.0f, 0.0f); // 设置颜色为红色
    glTranslatef(-2.0f, 0.0f, -6.0f); // 将立方体平移到 (-2, 0, -6)
    glutSolidCube(1.0f); // 绘制边长为 1 的实心立方体
    glPopMatrix();

    // 绘制第一个球体
    glPushMatrix();
    glColor3f(0.0f, 0.0f, 1.0f); // 设置颜色为蓝色
    glTranslatef(0.0f, 0.0f, -4.0f); // 将球体平移到 (0, 0, -4)
    glutSolidSphere(0.7f, 20, 20); // 绘制半径为 0.7 的实心球体，细分 20x20
    glPopMatrix();

    // 绘制第二个球体
    glPushMatrix();
    glColor3f(1.0f, 0.0f, 0.0f); // 设置颜色为红色
    glTranslatef(0.0f, 0.0f, -6.0f); // 将球体平移到 (0, 0, -6)
    glutSolidSphere(0.7f, 20, 20); // 绘制半径为 0.7 的实心球体，细分 20x20
    glPopMatrix();

    // 绘制第一个茶壶
    glPushMatrix();
    glColor3f(0.0f, 0.0f, 1.0f); // 设置颜色为蓝色
    glTranslatef(2.0f, 0.0f, -4.0f); // 将茶壶平移到 (2, 0, -4)
    glutSolidTeapot(0.6f); // 绘制尺寸为 0.6 的实心茶壶
    glPopMatrix();

    // 绘制第二个茶壶
    glPushMatrix();
    glColor3f(1.0f, 0.0f, 0.0f); // 设置颜色为红色
    glTranslatef(2.0f, 0.0f, -6.0f); // 将茶壶平移到 (2, 0, -6)
    glutSolidTeapot(0.6f); // 绘制尺寸为 0.6 的实心茶壶
    glPopMatrix();
}

// 渲染整个场景
void displayScene() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // 清除颜色缓冲区和深度缓冲区
    glMatrixMode(GL_PROJECTION); // 设置当前矩阵模式为投影矩阵
    glLoadIdentity(); // 重置投影矩阵为单位矩阵

    // 根据 projectionMode 设置投影类型
    if (projectionMode == 1) {
        gluPerspective(75.0f, 1.0f, 0.5f, 50.0f); // 透视投影：视场角 75°，宽高比 1，近裁剪面 0.5，远裁剪面 50
    }
    else {
        glOrtho(-10.0f, 10.0f, -10.0f, 10.0f, 0.5f, 50.0f); // 正交投影：视口范围 [-10, 10] x [-10, 10]，近裁剪面 0.5，远裁剪面 50
    }

    glMatrixMode(GL_MODELVIEW); // 切换到模型视图矩阵
    glLoadIdentity(); // 重置模型视图矩阵为单位矩阵
    gluLookAt(0.0f, 4.0f, 12.0f, 0.0f, 0.0f, -6.0f, 0.0f, 1.0f, 0.0f); // 设置相机：位置 (0, 4, 12)，观察点 (0, 0, -6)，上方向 (0, 1, 0)

    // 如果启用旋转，增加旋转角度
    if (isRotating) {
        rotationAngle += 0.3f; // 每次帧增加 0.3 度的旋转
    }
    glRotatef(rotationAngle, 0.0f, 1.0f, 0.0f); // 围绕 Y 轴旋转 rotationAngle 度

    drawCoordinateAxes(); // 绘制坐标轴
    drawObjects(); // 绘制所有 3D 对象

    glutSwapBuffers(); // 交换前后缓冲区，显示渲染结果
}

// 定时器回调函数，用于连续更新场景
void updateScene(int value) {
    glutPostRedisplay(); // 标记窗口需要重绘，触发 displayScene
    glutTimerFunc(10, updateScene, 0); // 每 10 毫秒调用一次 updateScene，形成动画循环
}

// 右键菜单回调函数，用于切换投影模式
void projectionMenu(int option) {
    projectionMode = option; // 设置投影模式（1: 透视，0: 正交）
    glutPostRedisplay(); // 触发重绘以应用新的投影模式
}

// 鼠标事件处理函数
void handleMouse(int button, int state, int x, int y) {
    if (button == GLUT_RIGHT_BUTTON && state == GLUT_DOWN) { // 检测右键按下事件
        glutCreateMenu(projectionMenu); // 创建右键菜单
        glutAddMenuEntry("Perspective Projection", 1); // 添加菜单项：透视投影
        glutAddMenuEntry("Orthographic Projection", 0); // 添加菜单项：正交投影
        glutAttachMenu(GLUT_RIGHT_BUTTON); // 将菜单绑定到右键
    }
}

// 键盘事件处理函数
void handleKeyboard(unsigned char key, int x, int y) {
    if (key == 'r' || key == 'R') { // 按 'r' 或 'R' 开始旋转
        isRotating = true;
    }
    else if (key == 's' || key == 'S') { // 按 's' 或 'S' 停止旋转
        isRotating = false;
    }
    glutPostRedisplay(); // 触发重绘以更新场景
}

// 窗口大小调整回调函数
void reshapeWindow(int width, int height) {
    glViewport(0, 0, width, height); // 设置视口大小与窗口大小一致
}

// 主函数，程序入口
int main(int argc, char** argv) {
    glutInit(&argc, argv); // 初始化 GLUT
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH); // 设置显示模式：双缓冲、RGB 颜色、启用深度缓冲
    glutInitWindowSize(600, 600); // 设置窗口大小为 600x600 像素
    glutCreateWindow("3D Scene with Projection Switching"); // 创建窗口，标题为“3D 场景与投影切换”

    initializeGL(); // 调用初始化函数
    glutDisplayFunc(displayScene); // 注册渲染回调函数
    glutReshapeFunc(reshapeWindow); // 注册窗口调整回调函数
    glutMouseFunc(handleMouse); // 注册鼠标事件回调函数
    glutKeyboardFunc(handleKeyboard); // 注册键盘事件回调函数
    glutTimerFunc(0, updateScene, 0); // 启动定时器，立即调用 updateScene

    glutMainLoop(); // 进入 GLUT 主循环，处理事件和渲染
    return 0; // 程序结束（实际不会执行到这里，因为 glutMainLoop 不返回）
}