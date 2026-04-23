#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <vector>
#include <cstdlib>
#include <cmath>
#include <iostream>
#include <string>

// ================= SHADERS =================
const char* vs = R"(
#version 330 core
layout(location = 0) in vec3 aPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
)";

const char* fs = R"(
#version 330 core
out vec4 FragColor;
uniform vec3 color;

void main()
{
    FragColor = vec4(color, 1.0);
}
)";

// ================= PIPE =================
struct Pipe {
    float x;
    float gapY;
    bool scored = false;
};

// ================= GLOBALS =================
glm::vec3 playerPos(-2.0f, 0.0f, -2.0f);
float velocityY = 0.0f;
float gravity = -4.5f;
float speed = 1.6f;

std::vector<Pipe> pipes;

int score = 0;
bool gameOver = false;
float goodTimer = 0.0f;

// ================= ENV (NEW) =================
glm::vec3 sunPos(4.5f, 3.5f, -4.0f);

std::vector<glm::vec3> clouds = {
    glm::vec3(-3.0f, 3.0f, -3.0f),
    glm::vec3(0.0f, 3.2f, -3.0f),
    glm::vec3(3.0f, 3.0f, -3.0f)
};

// ================= SHADER =================
unsigned int compileShader(unsigned int type, const char* src)
{
    unsigned int s = glCreateShader(type);
    glShaderSource(s, 1, &src, NULL);
    glCompileShader(s);
    return s;
}

unsigned int createProgram()
{
    unsigned int v = compileShader(GL_VERTEX_SHADER, vs);
    unsigned int f = compileShader(GL_FRAGMENT_SHADER, fs);

    unsigned int p = glCreateProgram();
    glAttachShader(p, v);
    glAttachShader(p, f);
    glLinkProgram(p);

    glDeleteShader(v);
    glDeleteShader(f);

    return p;
}

// ================= RESET =================
void reset()
{
    playerPos = glm::vec3(-2, 0, -2);
    velocityY = 0;
    score = 0;
    gameOver = false;
    goodTimer = 0;

    pipes.clear();

    float x = 4.0f;

    for (int i = 0; i < 10; i++)
    {
        Pipe p;
        p.x = x;
        p.gapY = ((rand() % 100) / 100.0f) * 1.5f - 0.75f;
        p.scored = false;
        pipes.push_back(p);
        x += 2.5f;
    }
}

