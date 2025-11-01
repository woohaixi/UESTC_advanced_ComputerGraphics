#define _CRT_SECURE_NO_WARNINGS
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <GL/glut.h>

// HSV和RGB颜色参数结构
typedef struct {
    int identifier;
    int pos_x, pos_y;
    float lower, upper;
    float current;
    float increment;
    const char* description;
    const char* display_fmt;
} param;

param hsv_params[3] = {
    { 1, 180, 80, 0.0, 1.0, 0.1, 0.01, "Hue component control.", "%.2f" },
    { 2, 240, 80, 0.0, 1.0, 0.1, 0.01, "Saturation component control.", "%.2f" },
    { 3, 300, 80, 0.0, 1.0, 0.1, 0.01, "Value component control.", "%.2f" },
};

param rgb_params[3] = {
    { 4, 180, 180, 0.0, 1.0, 0.1, 0.01, "Red component control.", "%.2f" },
    { 5, 240, 180, 0.0, 1.0, 0.1, 0.01, "Green component control.", "%.2f" },
    { 6, 300, 180, 0.0, 1.0, 0.1, 0.01, "Blue component control.", "%.2f" },
};

GLint active_param = 0;
GLuint main_win, hsv_win, rgb_win, ctrl_win;
GLuint win_width = 256, win_height = 256;

// 立方体旋转控制变量
static GLfloat rotation[] = { 15.0, 15.0, 0.0 };
static GLint rot_axis = 2;
static int mouse_x, mouse_y;
static int mouse_button = -1;

// 自动旋转控制变量
static int auto_rotate = 1;  // 1=开启自动旋转, 0=关闭
static float auto_rotate_speed = 0.5;  // 自动旋转速度
static float last_time = 0.0;

GLvoid* current_font = GLUT_BITMAP_TIMES_ROMAN_10;

void refresh_all(void);
void render_triangle(float x_pos, float y_pos, float scale, float red, float green, float blue);
void convert_hsv_rgb(float hue, float sat, float val, float* red, float* green, float* blue);
void convert_rgb_hsv(float red, float green, float blue, float* hue, float* sat, float* val);
void render_rgb_cube(void);
void draw_rgb_point(float r, float g, float b);
void animate_cube(void);

void select_font(const char* font_name, int font_size) {
    current_font = GLUT_BITMAP_HELVETICA_10;
    if (strcmp(font_name, "helvetica") == 0) {
        if (font_size == 12) current_font = GLUT_BITMAP_HELVETICA_12;
        else if (font_size == 18) current_font = GLUT_BITMAP_HELVETICA_18;
    }
    else if (strcmp(font_name, "times roman") == 0) {
        current_font = GLUT_BITMAP_TIMES_ROMAN_10;
        if (font_size == 24) current_font = GLUT_BITMAP_TIMES_ROMAN_24;
    }
    else if (strcmp(font_name, "8x13") == 0) {
        current_font = GLUT_BITMAP_8_BY_13;
    }
    else if (strcmp(font_name, "9x15") == 0) {
        current_font = GLUT_BITMAP_9_BY_15;
    }
}

void output_text(GLuint x_coord, GLuint y_coord, const char* fmt, ...) {
    va_list arguments;
    char buf[255], * ptr;

    va_start(arguments, fmt);
    vsnprintf(buf, sizeof(buf), fmt, arguments);
    va_end(arguments);

    glRasterPos2i(x_coord, y_coord);
    for (ptr = buf; *ptr; ptr++)
        glutBitmapCharacter(current_font, *ptr);
}

void render_param(param* p) {
    glColor3f(0.0, 1.0, 0.5);
    if (active_param == p->identifier) {
        glColor3f(1.0, 1.0, 0.0);
        output_text(10, 240, p->description);
        glColor3f(1.0, 0.0, 0.0);
    }

    output_text(p->pos_x, p->pos_y, p->display_fmt, p->current);
}

int detect_param_hit(param* p, int mouse_x, int mouse_y) {
    if (mouse_x > p->pos_x && mouse_x < p->pos_x + 60 &&
        mouse_y > p->pos_y - 30 && mouse_y < p->pos_y + 10)
        return p->identifier;
    return 0;
}

void adjust_param(param* p, int delta) {
    if (active_param != p->identifier) return;

    p->current += delta * p->increment;

    if (p->current < p->lower) p->current = p->lower;
    else if (p->current > p->upper) p->current = p->upper;
}

