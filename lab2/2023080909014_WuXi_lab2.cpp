#include <GL/freeglut.h>
#include <GL/glut.h>
#include <math.h>
#define PI 3.14159265358979323846

// 全局光照参数
struct LightParams {
    float position[4] = { 0.0f, 3.0f, 2.0f, 1.0f };  // 光源位置 (x,y,z,w)，w=1表示点光源
    float ambient[4] = { 0.0f, 0.0f, 0.0f, 1.0f };   // 环境光颜色 (RGBA)
    float diffuse[3] = { 1.0f, 1.0f, 1.0f };         // 漫反射光颜色 (RGB)
    float specular[4] = { 1.0f, 1.0f, 1.0f, 1.0f };  // 镜面反射光颜色 (RGBA)
    float moveSpeed = 0.5f;      // 光源移动速度
    float colorStep = 0.1f;      // 颜色变化步长
} light;

// 相机参数
struct Camera {
    float distance = 2.0f;       // 相机与场景原点的距离
    float angleX = 0.0f;         // 绕X轴旋转角度（俯仰角）
    float angleY = 0.0f;         // 绕Y轴旋转角度（偏航角）
    int lastMouseX = 0;          // 上一次鼠标X坐标（用于计算鼠标移动）
    int lastMouseY = 0;          // 上一次鼠标Y坐标（用于计算鼠标移动）
} camera;

// 球体位置配置：3行×4列的球体网格
const float spherePositions[3][4][3] = {
    // 第一行（顶部）
    {{-2.0f, 1.5f, -7.0f}, {-0.5f, 1.5f, -7.0f}, {1.0f, 1.5f, -7.0f}, {2.5f, 1.5f, -7.0f}},
    // 第二行（中间）
    {{-2.0f, 0.0f, -7.0f}, {-0.5f, 0.0f, -7.0f}, {1.0f, 0.0f, -7.0f}, {2.5f, 0.0f, -7.0f}},
    // 第三行（底部）
    {{-2.0f, -1.5f, -7.0f}, {-0.5f, -1.5f, -7.0f}, {1.0f, -1.5f, -7.0f}, {2.5f, -1.5f, -7.0f}}
};

// 预定义的材质数组
GLfloat noMaterial[4] = { 0.0f, 0.0f, 0.0f, 1.0f };         // 无材质属性
GLfloat grayAmbient[4] = { 0.7f, 0.7f, 0.7f, 1.0f };        // 灰色环境光
GLfloat yellowAmbient[4] = { 0.8f, 0.8f, 0.2f, 1.0f };      // 黄色环境光
GLfloat blueDiffuse[4] = { 0.1f, 0.5f, 0.8f, 1.0f };        // 蓝色漫反射
GLfloat whiteSpecular[4] = { 1.0f, 1.0f, 1.0f, 1.0f };      // 白色镜面反射
GLfloat redEmission[4] = { 0.3f, 0.2f, 0.2f, 1.0f };        // 红色自发光

// 初始化OpenGL环境
void initializeGraphics() {
    glClearColor(0.0f, 0.1f, 0.1f, 1.0f);  // 设置背景颜色（深蓝绿色）
    glEnable(GL_DEPTH_TEST);                // 启用深度测试
    glShadeModel(GL_SMOOTH);                // 设置平滑着色模式
    glEnable(GL_LIGHTING);                  // 启用光照
    glEnable(GL_LIGHT0);                    // 启用0号光源
    glEnable(GL_NORMALIZE);                 // 启用法向量归一化

    // 初始光源设置
    glLightfv(GL_LIGHT0, GL_AMBIENT, light.ambient);    // 设置环境光属性
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light.diffuse);    // 设置漫反射光属性
    glLightfv(GL_LIGHT0, GL_SPECULAR, light.specular);  // 设置镜面反射光属性
}

