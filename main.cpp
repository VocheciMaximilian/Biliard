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

const float BALL_RADIUS = 12.0f;
const float FRICTION = 0.98f;
const float CUE_LENGTH = 220.0f;
const float CUE_WIDTH = 4.0f;
const float DEFLECTION_GUIDE_LENGTH = 100.0f;
const float AIM_LINE_WIDTH = 2.0f;

float mouseWorldX = 0.0f;
float mouseWorldY = 0.0f;
bool isAiming = true;

struct Pocket {
    float x, y;
};
Pocket pockets[6];

void SetupRack();

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

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    SetupPockets();
    SetupRack();
    CreateGeometry();
    CreateShaders();
}

void DrawLineSegment(float x1, float y1, float x2, float y2, float thickness, int colorCode)
{
    float dx = x2 - x1;
    float dy = y2 - y1;
    float len = sqrtf(dx * dx + dy * dy);
    if (len < 0.001f) return;

    float angle = atan2f(dy, dx);
    float midX = (x1 + x2) * 0.5f;
    float midY = (y1 + y2) * 0.5f;

    glBindVertexArray(vaoRect);
    glUniform1i(codColLocation, colorCode);

    glm::mat4 transl = glm::translate(glm::mat4(1.0f), glm::vec3(midX, midY, 0.0f));
    glm::mat4 rot = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0.0f, 0.0f, 1.0f));
    glm::mat4 sc = glm::scale(glm::mat4(1.0f), glm::vec3(len, thickness, 1.0f));

    modelMatrix = transl * rot * sc;
    myMatrix = projMatrix * modelMatrix;
    glUniformMatrix4fv(myMatrixLocation, 1, GL_FALSE, glm::value_ptr(myMatrix));
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void DrawCircleShape(float cx, float cy, float radius, int colorCode)
{
    glBindVertexArray(vaoCircle);
    glUniform1i(codColLocation, colorCode);

    glm::mat4 transl = glm::translate(glm::mat4(1.0f), glm::vec3(cx, cy, 0.0f));
    glm::mat4 sc = glm::scale(glm::mat4(1.0f), glm::vec3(radius, radius, 1.0f));

    modelMatrix = transl * sc;
    myMatrix = projMatrix * modelMatrix;
    glUniformMatrix4fv(myMatrixLocation, 1, GL_FALSE, glm::value_ptr(myMatrix));
    glDrawArrays(GL_TRIANGLE_FAN, 0, CIRCLE_SEGMENTS + 2);
}

class Ball {
public:
    float x, y;
    float vx, vy;
    float startX, startY;
    float radius;
    int colorCode;
    bool isPocketed;

    Ball(float startX = 0.0f, float startY = 0.0f, float r = BALL_RADIUS, int color = 7)
        : x(startX), y(startY), vx(0.0f), vy(0.0f),
          startX(startX), startY(startY), radius(r), colorCode(color), isPocketed(false) {}

    virtual ~Ball() {}

    virtual void Render() const {
        if (isPocketed) return;
        DrawCircleShape(x, y, radius, colorCode);
    }

    virtual void UpdatePosition(float dt) {
        if (isPocketed) return;
        x += vx * dt;
        y += vy * dt;
    }

    virtual void ApplyFriction(float friction) {
        if (isPocketed) return;
        vx *= friction;
        vy *= friction;
    }

    void CheckCushions(float halfW, float halfH) {
        if (isPocketed) return;
        if (x < -halfW) { x = -halfW; vx = -vx; }
        if (x >  halfW) { x =  halfW; vx = -vx; }
        if (y < -halfH) { y = -halfH; vy = -vy; }
        if (y >  halfH) { y =  halfH; vy = -vy; }
    }

    virtual void CheckPockets(const Pocket pockets[], int numPockets, float pocketRadius) {
        if (isPocketed) return;
        for (int i = 0; i < numPockets; i++) {
            float dx = x - pockets[i].x;
            float dy = y - pockets[i].y;
            if (dx * dx + dy * dy < pocketRadius * pocketRadius) {
                isPocketed = true;
                vx = 0.0f;
                vy = 0.0f;
                x = -9999.0f;
                y = -9999.0f;
                break;
            }
        }
    }

    void CheckStop(float threshold = 0.01f) {
        if (isPocketed) {
            vx = 0.0f;
            vy = 0.0f;
            return;
        }
        if (fabs(vx) < threshold) vx = 0.0f;
        if (fabs(vy) < threshold) vy = 0.0f;
    }

    bool IsMoving() const {
        if (isPocketed) return false;
        return (vx != 0.0f || vy != 0.0f);
    }

    virtual void Reset() {
        x = startX;
        y = startY;
        vx = 0.0f;
        vy = 0.0f;
        isPocketed = false;
    }