void main_window_resize(int w, int h) {
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, w, h, 0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

#define SPACE 25
    win_width = (w - SPACE * 3) / 2.0;
    win_height = (h - SPACE * 3) / 2.0;

    glutSetWindow(hsv_win);
    glutPositionWindow(SPACE, SPACE);
    glutReshapeWindow(win_width, win_height);
    glutSetWindow(rgb_win);
    glutPositionWindow(SPACE + win_width + SPACE, SPACE);
    glutReshapeWindow(win_width, win_height);
    glutSetWindow(ctrl_win);
    glutPositionWindow(SPACE, SPACE + win_height + SPACE);
    glutReshapeWindow(win_width + SPACE + win_width, win_height);
}

void main_window_render(void) {
    glClearColor(0.8, 0.8, 0.8, 0.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glColor3f(0.0, 0.0, 0.0);
    select_font("helvetica", 12);
    output_text(SPACE, SPACE - 5, "HSV Display");
    output_text(SPACE + win_width + SPACE, SPACE - 5, "RGB Display");
    output_text(SPACE, SPACE + win_height + SPACE - 5, "Control Panel");
    glutSwapBuffers();
}

void ctrl_window_resize(int w, int h) {
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, w, h, 0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glClearColor(0.0, 0.0, 0.0, 0.0);
}

void ctrl_window_render(void) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glColor3f(1.0, 1.0, 1.0);

    select_font("helvetica", 18);

    output_text(180, hsv_params[0].pos_y - 40, "Hue");
    output_text(230, hsv_params[0].pos_y - 40, "Sat");
    output_text(300, hsv_params[0].pos_y - 40, "Val");
    output_text(40, hsv_params[0].pos_y, "HSV(");
    output_text(230, hsv_params[0].pos_y, ",");
    output_text(290, hsv_params[0].pos_y, ",");
    output_text(350, hsv_params[0].pos_y, ")");

    output_text(180, rgb_params[0].pos_y - 40, "Red");
    output_text(230, rgb_params[0].pos_y - 40, "Green");
    output_text(300, rgb_params[0].pos_y - 40, "Blue");
    output_text(40, rgb_params[0].pos_y, "RGB(");
    output_text(230, rgb_params[0].pos_y, ",");
    output_text(290, rgb_params[0].pos_y, ",");
    output_text(350, rgb_params[0].pos_y, ")");

    render_param(&hsv_params[0]);
    render_param(&hsv_params[1]);
    render_param(&hsv_params[2]);
    render_param(&rgb_params[0]);
    render_param(&rgb_params[1]);
    render_param(&rgb_params[2]);

    // 显示自动旋转状态
    glColor3f(auto_rotate ? 0.0 : 1.0, auto_rotate ? 1.0 : 0.0, 0.0);
    output_text(400, 80, "Auto Rotate: %s", auto_rotate ? "ON" : "OFF");

    glutSwapBuffers();
}

void hsv_window_resize(int w, int h) {
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, w, h, 0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHT0);
}

void hsv_window_render(void) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    float rgb_colors[3];
    convert_hsv_rgb(hsv_params[0].current, hsv_params[1].current, hsv_params[2].current, 
                   &rgb_colors[0], &rgb_colors[1], &rgb_colors[2]);
    render_triangle(50, 50, 100, rgb_colors[0], rgb_colors[1], rgb_colors[2]);

    glutSwapBuffers();
}

void rgb_window_resize(int w, int h) {
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    
    if (w <= h)
        glOrtho(-2.0, 2.0, -2.0 * (GLfloat)h / (GLfloat)w,
            2.0 * (GLfloat)h / (GLfloat)w, -10.0, 10.0);
    else
        glOrtho(-2.0 * (GLfloat)w / (GLfloat)h,
            2.0 * (GLfloat)w / (GLfloat)h, -2.0, 2.0, -10.0, 10.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glEnable(GL_DEPTH_TEST);
}

void rgb_window_mouse(int button, int state, int mx, int my) {
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        rot_axis = 0;
        mouse_button = 0;
    }
    else if (button == GLUT_MIDDLE_BUTTON && state == GLUT_DOWN) {
        rot_axis = 1;
        mouse_button = 1;
    }
    else if (button == GLUT_RIGHT_BUTTON && state == GLUT_DOWN) {
        rot_axis = 2;
        mouse_button = 2;
    }
    else if (state == GLUT_UP) {
        mouse_button = -1;
    }
    
    mouse_x = mx;
    mouse_y = my;
    glutPostRedisplay();
}

