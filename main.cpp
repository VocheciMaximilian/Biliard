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

float cueBallX = -180.0f;
float cueBallY = 0.0f;
const float BALL_RADIUS = 12.0f;

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

    glutSwapBuffers();
    glFlush();
}

void ProcessSpecialKeys(int key, int x, int y)
{
    float speed = 8.0f;
    float halfW = (TABLE_WIDTH / 2.0f) - BALL_RADIUS;
    float halfH = (TABLE_HEIGHT / 2.0f) - BALL_RADIUS;

    switch (key)
    {
    case GLUT_KEY_LEFT:
        cueBallX -= speed;
        if (cueBallX < -halfW) cueBallX = -halfW;
        break;
    case GLUT_KEY_RIGHT:
        cueBallX += speed;
        if (cueBallX > halfW) cueBallX = halfW;
        break;
    case GLUT_KEY_UP:
        cueBallY += speed;
        if (cueBallY > halfH) cueBallY = halfH;
        break;
    case GLUT_KEY_DOWN:
        cueBallY -= speed;
        if (cueBallY < -halfH) cueBallY = -halfH;
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
    glutKeyboardFunc(ProcessNormalKeys);
    glutCloseFunc(Cleanup);

    glutMainLoop();

    return 0;
}