    static void ResolveCollision(Ball& b1, Ball& b2) {
        if (b1.isPocketed || b2.isPocketed) return;

        float deltaX = b2.x - b1.x;
        float deltaY = b2.y - b1.y;
        float distSq = deltaX * deltaX + deltaY * deltaY;
        float minDist = b1.radius + b2.radius;

        if (distSq < minDist * minDist && distSq > 0.0001f) {
            float dist = sqrtf(distSq);
            float nx = deltaX / dist;
            float ny = deltaY / dist;

            float overlap = 0.5f * (minDist - dist);
            b1.x -= nx * overlap;
            b1.y -= ny * overlap;
            b2.x += nx * overlap;
            b2.y += ny * overlap;

            float relVx = b1.vx - b2.vx;
            float relVy = b1.vy - b2.vy;
            float impulse = relVx * nx + relVy * ny;

            if (impulse > 0.0f) {
                b1.vx -= impulse * nx;
                b1.vy -= impulse * ny;
                b2.vx += impulse * nx;
                b2.vy += impulse * ny;
            }
        }
    }
};

class CueBall : public Ball {
public:
    float cueAngle;

    CueBall(float startX = -180.0f, float startY = 0.0f, float r = BALL_RADIUS)
        : Ball(startX, startY, r, 4), cueAngle(0.0f) {}

    void Shoot(float power) {
        vx = cosf(cueAngle) * power;
        vy = sinf(cueAngle) * power;
    }

    void SetAimAngle(float targetX, float targetY) {
        float dx = targetX - x;
        float dy = targetY - y;
        cueAngle = atan2f(dy, dx);
    }

    virtual void CheckPockets(const Pocket pockets[], int numPockets, float pocketRadius) override {
        for (int i = 0; i < numPockets; i++) {
            float dx = x - pockets[i].x;
            float dy = y - pockets[i].y;
            if (dx * dx + dy * dy < pocketRadius * pocketRadius) {
                Reset();
                break;
            }
        }
    }

    void RenderCue(float powerPercent) const {
        glBindVertexArray(vaoRect);
        glUniform1i(codColLocation, 1);

        glm::mat4 translBall = glm::translate(glm::mat4(1.0f), glm::vec3(x, y, 0.0f));
        glm::mat4 rotCue = glm::rotate(glm::mat4(1.0f), cueAngle, glm::vec3(0.0f, 0.0f, 1.0f));
        float cueOffset = radius + (CUE_LENGTH / 2.0f) + 6.0f + (powerPercent * 20.0f);
        glm::mat4 tOffset = glm::translate(glm::mat4(1.0f), glm::vec3(-cueOffset, 0.0f, 0.0f));
        glm::mat4 scCue = glm::scale(glm::mat4(1.0f), glm::vec3(CUE_LENGTH, CUE_WIDTH, 1.0f));

        glm::mat4 model = translBall * rotCue * tOffset * scCue;
        glm::mat4 mvp = projMatrix * model;
        glUniformMatrix4fv(myMatrixLocation, 1, GL_FALSE, glm::value_ptr(mvp));
        glDrawArrays(GL_TRIANGLES, 0, 6);

        glBindVertexArray(0);
    }

    void RenderTrajectory(const std::vector<Ball>& otherBalls, float halfW, float halfH) const {
        float dirX = cosf(cueAngle);
        float dirY = sinf(cueAngle);

        float p0x = x;
        float p0y = y;

        float stepSize = 2.0f;
        float currentX = p0x;
        float currentY = p0y;

        const Ball* hitBall = nullptr;
        bool hitWall = false;
        bool hitVerticalWall = false;

        while (true) {
            currentX += dirX * stepSize;
            currentY += dirY * stepSize;

            if (currentX <= -halfW || currentX >= halfW) {
                hitWall = true;
                hitVerticalWall = true;
                if (currentX < -halfW) currentX = -halfW;
                if (currentX > halfW) currentX = halfW;
                break;
            }
            if (currentY <= -halfH || currentY >= halfH) {
                hitWall = true;
                hitVerticalWall = false;
                if (currentY < -halfH) currentY = -halfH;
                if (currentY > halfH) currentY = halfH;
                break;
            }

            for (const auto& target : otherBalls) {
                if (target.isPocketed) continue;

                float dx = currentX - target.x;
                float dy = currentY - target.y;
                float minDist = radius + target.radius;

                if (dx * dx + dy * dy <= minDist * minDist) {
                    hitBall = &target;
                    break;
                }
            }

            if (hitBall != nullptr) {
                break;
            }
        }

        DrawLineSegment(p0x, p0y, currentX, currentY, AIM_LINE_WIDTH, 5);
        DrawCircleShape(currentX, currentY, radius, 8);

        if (hitBall != nullptr) {
            float nx = hitBall->x - currentX;
            float ny = hitBall->y - currentY;
            float nLen = sqrtf(nx * nx + ny * ny);

            if (nLen > 0.0001f) {
                nx /= nLen;
                ny /= nLen;

                float targetEndX = hitBall->x + nx * DEFLECTION_GUIDE_LENGTH;
                float targetEndY = hitBall->y + ny * DEFLECTION_GUIDE_LENGTH;
                DrawLineSegment(hitBall->x, hitBall->y, targetEndX, targetEndY, AIM_LINE_WIDTH, 5);

                float tx = -ny;
                float ty = nx;
                if (dirX * tx + dirY * ty < 0.0f) {
                    tx = -tx;
                    ty = -ty;
                }

                float cueEndX = currentX + tx * DEFLECTION_GUIDE_LENGTH;
                float cueEndY = currentY + ty * DEFLECTION_GUIDE_LENGTH;
                DrawLineSegment(currentX, currentY, cueEndX, cueEndY, AIM_LINE_WIDTH, 5);
            }
        }
        else if (hitWall) {
            float reflX = hitVerticalWall ? -dirX : dirX;
            float reflY = hitVerticalWall ? dirY : -dirY;

            float reflEndX = currentX + reflX * DEFLECTION_GUIDE_LENGTH;
            float reflEndY = currentY + reflY * DEFLECTION_GUIDE_LENGTH;
            DrawLineSegment(currentX, currentY, reflEndX, reflEndY, AIM_LINE_WIDTH, 5);
        }
    }

