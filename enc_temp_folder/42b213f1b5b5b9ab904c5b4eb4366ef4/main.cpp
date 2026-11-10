#include <iostream>
#include <vector>
#include <cmath>
#include <windows.h>
#include <GL/glew.h>
#include <GL/freeglut.h>

#include "loadShaders.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

using namespace std;

const float PI = 3.14159265358979323846f;

int winWidth = 1000;
int winHeight = 600;

float xMin = -500.0f, xMax = 500.0f;
float yMin = -300.0f, yMax = 300.0f;

GLuint ProgramId, myMatrixLocation, codColLocation, vaoRect, vboRect, vaoCircle, vboCircle;
const int CIRCLE_SEGMENTS = 50;

glm::mat4 projMatrix;
glm::mat4 modelMatrix;
glm::mat4 myMatrix;

const float TABLE_WIDTH = 760.0f;
const float TABLE_HEIGHT = 380.0f;
const float BORDER_SIZE = 35.0f;
const float POCKET_RADIUS = 20.0f;
const float SLIDER_X = 0.0f;
const float SLIDER_Y = -250.0f;
const float SLIDER_WIDTH = 200.0f;
const float SLIDER_HEIGHT = 20.0f;
const float MIN_POWER = 1.0f;
const float MAX_POWER = 35.0f;

float powerPercent = 0.5f;
bool isDraggingSlider = false;

float cueBallX = -180.0f;
float cueBallY = 0.0f;
const float BALL_RADIUS = 12.0f;

float ballVx = 0.0f;
float ballVy = 0.0f;

const float FRICTION = 0.98f;
const float CUE_LENGTH = 220.0f;
const float CUE_WIDTH = 4.0f;

float cueAngle = 0.0f;
float mouseWorldX = 0.0f;
float mouseWorldY = 0.0f;
bool isAiming = true;

struct Pocket {
    float x, y;
};
Pocket pockets[6];

void SetupPockets()
{
    float halfW = TABLE_WIDTH / 2.0f;
    float halfH = TABLE_HEIGHT / 2.0f;

    pockets[0] = { -halfW + 6.0f,  halfH - 6.0f };
    pockets[1] = {  halfW - 6.0f,  halfH - 6.0f };
    pockets[2] = { -halfW + 6.0f, -halfH + 6.0f };
    pockets[3] = {  halfW - 6.0f, -halfH + 6.0f };

    pockets[4] = { 0.0f,  halfH };
    pockets[5] = { 0.0f, -halfH };
}

void CreateGeometry()
{
    GLfloat rectVertices[] = {
        -0.5f, -0.5f,  0.0f,  1.0f,
         0.5f, -0.5f,  0.0f,  1.0f,
         0.5f,  0.5f,  0.0f,  1.0f,

        -0.5f, -0.5f,  0.0f,  1.0f,
         0.5f,  0.5f,  0.0f,  1.0f,
        -0.5f,  0.5f,  0.0f,  1.0f
    };

    glGenVertexArrays(1, &vaoRect);
    glBindVertexArray(vaoRect);

    glGenBuffers(1, &vboRect);
    glBindBuffer(GL_ARRAY_BUFFER, vboRect);
    glBufferData(GL_ARRAY_BUFFER, sizeof(rectVertices), rectVertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (GLvoid*)0);

    vector<GLfloat> circleVertices;
    circleVertices.push_back(0.0f);
    circleVertices.push_back(0.0f);
    circleVertices.push_back(0.0f);
    circleVertices.push_back(1.0f);

    for (int i = 0; i <= CIRCLE_SEGMENTS; i++)
    {
        float angle = 2.0f * PI * (float)i / (float)CIRCLE_SEGMENTS;
        float x = cosf(angle);
        float y = sinf(angle);
        circleVertices.push_back(x);
        circleVertices.push_back(y);
        circleVertices.push_back(0.0f);
        circleVertices.push_back(1.0f);
    }

    glGenVertexArrays(1, &vaoCircle);
    glBindVertexArray(vaoCircle);

    glGenBuffers(1, &vboCircle);
    glBindBuffer(GL_ARRAY_BUFFER, vboCircle);
    glBufferData(GL_ARRAY_BUFFER, circleVertices.size() * sizeof(GLfloat), circleVertices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (GLvoid*)0);

    glBindVertexArray(0);
}