// 绘制单个球体
void renderSphereAt(float x, float y, float z, float radius, int row, int col) {
    glPushMatrix();     // 保存当前矩阵状态
    glTranslatef(x, y, z);  // 将球体移动到指定位置

    // 根据位置设置材质 - 使用预定义数组
    // row: 1=第一行, 2=第二行, 3=第三行
    // col: 1=第一列, 2=第二列, 3=第三列, 4=第四列
    switch (row) {
    case 1: // 第一行 - 无环境光
        if (col == 1) { // 仅漫反射
            glMaterialfv(GL_FRONT, GL_AMBIENT, noMaterial);    // 无环境光
            glMaterialfv(GL_FRONT, GL_DIFFUSE, blueDiffuse);   // 蓝色漫反射
            glMaterialfv(GL_FRONT, GL_SPECULAR, noMaterial);   // 无镜面反射
            glMaterialf(GL_FRONT, GL_SHININESS, 0.0f);         // 无光泽度
            glMaterialfv(GL_FRONT, GL_EMISSION, noMaterial);   // 无自发光
        }
        else if (col == 2) { // 漫反射 + 低镜面
            glMaterialfv(GL_FRONT, GL_AMBIENT, noMaterial);
            glMaterialfv(GL_FRONT, GL_DIFFUSE, blueDiffuse);
            glMaterialfv(GL_FRONT, GL_SPECULAR, whiteSpecular); // 白色镜面反射
            glMaterialf(GL_FRONT, GL_SHININESS, 5.0f);          // 低光泽度
            glMaterialfv(GL_FRONT, GL_EMISSION, noMaterial);
        }
        else if (col == 3) { // 漫反射 + 高镜面
            glMaterialfv(GL_FRONT, GL_AMBIENT, noMaterial);
            glMaterialfv(GL_FRONT, GL_DIFFUSE, blueDiffuse);
            glMaterialfv(GL_FRONT, GL_SPECULAR, whiteSpecular);
            glMaterialf(GL_FRONT, GL_SHININESS, 100.0f);        // 高光泽度
            glMaterialfv(GL_FRONT, GL_EMISSION, noMaterial);
        }
        else { // 漫反射 + 自发光
            glMaterialfv(GL_FRONT, GL_AMBIENT, noMaterial);
            glMaterialfv(GL_FRONT, GL_DIFFUSE, blueDiffuse);
            glMaterialfv(GL_FRONT, GL_SPECULAR, noMaterial);
            glMaterialfv(GL_FRONT, GL_EMISSION, redEmission);   // 红色自发光
            glMaterialf(GL_FRONT, GL_SHININESS, 0.0f);
        }
        break;

    case 2: // 第二行 - 有环境光
        if (col == 1) { // 漫反射 + 环境，无镜面
            glMaterialfv(GL_FRONT, GL_AMBIENT, grayAmbient);   // 灰色环境光
            glMaterialfv(GL_FRONT, GL_DIFFUSE, blueDiffuse);
            glMaterialfv(GL_FRONT, GL_SPECULAR, noMaterial);
            glMaterialf(GL_FRONT, GL_SHININESS, 0.0f);
            glMaterialfv(GL_FRONT, GL_EMISSION, noMaterial);
        }
        else if (col == 2) { // 漫反射 + 环境 + 低镜面
            glMaterialfv(GL_FRONT, GL_AMBIENT, grayAmbient);
            glMaterialfv(GL_FRONT, GL_DIFFUSE, blueDiffuse);
            glMaterialfv(GL_FRONT, GL_SPECULAR, whiteSpecular);
            glMaterialf(GL_FRONT, GL_SHININESS, 5.0f);
            glMaterialfv(GL_FRONT, GL_EMISSION, noMaterial);
        }
        else if (col == 3) { // 漫反射 + 环境 + 高镜面
            glMaterialfv(GL_FRONT, GL_AMBIENT, grayAmbient);
            glMaterialfv(GL_FRONT, GL_DIFFUSE, blueDiffuse);
            glMaterialfv(GL_FRONT, GL_SPECULAR, whiteSpecular);
            glMaterialf(GL_FRONT, GL_SHININESS, 100.0f);
            glMaterialfv(GL_FRONT, GL_EMISSION, noMaterial);
        }
        else { // 漫反射 + 环境 + 自发光
            glMaterialfv(GL_FRONT, GL_AMBIENT, grayAmbient);
            glMaterialfv(GL_FRONT, GL_DIFFUSE, blueDiffuse);
            glMaterialfv(GL_FRONT, GL_SPECULAR, noMaterial);
            glMaterialfv(GL_FRONT, GL_EMISSION, redEmission);
            glMaterialf(GL_FRONT, GL_SHININESS, 0.0f);
        }
        break;

    case 3: // 第三行 - 有彩色环境光
        if (col == 1) { // 漫反射 + 彩色环境，无镜面
            glMaterialfv(GL_FRONT, GL_AMBIENT, yellowAmbient); // 黄色环境光
            glMaterialfv(GL_FRONT, GL_DIFFUSE, blueDiffuse);
            glMaterialfv(GL_FRONT, GL_SPECULAR, noMaterial);
            glMaterialf(GL_FRONT, GL_SHININESS, 0.0f);
            glMaterialfv(GL_FRONT, GL_EMISSION, noMaterial);
        }
        else if (col == 2) { // 漫反射 + 彩色环境 + 低镜面
            glMaterialfv(GL_FRONT, GL_AMBIENT, yellowAmbient);
            glMaterialfv(GL_FRONT, GL_DIFFUSE, blueDiffuse);
            glMaterialfv(GL_FRONT, GL_SPECULAR, whiteSpecular);
            glMaterialf(GL_FRONT, GL_SHININESS, 5.0f);
            glMaterialfv(GL_FRONT, GL_EMISSION, noMaterial);
        }
        else if (col == 3) { // 漫反射 + 彩色环境 + 高镜面
            glMaterialfv(GL_FRONT, GL_AMBIENT, yellowAmbient);
            glMaterialfv(GL_FRONT, GL_DIFFUSE, blueDiffuse);
            glMaterialfv(GL_FRONT, GL_SPECULAR, whiteSpecular);
            glMaterialf(GL_FRONT, GL_SHININESS, 100.0f);
            glMaterialfv(GL_FRONT, GL_EMISSION, noMaterial);
        }
        else { // 漫反射 + 彩色环境 + 自发光
            glMaterialfv(GL_FRONT, GL_AMBIENT, yellowAmbient);
            glMaterialfv(GL_FRONT, GL_DIFFUSE, blueDiffuse);
            glMaterialfv(GL_FRONT, GL_SPECULAR, noMaterial);
            glMaterialfv(GL_FRONT, GL_EMISSION, redEmission);
            glMaterialf(GL_FRONT, GL_SHININESS, 0.0f);
        }
        break;
    }

    // 绘制球体
    GLUquadric* quadric = gluNewQuadric();          // 创建二次曲面对象
    gluQuadricNormals(quadric, GLU_SMOOTH);         // 设置平滑法向量
    gluSphere(quadric, radius, 30, 30);             // 绘制球体（半径，经度分段数，纬度分段数）
    gluDeleteQuadric(quadric);                      // 删除二次曲面对象
    glPopMatrix();                                  // 恢复矩阵状态
}