    void RenderTrajectory(const Ball& otherBall, float halfW, float halfH) const {
        std::vector<Ball> single;
        single.push_back(otherBall);
        RenderTrajectory(single, halfW, halfH);
    }
};

CueBall cueBall(-180.0f, 0.0f);
std::vector<Ball> redBalls;

void SetupRack()
{
    redBalls.clear();
    float startApexX = 120.0f;
    float startApexY = 0.0f;
    float dx = sqrtf(3.0f) * (BALL_RADIUS + 0.15f);
    float dy = (2.0f * BALL_RADIUS) + 0.2f;

    for (int col = 0; col < 5; col++)
    {
        for (int row = 0; row <= col; row++)
        {
            float bx = startApexX + col * dx;
            float by = startApexY + (row - col / 2.0f) * dy;
            redBalls.emplace_back(bx, by, BALL_RADIUS, 7);
        }
    }
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

    glBindVertexArray(0);

    cueBall.Render();
    for (const auto& ball : redBalls)
    {
        ball.Render();
    }

	glBindVertexArray(vaoRect);

	glUniform1i(codColLocation, 3);
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
        float halfW = (TABLE_WIDTH / 2.0f) - BALL_RADIUS;
        float halfH = (TABLE_HEIGHT / 2.0f) - BALL_RADIUS;

        cueBall.RenderTrajectory(redBalls, halfW, halfH);
        cueBall.RenderCue(powerPercent);
    }

    glutSwapBuffers();
    glFlush();
}

void ProcessSpecialKeys(int key, int x, int y)
{
    float step = 0.05f;

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
        cueBall.Reset();
        for (auto& ball : redBalls)
        {
            ball.Reset();
        }
        isAiming = true;
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
    cueBall.SetAimAngle(mouseWorldX, mouseWorldY);
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
                cueBall.Shoot(actualPower);
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

void updatePhysics(int value)
{
    if (!isAiming)
    {
        const int SUB_STEPS = 10;
        float subDt = 1.0f / (float)SUB_STEPS;

        float halfW = (TABLE_WIDTH / 2.0f) - BALL_RADIUS;
        float halfH = (TABLE_HEIGHT / 2.0f) - BALL_RADIUS;

        for (int step = 0; step < SUB_STEPS; step++)
        {
            cueBall.UpdatePosition(subDt);
            for (auto& ball : redBalls)
            {
                ball.UpdatePosition(subDt);
            }

            cueBall.CheckCushions(halfW, halfH);
            for (auto& ball : redBalls)
            {
                ball.CheckCushions(halfW, halfH);
            }

            for (auto& ball : redBalls)
            {
                Ball::ResolveCollision(cueBall, ball);
            }

            for (size_t i = 0; i < redBalls.size(); i++)
            {
                for (size_t j = i + 1; j < redBalls.size(); j++)
                {
                    Ball::ResolveCollision(redBalls[i], redBalls[j]);
                }
            }

            cueBall.CheckPockets(pockets, 6, POCKET_RADIUS);
            for (auto& ball : redBalls)
            {
                ball.CheckPockets(pockets, 6, POCKET_RADIUS);
            }
        }

        cueBall.ApplyFriction(FRICTION);
        for (auto& ball : redBalls)
        {
            ball.ApplyFriction(FRICTION);
        }

        cueBall.CheckStop();
        for (auto& ball : redBalls)
        {
            ball.CheckStop();
        }

        bool anyMoving = cueBall.IsMoving();
        if (!anyMoving)
        {
            for (const auto& ball : redBalls)
            {
                if (ball.IsMoving())
                {
                    anyMoving = true;
                    break;
                }
            }
        }

        if (!anyMoving)
        {
            isAiming = true;
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
    glutCreateWindow("Proiect Biliard 2D - Demo");

    glewInit();

    Initialize();

    glutDisplayFunc(RenderFunction);
    glutSpecialFunc(ProcessSpecialKeys);
	glutMouseFunc(processMouseClick);
	glutPassiveMotionFunc(processMouseMotion);
	glutMotionFunc(processActiveMotion);
    glutTimerFunc(16, updatePhysics, 0);
    glutCloseFunc(Cleanup);

    glutMainLoop();

    return 0;
}