// ================= MAIN =================
int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(800, 600, "Flappy 3D", NULL, NULL);
    glfwMakeContextCurrent(window);

    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

    glEnable(GL_DEPTH_TEST);

    unsigned int shader = createProgram();

    glm::mat4 projection = glm::perspective(glm::radians(70.0f), 800.f / 600.f, 0.1f, 100.f);

    glm::mat4 view = glm::lookAt(
        glm::vec3(2.5f, 2.0f, 7.0f),
        glm::vec3(0, 0, 0),
        glm::vec3(0, 1, 0)
    );

    // ================= CUBE =================
    float cube[] = {
        -0.3f,-0.3f, 0.3f,  0.3f,-0.3f, 0.3f,  0.3f, 0.3f, 0.3f,
         0.3f, 0.3f, 0.3f, -0.3f, 0.3f, 0.3f, -0.3f,-0.3f, 0.3f,

        -0.3f,-0.3f,-0.3f, -0.3f, 0.3f,-0.3f,  0.3f, 0.3f,-0.3f,
         0.3f, 0.3f,-0.3f,  0.3f,-0.3f,-0.3f, -0.3f,-0.3f,-0.3f,

        -0.3f,-0.3f,-0.3f, -0.3f,-0.3f, 0.3f, -0.3f, 0.3f, 0.3f,
        -0.3f, 0.3f, 0.3f, -0.3f, 0.3f,-0.3f, -0.3f,-0.3f,-0.3f,

         0.3f,-0.3f,-0.3f,  0.3f, 0.3f,-0.3f,  0.3f, 0.3f, 0.3f,
         0.3f, 0.3f, 0.3f,  0.3f,-0.3f, 0.3f,  0.3f,-0.3f,-0.3f,

        -0.3f, 0.3f,-0.3f, -0.3f, 0.3f, 0.3f,  0.3f, 0.3f, 0.3f,
         0.3f, 0.3f, 0.3f,  0.3f, 0.3f,-0.3f, -0.3f, 0.3f,-0.3f,

        -0.3f,-0.3f,-0.3f,  0.3f,-0.3f,-0.3f,  0.3f,-0.3f, 0.3f,
         0.3f,-0.3f, 0.3f, -0.3f,-0.3f, 0.3f, -0.3f,-0.3f,-0.3f
    };

    unsigned int VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cube), cube, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    reset();

    float last = 0;

    while (!glfwWindowShouldClose(window))
    {
        float now = glfwGetTime();
        float dt = now - last;
        last = now;

        // ================= INPUT =================
        if (!gameOver)
        {
            if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
                velocityY = 3.8f;
            else
                velocityY += gravity * dt;

            playerPos.y += velocityY * dt;
        }

        if (playerPos.y < -3.0f || playerPos.y > 3.5f)
            gameOver = true;

        // ================= MOVE CLOUDS =================
        for (auto& c : clouds)
        {
            c.x -= 0.5f * dt;
            if (c.x < -7) c.x = 7;
        }

        // ================= MOVE PIPES =================
        for (auto& p : pipes)
        {
            if (!gameOver)
                p.x -= speed * dt;

            if (p.x < -6)
            {
                p.x = 6;
                p.gapY = ((rand() % 100) / 100.0f) * 1.5f - 0.75f;
                p.scored = false;
            }

            if (!p.scored && p.x < playerPos.x)
            {
                score++;
                p.scored = true;

                if (score % 3 == 0)
                    goodTimer = 2.0f;
            }
        }

        for (auto& p : pipes)
        {
            float dx = fabs(playerPos.x - p.x);

            if (dx < 0.9f)
            {
                float gap = 1.2f;
                if (playerPos.y > p.gapY + gap || playerPos.y < p.gapY - gap)
                    gameOver = true;
            }
        }

        if (goodTimer > 0) goodTimer -= dt;

        // ================= TITLE =================
        std::string title = "Score: " + std::to_string(score);

        if (gameOver) title += " | GAME OVER";
        else if (goodTimer > 0) title += " | GOOD";

        glfwSetWindowTitle(window, title.c_str());

        if (gameOver && glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS)
            reset();

        // ================= RENDER =================
        glClearColor(0.5f, 0.8f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shader);

        int modelLoc = glGetUniformLocation(shader, "model");
        int viewLoc = glGetUniformLocation(shader, "view");
        int projLoc = glGetUniformLocation(shader, "projection");
        int colorLoc = glGetUniformLocation(shader, "color");

        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

        glBindVertexArray(VAO);

        // ☀️ SUN
        glm::mat4 sun = glm::translate(glm::mat4(1.0f), sunPos);
        sun = glm::scale(sun, glm::vec3(0.8f));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(sun));
        glUniform3f(colorLoc, 1.0f, 0.9f, 0.2f);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // ☁️ CLOUDS
        for (auto& c : clouds)
        {
            glm::mat4 cloud = glm::translate(glm::mat4(1.0f), c);
            cloud = glm::scale(cloud, glm::vec3(0.7f));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(cloud));
            glUniform3f(colorLoc, 0.95f, 0.95f, 0.95f);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        // 🐦 PLAYER
        glm::mat4 bird = glm::translate(glm::mat4(1.0f), playerPos);
        bird = glm::scale(bird, glm::vec3(0.6f));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(bird));
        glUniform3f(colorLoc, 0.1f, 0.4f, 1.0f);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // 🟩 PIPES
        for (auto& p : pipes)
        {
            float gap = 1.2f;
            float h = 3.0f;

            glm::mat4 top = glm::translate(glm::mat4(1.0f),
                glm::vec3(p.x, p.gapY + gap + h / 2, -2));
            top = glm::scale(top, glm::vec3(0.6, h, 0.6));

            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(top));
            glUniform3f(colorLoc, 0.1f, 0.8f, 0.1f);
            glDrawArrays(GL_TRIANGLES, 0, 36);

            glm::mat4 bottom = glm::translate(glm::mat4(1.0f),
                glm::vec3(p.x, p.gapY - gap - h / 2, -2));
            bottom = glm::scale(bottom, glm::vec3(0.6, h, 0.6));

            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(bottom));
            glUniform3f(colorLoc, 0.1f, 0.8f, 0.1f);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
}