// 主渲染函数
void renderScene() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);  // 清除颜色和深度缓冲区

    // 计算相机位置 - 使用球坐标系计算相机在世界空间中的位置
    // camX = distance * sin(angleY) * cos(angleX)
    // camY = distance * sin(angleX)
    // camZ = distance * cos(angleY) * cos(angleX)
    float camX = camera.distance * static_cast<float>(sin(camera.angleY) * cos(camera.angleX));
    float camY = camera.distance * static_cast<float>(sin(camera.angleX));
    float camZ = camera.distance * static_cast<float>(cos(camera.angleY) * cos(camera.angleX));

    glLoadIdentity();  // 重置模型视图矩阵
    // 设置相机：眼睛位置，观察点，上向量
    gluLookAt(camX, camY, camZ,        // 相机位置
        0.0f, 0.0f, 0.0f,        // 观察点（场景中心）
        0.0f, 1.0f, 0.0f);       // 上向量（Y轴正方向）

    // 更新光源属性
    glLightfv(GL_LIGHT0, GL_POSITION, light.position);  // 设置光源位置
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light.diffuse);    // 设置漫反射光颜色
    glLightfv(GL_LIGHT0, GL_SPECULAR, light.specular);  // 设置镜面反射光颜色

    // 渲染所有球体（3行×4列）
    for (int row = 0; row < 3; row++) {
        for (int col = 0; col < 4; col++) {
            renderSphereAt(
                spherePositions[row][col][0],  // X坐标
                spherePositions[row][col][1],  // Y坐标
                spherePositions[row][col][2],  // Z坐标
                0.5f,                          // 半径
                row + 1,                       // 行号（1-based）
                col + 1                        // 列号（1-based）
            );
        }
    }

    glutSwapBuffers();  // 交换前后缓冲区（双缓冲）
}