void CreateShaders(void)
{
    ProgramId = LoadShaders("example.vert", "example.frag");
    glUseProgram(ProgramId);

    myMatrixLocation = glGetUniformLocation(ProgramId, "myMatrix");
    codColLocation = glGetUniformLocation(ProgramId, "codCol");
}

void Initialize(void)
{
    glClearColor(0.12f, 0.14f, 0.16f, 1.0f);

    SetupPockets();
    CreateGeometry();
    CreateShaders();
}

void RenderFunction(void)
{
    glClear(GL_COLOR_BUFFER_BIT);
    projMatrix = glm::ortho(xMin, xMax, yMin, yMax);

    glBindVertexArray(vaoRect);
    glUniform1i(codColLocation, 1);

    float frameWidth = TABLE_WIDTH + 2.0f * BORDER_SIZE;
    float frameHeight = TABLE_HEIGHT + 2.0f * BORDER_SIZE;
    modelMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(frameWidth, frameHeight, 1.0f));
    myMatrix = projMatrix * modelMatrix;
    glUniformMatrix4fv(myMatrixLocation, 1, GL_FALSE, glm::value_ptr(myMatrix));
    glDrawArrays(GL_TRIANGLES, 0, 6);

    glUniform1i(codColLocation, 2);
    modelMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(TABLE_WIDTH, TABLE_HEIGHT, 1.0f));
    myMatrix = projMatrix * modelMatrix;
    glUniformMatrix4fv(myMatrixLocation, 1, GL_FALSE, glm::value_ptr(myMatrix));
    glDrawArrays(GL_TRIANGLES, 0, 6);

    glBindVertexArray(vaoCircle);
    glUniform1i(codColLocation, 3);

    for (int i = 0; i < 6; i++)
    {
        glm::mat4 transl = glm::translate(glm::mat4(1.0f), glm::vec3(pockets[i].x, pockets[i].y, 0.0f));
        glm::mat4 sc = glm::scale(glm::mat4(1.0f), glm::vec3(POCKET_RADIUS, POCKET_RADIUS, 1.0f));
        modelMatrix = transl * sc;
        myMatrix = projMatrix * modelMatrix;
        glUniformMatrix4fv(myMatrixLocation, 1, GL_FALSE, glm::value_ptr(myMatrix));
        glDrawArrays(GL_TRIANGLE_FAN, 0, CIRCLE_SEGMENTS + 2);
    }

    glUniform1i(codColLocation, 4);

    glm::mat4 translBall = glm::translate(glm::mat4(1.0f), glm::vec3(cueBallX, cueBallY, 0.0f));
    glm::mat4 scBall = glm::scale(glm::mat4(1.0f), glm::vec3(BALL_RADIUS, BALL_RADIUS, 1.0f));
    modelMatrix = translBall * scBall;
    myMatrix = projMatrix * modelMatrix;
    glUniformMatrix4fv(myMatrixLocation, 1, GL_FALSE, glm::value_ptr(myMatrix));
    glDrawArrays(GL_TRIANGLE_FAN, 0, CIRCLE_SEGMENTS + 2);

    glBindVertexArray(0);

	glBindVertexArray(vaoRect);

	glUniform1d(codColLocation, 5);
	glm::mat4 tSliderBg = glm::translate(glm::mat4(1.0f), glm::vec3(SLIDER_X, SLIDER_Y, 0.0f));
	glm::mat4 sSliderBg = glm::scale(glm::mat4(1.0f), glm::vec3(SLIDER_WIDTH, SLIDER_HEIGHT, 1.0f));
	myMatrix = projMatrix * tSliderBg * sSliderBg;
	glUniformMatrix4fv(myMatrixLocation, 1, GL_FALSE, glm::value_ptr(myMatrix));
	glDrawArrays(GL_TRIANGLES, 0, 6);
	float currentFillWidth = SLIDER_WIDTH * powerPercent;
    if(currentFillWidth > 0.0f)
    {
        glUniform1i(codColLocation, 6);
        glm::mat4 tSliderFill = glm::translate(glm::mat4(1.0f), glm::vec3(SLIDER_X - SLIDER_WIDTH / 2.0f + currentFillWidth / 2.0f, SLIDER_Y, 0.0f));
        glm::mat4 sSliderFill = glm::scale(glm::mat4(1.0f), glm::vec3(currentFillWidth, SLIDER_HEIGHT, 1.0f));
        myMatrix = projMatrix * tSliderFill * sSliderFill;
        glUniformMatrix4fv(myMatrixLocation, 1, GL_FALSE, glm::value_ptr(myMatrix));
        glDrawArrays(GL_TRIANGLES, 0, 6);
	}


    if (isAiming)
    {
        glBindVertexArray(vaoRect);
        glUniform1i(codColLocation, 1); // Culoare lemn pentru tac

        // 1. Translatie in centrul bilei
        glm::mat4 translBall = glm::translate(glm::mat4(1.0f), glm::vec3(cueBallX, cueBallY, 0.0f));
        // 2. Rotatie dupa unghiul mouse-ului
        glm::mat4 rotCue = glm::rotate(glm::mat4(1.0f), cueAngle, glm::vec3(0.0f, 0.0f, 1.0f));
        // 3. Retragere in spatele bilei (bila + jumatate de tac + distanta de tragere conform puterii)
        float cueOffset = BALL_RADIUS + (CUE_LENGTH / 2.0f) + 6.0f + (powerPercent * 20.0f);
        glm::mat4 tOffset = glm::translate(glm::mat4(1.0f), glm::vec3(-cueOffset, 0.0f, 0.0f));
        // 4. Scalare la dimensiunile tacului
        glm::mat4 scCue = glm::scale(glm::mat4(1.0f), glm::vec3(CUE_LENGTH, CUE_WIDTH, 1.0f));

        // Compunerea corecta a transformarilor
        modelMatrix = translBall * rotCue * tOffset * scCue;
        myMatrix = projMatrix * modelMatrix;
        glUniformMatrix4fv(myMatrixLocation, 1, GL_FALSE, glm::value_ptr(myMatrix));
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);
    }

    glutSwapBuffers();
    glFlush();
}