void rgb_window_motion(int mx, int my) {
    if (mouse_button >= 0) {
        // 鼠标拖拽时暂停自动旋转
        auto_rotate = 0;
        
        // 更流畅的旋转控制
        float sensitivity = 0.5;
        rotation[rot_axis] += (mx - mouse_x) * sensitivity;
        rotation[(rot_axis + 1) % 3] += (my - mouse_y) * sensitivity;
        
        // 保持角度在合理范围内
        for (int i = 0; i < 3; i++) {
            if (rotation[i] > 360.0) rotation[i] -= 360.0;
            if (rotation[i] < 0.0) rotation[i] += 360.0;
        }
        
        mouse_x = mx;
        mouse_y = my;
        glutPostRedisplay();
    }
}

void rgb_window_render(void) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glLoadIdentity();
    
    // 应用旋转
    glRotatef(rotation[0], 1.0, 0.0, 0.0);
    glRotatef(rotation[1], 0.0, 1.0, 0.0);
    glRotatef(rotation[2], 0.0, 0.0, 1.0);

    // 渲染RGB立方体
    render_rgb_cube();
    
    // 标记当前RGB值对应的点
    draw_rgb_point(rgb_params[0].current, rgb_params[1].current, rgb_params[2].current);

    glutSwapBuffers();
}

void animate_cube(void) {
    if (auto_rotate) {
        // 自动旋转立方体
        rotation[0] += auto_rotate_speed;
        rotation[1] += auto_rotate_speed * 0.7;
        rotation[2] += auto_rotate_speed * 0.3;
        
        // 保持角度在合理范围内
        for (int i = 0; i < 3; i++) {
            if (rotation[i] > 360.0) rotation[i] -= 360.0;
        }
        
        glutPostRedisplay();
    }
}

void render_rgb_cube(void) {
    // 使用点云方式渲染RGB颜色立方体 (参考lab1_2的实现)
    float increment = 0.05;
    glPointSize(2.0);
    glBegin(GL_POINTS);
    for (float x_coord = -1.0; x_coord <= 1.0; x_coord += increment)
    {
        for (float y_coord = -1.0; y_coord <= 1.0; y_coord += increment)
        {
            for (float z_coord = -1.0; z_coord <= 1.0; z_coord += increment)
            {
                // 将立方体坐标[-1,1]映射到RGB值[0,1]
                float r = (x_coord + 1.0) / 2.0;
                float g = (y_coord + 1.0) / 2.0;
                float b = (z_coord + 1.0) / 2.0;
                
                glColor3f(r, g, b);
                glVertex3f(x_coord, y_coord, z_coord);
            }
        }
    }
    glEnd();
    
    // 绘制立方体边框
    glColor3f(0.8, 0.8, 0.8);
    glLineWidth(2.0);
    glBegin(GL_LINES);
    
    // 绘制立方体的12条边
    // 底面的4条边
    glVertex3f(-1.0, -1.0, -1.0); glVertex3f(1.0, -1.0, -1.0);
    glVertex3f(1.0, -1.0, -1.0); glVertex3f(1.0, -1.0, 1.0);
    glVertex3f(1.0, -1.0, 1.0); glVertex3f(-1.0, -1.0, 1.0);
    glVertex3f(-1.0, -1.0, 1.0); glVertex3f(-1.0, -1.0, -1.0);
    
    // 顶面的4条边
    glVertex3f(-1.0, 1.0, -1.0); glVertex3f(1.0, 1.0, -1.0);
    glVertex3f(1.0, 1.0, -1.0); glVertex3f(1.0, 1.0, 1.0);
    glVertex3f(1.0, 1.0, 1.0); glVertex3f(-1.0, 1.0, 1.0);
    glVertex3f(-1.0, 1.0, 1.0); glVertex3f(-1.0, 1.0, -1.0);
    
    // 连接底面和顶面的4条边
    glVertex3f(-1.0, -1.0, -1.0); glVertex3f(-1.0, 1.0, -1.0);
    glVertex3f(1.0, -1.0, -1.0); glVertex3f(1.0, 1.0, -1.0);
    glVertex3f(1.0, -1.0, 1.0); glVertex3f(1.0, 1.0, 1.0);
    glVertex3f(-1.0, -1.0, 1.0); glVertex3f(-1.0, 1.0, 1.0);
    
    glEnd();
    glLineWidth(1.0);
}

