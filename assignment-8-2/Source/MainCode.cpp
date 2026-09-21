// ===========================
// IMPROVED main.cpp
//  Ahmed Ahmed
// Cs330
// ===========================

#include <GLFW/glfw3.h>
#include "linmath.h"
#include <glm/glm.hpp>

#include <vector>
#include <ctime>
#include <cmath>
#include <cstdlib>

using namespace std;

const float DEG2RAD = 3.14159f / 180.0f;

enum BRICKTYPE { REFLECTIVE, DESTRUCTABLE, PADDLE };
enum ONOFF { ON, OFF };

glm::vec2 normal;

void processInput(GLFWwindow* window);
glm::vec2 preventStuckBalls(glm::vec2 vector);
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

class Brick
{
public:
    float red, green, blue;
    float x, y, width, height;
    BRICKTYPE brick_type;
    ONOFF onoff;
    int health;
    int maxHealth;

    Brick(BRICKTYPE bt, float xx, float yy,
        float ww, float hh,
        float rr, float gg, float bb,
        int hp)
    {
        brick_type = bt;
        x = xx;
        y = yy;
        width = ww;
        height = hh;

        red = rr;
        green = gg;
        blue = bb;

        health = hp;
        maxHealth = hp;

        onoff = ON;
    }

    void updateColor()
    {
        float damage = (float)health / maxHealth;

        red = 1.0f - damage;
        green = damage;
        blue = 0.2f;
    }

    void drawBrick()
    {
        if (onoff == OFF)
            return;

        float halfW = width / 2;
        float halfH = height / 2;

        glColor3f(red, green, blue);

        glBegin(GL_POLYGON);

        glVertex2f(x + halfW, y + halfH);
        glVertex2f(x + halfW, y - halfH);
        glVertex2f(x - halfW, y - halfH);
        glVertex2f(x - halfW, y + halfH);

        glEnd();
    }
};

class Circle
{
public:
    float red, green, blue;
    float radius;

    float x;
    float y;

    float speed;

    glm::vec2 vector;

    int life;

    Circle(float xx, float yy,
        float rad,
        float r, float g, float b,
        glm::vec2 vel,
        float spd = 0.01f,
        int lf = 25)
    {
        x = xx;
        y = yy;

        radius = rad;

        red = r;
        green = g;
        blue = b;

        vector = vel;
        speed = spd;

        life = lf;
    }

    void DrawCircle()
    {
        glColor3f(red, green, blue);

        glBegin(GL_POLYGON);

        for (int i = 0; i < 360; i++)
        {
            float rad = i * DEG2RAD;

            glVertex2f(
                cos(rad) * radius + x,
                sin(rad) * radius + y
            );
        }

        glEnd();
    }

    bool CheckCollision(Brick* brk)
    {
        if (brk->onoff == OFF)
            return false;

        bool collision =
            (x + radius > brk->x - brk->width / 2) &&
            (x - radius < brk->x + brk->width / 2) &&
            (y + radius > brk->y - brk->height / 2) &&
            (y - radius < brk->y + brk->height / 2);

        if (!collision)
            return false;

        // Reflection physics
        glm::vec2 ballVector = vector;

        normal = glm::vec2(0.0f, 1.0f);

        vector =
            ballVector -
            2.0f * glm::dot(ballVector, normal) * normal;

        // Speed boost
        speed *= 1.02f;

        // Damage brick
        if (brk->brick_type == DESTRUCTABLE)
        {
            brk->health--;

            brk->updateColor();

            if (brk->health <= 0)
                brk->onoff = OFF;
        }

        return true;
    }

    void MoveOneStep()
    {
        // Top wall
        if (y > 1 - radius)
        {
            vector.y *= -1;
            speed *= 0.98f; // friction
        }

        // Right wall
        if (x > 1 - radius)
        {
            vector.x *= -1;

            // color change
            red = (float)rand() / RAND_MAX;
            green = (float)rand() / RAND_MAX;
            blue = (float)rand() / RAND_MAX;
        }

        // Left wall
        if (x < -1 + radius)
        {
            vector.x *= -1;
        }

        x += vector.x * speed;
        y += vector.y * speed;
    }
};