// 键盘控制 - 灯光颜色和位置
void handleKeyboard(unsigned char key, int x, int y) {
    switch (key) {
        // Z轴移动
    case 'q': light.position[2] -= light.moveSpeed; break;  // 远离观察者
    case 'w': light.position[2] += light.moveSpeed; break;  // 靠近观察者

        // RGB颜色控制
    case 'e': light.diffuse[0] = fminf(1.0f, light.diffuse[0] + light.colorStep); break;  // 增加红色
    case 'r': light.diffuse[0] = fmaxf(0.0f, light.diffuse[0] - light.colorStep); break;  // 减少红色
    case 't': light.diffuse[1] = fminf(1.0f, light.diffuse[1] + light.colorStep); break;  // 增加绿色
    case 'y': light.diffuse[1] = fmaxf(0.0f, light.diffuse[1] - light.colorStep); break;  // 减少绿色
    case 'u': light.diffuse[2] = fminf(1.0f, light.diffuse[2] + light.colorStep); break;  // 增加蓝色
    case 'i': light.diffuse[2] = fmaxf(0.0f, light.diffuse[2] - light.colorStep); break;  // 减少蓝色
    }
    glutPostRedisplay();  // 标记需要重新绘制
}

// 特殊键控制 - 灯光位置
void handleSpecialKeys(int key, int x, int y) {
    switch (key) {
    case GLUT_KEY_UP:    light.position[1] += light.moveSpeed; break;   // 上移
    case GLUT_KEY_DOWN:  light.position[1] -= light.moveSpeed; break;   // 下移
    case GLUT_KEY_LEFT:  light.position[0] -= light.moveSpeed; break;   // 左移
    case GLUT_KEY_RIGHT: light.position[0] += light.moveSpeed; break;   // 右移
    }
    glutPostRedisplay();  // 标记需要重新绘制
}

// 鼠标移动 - 旋转视角
void handleMouseMotion(int x, int y) {
    // 计算鼠标移动的增量
    int deltaX = x - camera.lastMouseX;
    int deltaY = y - camera.lastMouseY;

    // 根据鼠标移动更新相机角度
    camera.angleY += deltaX * 0.01f;  // 水平移动控制偏航角
    camera.angleX -= deltaY * 0.01f;  // 垂直移动控制俯仰角

    // 限制垂直旋转角度（避免过度翻转）
    if (camera.angleX > PI / 2) camera.angleX = PI / 2;      // 最大向上角度
    if (camera.angleX < -PI / 2) camera.angleX = -PI / 2;    // 最大向下角度

    // 更新上一次鼠标位置
    camera.lastMouseX = x;
    camera.lastMouseY = y;

    glutPostRedisplay();  // 标记需要重新绘制
}

// 鼠标滚轮 - 缩放
void handleMouseWheel(int button, int dir, int x, int y) {
    if (dir > 0) {
        camera.distance = fmaxf(1.0f, camera.distance - 0.5f);  // 滚轮向上，拉近视角
    }
    else {
        camera.distance += 0.5f;  // 滚轮向下，拉远视角
    }
    glutPostRedisplay();  // 标记需要重新绘制
}

// 窗口大小调整
void handleReshape(int width, int height) {
    glViewport(0, 0, width, height);  // 设置视口为整个窗口

    glMatrixMode(GL_PROJECTION);      // 切换到投影矩阵
    glLoadIdentity();                 // 重置投影矩阵

    // 设置透视投影：视角45度，宽高比，近裁剪面1.0，远裁剪面100.0
    gluPerspective(45.0, static_cast<double>(width) / static_cast<double>(height), 1.0, 100.0);

    glMatrixMode(GL_MODELVIEW);       // 切换回模型视图矩阵
}

int main(int argc, char** argv) {
    // 初始化GLUT
    glutInit(&argc, argv);
    // 设置显示模式：双缓冲、RGB颜色、深度缓冲
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(800, 600);                    // 设置窗口大小
    glutCreateWindow("光照模型与材质");              // 创建窗口

    // 设置回调函数
    initializeGraphics();              // 初始化OpenGL设置
    glutDisplayFunc(renderScene);      // 设置显示回调函数
    glutReshapeFunc(handleReshape);    // 设置窗口重塑回调函数
    glutKeyboardFunc(handleKeyboard);  // 设置键盘回调函数
    glutSpecialFunc(handleSpecialKeys); // 设置特殊键回调函数
    glutMotionFunc(handleMouseMotion); // 设置鼠标移动回调函数
    glutMouseWheelFunc(handleMouseWheel); // 设置鼠标滚轮回调函数

    // 进入GLUT主循环
    glutMainLoop();
    return 0;
}