void ProcessSpecialKeys(int key, int x, int y)
{
    float step = 0.05f; // Pasul de ajustare a puterii (5%)

    switch (key)
    {
    case GLUT_KEY_LEFT:
    case GLUT_KEY_DOWN:
        powerPercent -= step;
        if (powerPercent < 0.05f) powerPercent = 0.05f;
        break;
    case GLUT_KEY_RIGHT:
    case GLUT_KEY_UP:
        powerPercent += step;
        if (powerPercent > 1.0f) powerPercent = 1.0f;
        break;
    }

    glutPostRedisplay();
}

void ProcessNormalKeys(unsigned char key, int x, int y)
{
    if (key == 27)
    {
        exit(0);
    }
    if (key == 'r' || key == 'R')
    {
        cueBallX = -180.0f;
        cueBallY = 0.0f;
        glutPostRedisplay();
    }
}

void Cleanup(void)
{
    glDeleteBuffers(1, &vboRect);
    glDeleteVertexArrays(1, &vaoRect);

    glDeleteBuffers(1, &vboCircle);
    glDeleteVertexArrays(1, &vaoCircle);

    glDeleteProgram(ProgramId);
}

void processMouseMotion(int x, int y)
{
    mouseWorldX = xMin + (xMax - xMin) * ((float)x / (float)winWidth);
    mouseWorldY = yMax - (yMax - yMin) * ((float)y / (float)winHeight);
    float dx = mouseWorldX - cueBallX;
    float dy = mouseWorldY - cueBallY;
    cueAngle = atan2f(dy, dx);
    glutPostRedisplay();
}
bool isMouseOverSlider(float mx, float my)
{
    float left = SLIDER_X - SLIDER_WIDTH / 2.0f;
    float right = SLIDER_X + SLIDER_WIDTH / 2.0f;
    float bottom = SLIDER_Y - SLIDER_HEIGHT / 2.0f - 10.0f;
    float top = SLIDER_Y + SLIDER_HEIGHT / 2.0f + 10.0f;

    return (mx >= left && mx <= right && my >= bottom && my <= top);
}
void processMouseClick(int button, int state, int x, int y)
{
    if (button == GLUT_LEFT_BUTTON)
    {
        if (state == GLUT_DOWN)
        {
            if (isMouseOverSlider(mouseWorldX, mouseWorldY))
            {
                isDraggingSlider = true;

                float leftEdge = SLIDER_X - SLIDER_WIDTH / 2.0f;
                powerPercent = (mouseWorldX - leftEdge) / SLIDER_WIDTH;
                if (powerPercent < 0.0f) powerPercent = 0.0f;
                if (powerPercent > 1.0f) powerPercent = 1.0f;

                glutPostRedisplay();
            }
            else if (isAiming)
            {
                float actualPower = MIN_POWER + powerPercent * (MAX_POWER - MIN_POWER);

                ballVx = cosf(cueAngle) * actualPower;
                ballVy = sinf(cueAngle) * actualPower;
                isAiming = false;
            }
        }
        else if (state == GLUT_UP)
        {
            isDraggingSlider = false;
        }
    }
}
void processActiveMotion(int x, int y)
{
    mouseWorldX = xMin + (xMax - xMin) * ((float)x / (float)winWidth);
    mouseWorldY = yMax - (yMax - yMin) * ((float)y / (float)winHeight);

    if (isDraggingSlider)
    {
        float leftEdge = SLIDER_X - SLIDER_WIDTH / 2.0f;
        powerPercent = (mouseWorldX - leftEdge) / SLIDER_WIDTH;
        if (powerPercent < 0.05f) powerPercent = 0.05f;
        if (powerPercent > 1.0f) powerPercent = 1.0f;

        glutPostRedisplay();
    }
}