void draw_rgb_point(float r, float g, float b) {
    // 将RGB值从[0,1]映射到立方体坐标[-1,1]
    float x = 2.0 * r - 1.0;
    float y = 2.0 * g - 1.0;
    float z = 2.0 * b - 1.0;
    
    // 绘制一个更大的标记点
    glPointSize(12.0);
    glColor3f(1.0, 1.0, 0.0); // 黄色标记点
    glBegin(GL_POINTS);
    glVertex3f(x, y, z);
    glEnd();
    
    // 绘制标记点的边框（黑色）
    glPointSize(16.0);
    glColor3f(0.0, 0.0, 0.0);
    glBegin(GL_POINTS);
    glVertex3f(x, y, z);
    glEnd();
    
    // 重新绘制黄色中心点
    glPointSize(12.0);
    glColor3f(1.0, 1.0, 0.0);
    glBegin(GL_POINTS);
    glVertex3f(x, y, z);
    glEnd();
    
    // 绘制连接原点的线
    glLineWidth(4.0);
    glColor3f(1.0, 1.0, 0.0);
    glBegin(GL_LINES);
    glVertex3f(0.0, 0.0, 0.0);
    glVertex3f(x, y, z);
    glEnd();
    glLineWidth(1.0);
    
    // 绘制坐标轴标记
    glLineWidth(2.0);
    glBegin(GL_LINES);
    // X轴 (红色)
    glColor3f(0.8, 0.2, 0.2);
    glVertex3f(-1.0, 0.0, 0.0);
    glVertex3f(1.0, 0.0, 0.0);
    // Y轴 (绿色)
    glColor3f(0.2, 0.8, 0.2);
    glVertex3f(0.0, -1.0, 0.0);
    glVertex3f(0.0, 1.0, 0.0);
    // Z轴 (蓝色)
    glColor3f(0.2, 0.2, 0.8);
    glVertex3f(0.0, 0.0, -1.0);
    glVertex3f(0.0, 0.0, 1.0);
    glEnd();
    glLineWidth(1.0);
}

// 键盘事件处理
void keyboard(unsigned char key, int x, int y) {
    switch (key) {
        case 'a':
        case 'A':
            auto_rotate = !auto_rotate;  // 切换自动旋转
            refresh_all();
            break;
        case 'r':
        case 'R':
            rotation[0] = rotation[1] = rotation[2] = 0.0;  // 重置旋转
            refresh_all();
            break;
        case 27:  // ESC键退出
            exit(0);
            break;
    }
}

// 特殊键事件处理（用于确保键盘事件被捕获）
void special_keys(int key, int x, int y) {
    switch (key) {
        case GLUT_KEY_F1:
            auto_rotate = !auto_rotate;  // F1切换自动旋转
            refresh_all();
            break;
        case GLUT_KEY_F2:
            rotation[0] = rotation[1] = rotation[2] = 0.0;  // F2重置旋转
            refresh_all();
            break;
    }
}

int prev_mouse_y;

void ctrl_mouse_handler(int btn, int st, int mx, int my) {
    active_param = 0;
    if (st == GLUT_DOWN) {
        // 检查是否点击了自动旋转控制区域
        if (mx >= 400 && mx <= 500 && my >= 60 && my <= 100) {
            auto_rotate = !auto_rotate;
            refresh_all();
            return;
        }
        
        active_param += detect_param_hit(&hsv_params[0], mx, my);
        active_param += detect_param_hit(&hsv_params[1], mx, my);
        active_param += detect_param_hit(&hsv_params[2], mx, my);
        active_param += detect_param_hit(&rgb_params[0], mx, my);
        active_param += detect_param_hit(&rgb_params[1], mx, my);
        active_param += detect_param_hit(&rgb_params[2], mx, my);
    }

    prev_mouse_y = my;
    refresh_all();
}

void ctrl_motion_handler(int mx, int my) {
    if (active_param > 0) {
        adjust_param(&hsv_params[0], prev_mouse_y - my);
        adjust_param(&hsv_params[1], prev_mouse_y - my);
        adjust_param(&hsv_params[2], prev_mouse_y - my);
        adjust_param(&rgb_params[0], prev_mouse_y - my);
        adjust_param(&rgb_params[1], prev_mouse_y - my);
        adjust_param(&rgb_params[2], prev_mouse_y - my);

        // 如果修改了HSV参数，更新RGB参数
        if (active_param >= 1 && active_param <= 3) {
            float rgb_colors[3];
            convert_hsv_rgb(hsv_params[0].current, hsv_params[1].current, hsv_params[2].current,
                           &rgb_colors[0], &rgb_colors[1], &rgb_colors[2]);
            rgb_params[0].current = rgb_colors[0];
            rgb_params[1].current = rgb_colors[1];
            rgb_params[2].current = rgb_colors[2];
        }
        // 如果修改了RGB参数，更新HSV参数
        else if (active_param >= 4 && active_param <= 6) {
            float hsv_colors[3];
            convert_rgb_hsv(rgb_params[0].current, rgb_params[1].current, rgb_params[2].current,
                           &hsv_colors[0], &hsv_colors[1], &hsv_colors[2]);
            hsv_params[0].current = hsv_colors[0];
            hsv_params[1].current = hsv_colors[1];
            hsv_params[2].current = hsv_colors[2];
        }

        prev_mouse_y = my;
        refresh_all();
    }
}