vector<Brick> brickVector;
vector<Circle> world;

Brick paddle(
    PADDLE,
    0.0f,
    -0.9f,
    0.5f,
    0.08f,
    0.2f,
    0.7f,
    1.0f,
    999
);

void createBrickLayout()
{
    // Pyramid layout

    int rows = 5;

    for (int row = 0; row < rows; row++)
    {
        int bricksInRow = rows - row;

        for (int col = 0; col < bricksInRow; col++)
        {
            float x =
                -0.6f +
                col * 0.3f +
                row * 0.15f;

            float y =
                0.8f -
                row * 0.15f;

            Brick b(
                DESTRUCTABLE,
                x,
                y,
                0.25f,
                0.10f,
                0.0f,
                1.0f,
                0.0f,
                row + 1
            );

            brickVector.push_back(b);
        }
    }
}

void handleCircleCollisions()
{
    for (int i = 0; i < world.size(); i++)
    {
        for (int j = i + 1; j < world.size(); j++)
        {
            float dx = world[i].x - world[j].x;
            float dy = world[i].y - world[j].y;

            float dist = sqrt(dx * dx + dy * dy);

            if (dist < world[i].radius + world[j].radius)
            {
                // Change colors
                world[i].red = 1;
                world[i].green = 0;
                world[i].blue = 0;

                world[j].red = 0;
                world[j].green = 0;
                world[j].blue = 1;

                // Bounce
                world[i].vector *= -1.0f;
                world[j].vector *= -1.0f;

                // Spawn smaller balls
                Circle mini(
                    world[i].x,
                    world[i].y,
                    0.02f,
                    1,
                    1,
                    0,
                    glm::normalize(glm::vec2(
                        rand() % 10 - 5,
                        rand() % 10 - 5
                    )),
                    0.015f,
                    10
                );

                world.push_back(mini);
            }
        }
    }
}

int main()
{
    srand((unsigned)time(NULL));

    if (!glfwInit())
        return -1;

    GLFWwindow* window =
        glfwCreateWindow(
            800,
            800,
            "Physics Brick World",
            NULL,
            NULL
        );

    glfwMakeContextCurrent(window);

    glfwSetKeyCallback(window, keyCallback);

    createBrickLayout();

    while (!glfwWindowShouldClose(window))
    {
        glClear(GL_COLOR_BUFFER_BIT);

        processInput(window);

        paddle.drawBrick();

        for (Brick& b : brickVector)
            b.drawBrick();

        for (int i = 0; i < world.size(); i++)
        {
            world[i].MoveOneStep();

            world[i].CheckCollision(&paddle);

            for (Brick& b : brickVector)
                world[i].CheckCollision(&b);

            world[i].DrawCircle();
        }

        handleCircleCollisions();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();

    return 0;
}

void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
    {
        paddle.x -= 0.03f;

        if (paddle.x < -0.75f)
            paddle.x = -0.75f;
    }

    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
    {
        paddle.x += 0.03f;

        if (paddle.x > 0.75f)
            paddle.x = 0.75f;
    }
}

glm::vec2 preventStuckBalls(glm::vec2 vector)
{
    if (abs(vector.x) < 0.1f)
        vector.x = 0.3f;

    if (abs(vector.y) < 0.1f)
        vector.y = 0.3f;

    return glm::normalize(vector);
}

void keyCallback(GLFWwindow* window,
    int key,
    int scancode,
    int action,
    int mods)
{
    if (key == GLFW_KEY_SPACE &&
        action == GLFW_PRESS)
    {
        glm::vec2 vel =
            glm::normalize(glm::vec2(
                rand() % 10 - 5,
                -(rand() % 10)
            ));

        vel = preventStuckBalls(vel);

        Circle c(
            0,
            0,
            0.05f,
            (float)rand() / RAND_MAX,
            (float)rand() / RAND_MAX,
            (float)rand() / RAND_MAX,
            vel,
            0.01f,
            20
        );

        world.push_back(c);
    }
}