void updatePhysics(int value) {
        if (!isAiming)
    {
        cueBallX += ballVx;
        cueBallY += ballVy;
        ballVx *= FRICTION;
        ballVy *= FRICTION;
        if (fabs(ballVx) < 0.01f) ballVx = 0.0f;
        if (fabs(ballVy) < 0.01f) ballVy = 0.0f;
        if (ballVx == 0.0f && ballVy == 0.0f)
        {
            isAiming = true;
        }
        float halfW = (TABLE_WIDTH / 2.0f) - BALL_RADIUS;
        float halfH = (TABLE_HEIGHT / 2.0f) - BALL_RADIUS;
        if (cueBallX < -halfW) { cueBallX = -halfW; ballVx = -ballVx; }
        if (cueBallX > halfW) { cueBallX = halfW; ballVx = -ballVx; }
        if (cueBallY < -halfH) { cueBallY = -halfH; ballVy = -ballVy; }
        if (cueBallY > halfH) { cueBallY = halfH; ballVy = -ballVy; }
        for (int i = 0; i < 6; i++)
        {
            float dx = cueBallX - pockets[i].x;
            float dy = cueBallY - pockets[i].y;
            float distanceSq = dx * dx + dy * dy;
            if (distanceSq < POCKET_RADIUS * POCKET_RADIUS)
            {
                cueBallX = -180.0f;
                cueBallY = 0.0f;
                ballVx = 0.0f;
                ballVy = 0.0f;
                isAiming = true;
                break;
            }
        }
        glutPostRedisplay();
		}
        glutTimerFunc(16, updatePhysics, 0);
}

int main(int argc, char* argv[])
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowPosition(100, 100);
    glutInitWindowSize(winWidth, winHeight);
    glutCreateWindow("Proiect Biliard 2D - Demo Masa si Bila Alba");

    glewInit();

    Initialize();

    glutDisplayFunc(RenderFunction);
    glutSpecialFunc(ProcessSpecialKeys);
    //glutKeyboardFunc(ProcessNormalKeys);
	glutMouseFunc(processMouseClick);
	glutPassiveMotionFunc(processMouseMotion);
	glutMotionFunc(processActiveMotion);
    glutTimerFunc(16, updatePhysics, 0);
    glutCloseFunc(Cleanup);

    glutMainLoop();

    return 0;
}