void convert_hsv_rgb(float h, float s, float v, float* r, float* g, float* b) {
    int sector;
    float frac, p_val, q_val, t_val;
    if (s == 0) {
        *r = *g = *b = v;
        return;
    }
    if (h == 1.0) h = 0;
    else h *= 6;
    sector = floor(h);
    frac = h - sector;
    p_val = v * (1 - s);
    q_val = v * (1 - s * frac);
    t_val = v * (1 - s * (1 - frac));
    switch (sector % 6) {
    case 0: *r = v; *g = t_val; *b = p_val; break;
    case 1: *r = q_val; *g = v; *b = p_val; break;
    case 2: *r = p_val; *g = v; *b = t_val; break;
    case 3: *r = p_val; *g = q_val; *b = v; break;
    case 4: *r = t_val; *g = p_val; *b = v; break;
    case 5: *r = v; *g = p_val; *b = q_val; break;
    }
}

void convert_rgb_hsv(float r, float g, float b, float* h, float* s, float* v) {
    float max_val = fmaxf(fmaxf(r, g), b);
    float min_val = fminf(fminf(r, g), b);
    float delta = max_val - min_val;
    
    *v = max_val;
    
    if (delta == 0.0f) {
        *h = *s = 0.0f;
        return;
    }
    
    *s = delta / max_val;
    
    if (max_val == r) {
        *h = fmodf((g - b) / delta + 6.0f, 6.0f) / 6.0f;
    } else if (max_val == g) {
        *h = ((b - r) / delta + 2.0f) / 6.0f;
    } else {
        *h = ((r - g) / delta + 4.0f) / 6.0f;
    }
}

void render_triangle(float x_pos, float y_pos, float scale, float red, float green, float blue) {
    glColor3f(red, green, blue);
    glBegin(GL_TRIANGLES);
    glVertex2f(x_pos, y_pos);
    glVertex2f(x_pos + scale, y_pos);
    glVertex2f(x_pos + scale / 2, y_pos + scale * sqrt(3) / 2);
    glEnd();
}

void refresh_all(void) {
    glutSetWindow(ctrl_win);
    glutPostRedisplay();
    glutSetWindow(hsv_win);
    glutPostRedisplay();
    glutSetWindow(rgb_win);
    glutPostRedisplay();
}

int main(int argc, char** argv) {
    glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE);
    glutInitWindowSize(512 + 25 * 3, 512 + 25 * 3);
    glutInitWindowPosition(250, 50);
    glutInit(&argc, argv);

    main_win = glutCreateWindow("HSV-RGB Color Model Converter - Auto Rotating 3D RGB Cube");
    glutReshapeFunc(main_window_resize);
    glutDisplayFunc(main_window_render);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(special_keys);

    hsv_win = glutCreateSubWindow(main_win, 25, 25, 256, 256);
    glutReshapeFunc(hsv_window_resize);
    glutDisplayFunc(hsv_window_render);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(special_keys);

    rgb_win = glutCreateSubWindow(main_win, 25 + 256 + 25, 25, 256, 256);
    glutReshapeFunc(rgb_window_resize);
    glutDisplayFunc(rgb_window_render);
    glutMouseFunc(rgb_window_mouse);
    glutMotionFunc(rgb_window_motion);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(special_keys);

    ctrl_win = glutCreateSubWindow(main_win, 25, 25 + 256 + 25, 512 + 25, 256);
    glutReshapeFunc(ctrl_window_resize);
    glutDisplayFunc(ctrl_window_render);
    glutMotionFunc(ctrl_motion_handler);
    glutMouseFunc(ctrl_mouse_handler);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(special_keys);

    // 设置空闲回调函数，用于自动旋转
    glutIdleFunc(animate_cube);

    refresh_all();

    glutMainLoop();

    return 0;
}
