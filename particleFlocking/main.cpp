#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/shader_m.h>
#include <learnopengl/camera.h>
#include <learnopengl/model.h>

#include <iostream>
#include <cstdlib>
#include <ctime>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

int LeftButtonDown = 0;
int RightButtonDown = 0;

const unsigned int SCR_WIDTH  = 1200;
const unsigned int SCR_HEIGHT = 1200;

Camera camera(glm::vec3(0.0f, 1.0f, 60.0f));
float lastX = SCR_WIDTH  / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;
float deltaTime = 0.0f;
float lastFrame = 0.0f;

glm::vec3 lightColor = glm::vec3(0.48f, 0.68f, 1.0f);
Shader* lightingShader;
Model*  birdObject1;
Model*  birdObject2;
const char* birdObjectPath1 = "./bird5.obj";
const char* birdObjectPath2 = "./bird6.obj";

void initGL(GLFWwindow** window);
void setupShader();
void destroyShader();
void createGLPrimitives();
void destroyGLPrimitives();
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void processInput(GLFWwindow* window, int key, int scancode, int action, int mods);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void DrawObject(glm::mat4 model);
void DrawFingerBase(glm::mat4 model);
void DrawFingerTip(glm::mat4 model);
void DrawBoundingBox();
glm::vec3 separation(int idx);
glm::vec3 alignment(int idx);
glm::vec3 cohesion(int idx);
glm::vec3 boundingForce(int idx);
void initBoid(int i);
void RenderImGui();

bool hasTextures    = false;
bool useCursor      = true;
bool cohesionFlag   = true;
bool separationFlag = true;
bool alignmentFlag  = true;
bool obstacleFlag   = false;

glm::vec3 boxMin(-10.0f, -10.0f,  5.0f);
glm::vec3 boxMax( 10.0f,  10.0f, 40.0f);
float boundMargin = 4.0f;

int boidAmount = 500;
glm::vec3 boidPos[2000];
glm::vec3 boidVel[2000];
glm::mat4 boidModel[2000];

class Primitive {
public:
    Primitive() {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ebo);
    }
    ~Primitive() {
        if (!ebo) glDeleteBuffers(1, &ebo);
        if (!vbo) glDeleteBuffers(1, &vbo);
        if (!VAO) glDeleteVertexArrays(1, &VAO);
    }
    void Draw() {
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLE_STRIP, IndexCount, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }
protected:
    unsigned int VAO = 0, vbo = 0, ebo = 0;
    unsigned int IndexCount = 0;
    float height = 1.0f;
    float radius[2] = { 1.0f, 1.0f };
};

class Cylinder : public Primitive {
public:
    Cylinder(float bottomRadius = 0.5f, float topRadius = 0.5f, int NumSegs = 16);
};

class Sphere : public Primitive {
public:
    Sphere(int NumSegs = 16);
};

class Plane : public Primitive {
public:
    Plane();
    void Draw() {
        glBindVertexArray(VAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, floorTexture);
        glDrawElements(GL_TRIANGLE_STRIP, IndexCount, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }
private:
    unsigned int floorTexture;
};

class WireBox {
public:
    WireBox() { glGenVertexArrays(1, &VAO); glGenBuffers(1, &VBO); }
    ~WireBox() { glDeleteBuffers(1, &VBO); glDeleteVertexArrays(1, &VAO); }
    void setBox(glm::vec3 mn, glm::vec3 mx) {
        float v[] = {
            mn.x,mn.y,mn.z,0,1,0,  mx.x,mn.y,mn.z,0,1,0,
            mx.x,mn.y,mn.z,0,1,0,  mx.x,mn.y,mx.z,0,1,0,
            mx.x,mn.y,mx.z,0,1,0,  mn.x,mn.y,mx.z,0,1,0,
            mn.x,mn.y,mx.z,0,1,0,  mn.x,mn.y,mn.z,0,1,0,
            mn.x,mx.y,mn.z,0,1,0,  mx.x,mx.y,mn.z,0,1,0,
            mx.x,mx.y,mn.z,0,1,0,  mx.x,mx.y,mx.z,0,1,0,
            mx.x,mx.y,mx.z,0,1,0,  mn.x,mx.y,mx.z,0,1,0,
            mn.x,mx.y,mx.z,0,1,0,  mn.x,mx.y,mn.z,0,1,0,
            mn.x,mn.y,mn.z,0,1,0,  mn.x,mx.y,mn.z,0,1,0,
            mx.x,mn.y,mn.z,0,1,0,  mx.x,mx.y,mn.z,0,1,0,
            mx.x,mn.y,mx.z,0,1,0,  mx.x,mx.y,mx.z,0,1,0,
            mn.x,mn.y,mx.z,0,1,0,  mn.x,mx.y,mx.z,0,1,0,
        };
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(v), v, GL_STATIC_DRAW);
        GLsizei stride = (GLsizei)(6 * sizeof(float));
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
        glBindVertexArray(0);
    }
    void Draw() {
        glBindVertexArray(VAO);
        glDrawArrays(GL_LINES, 0, 24);
        glBindVertexArray(0);
    }
private:
    unsigned int VAO = 0, VBO = 0;
};

Sphere*   unitSphere   = nullptr;
Plane*    groundPlane  = nullptr;
Cylinder* unitCylinder = nullptr;
Cylinder* unitCone     = nullptr;
WireBox*  wireBox      = nullptr;

float maxspeed  = 6.0f;
float maxforce  = 3.0f;
float sepRadius = 6.0f;
float aliRadius = 4.0f;
float cohRadius = 6.0f;

void myDisplay()
{
    glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    for (int i = 0; i < boidAmount; i++)
    {
        glm::vec3 steer(0.0f);
        if (separationFlag) steer += separation(i) * 1.5f;
        if (alignmentFlag)  steer += alignment(i)  * 1.0f;
        if (cohesionFlag)   steer += cohesion(i)   * 1.0f;
        steer += boundingForce(i) * 3.0f;

        float flen = glm::length(steer);
        if (flen > maxforce) steer *= maxforce / flen;

        boidVel[i] += steer * deltaTime;

        float speed = glm::length(boidVel[i]);
        if (speed > maxspeed)              boidVel[i] *= maxspeed / speed;
        else if (speed < 1.0f && speed > 0.0f) boidVel[i] *= 1.0f / speed;

        boidPos[i] += boidVel[i] * deltaTime;

        boidModel[i] = glm::translate(glm::mat4(1.0f), boidPos[i]);
        boidModel[i] = glm::scale(boidModel[i], glm::vec3(0.9f));

        DrawObject(boidModel[i]);
    }

    if (obstacleFlag)
    {
        glm::mat4 obsModel = glm::translate(glm::mat4(1.0f), (boxMin + boxMax) * 0.5f);
        obsModel = glm::scale(obsModel, glm::vec3(3.0f));
        lightingShader->use();
        lightingShader->setMat4("model", obsModel);
        lightingShader->setVec3("ObjColor", glm::vec3(1.0f, 0.3f, 0.3f));
        lightingShader->setInt("hasTextures", false);
        unitSphere->Draw();
    }

    DrawBoundingBox();

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    RenderImGui();
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

int main()
{
    GLFWwindow* window = NULL;
    initGL(&window);
    setupShader();
    createGLPrimitives();

    srand((unsigned int)time(nullptr));
    for (int i = 0; i < boidAmount; i++)
        initBoid(i);

    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = (float)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        lightingShader->use();
        lightingShader->setVec3("light.position",  camera.Position);
        lightingShader->setVec3("light.direction", camera.Front);
        lightingShader->setVec3("viewPos",         camera.Position);
        lightingShader->setVec3("light.ambient",   0.2f, 0.2f, 0.2f);
        lightingShader->setVec3("light.diffuse",   10.0f, 10.0f, 10.0f);
        lightingShader->setVec3("light.specular",  1.0f, 1.0f, 1.0f);
        lightingShader->setFloat("light.constant",   0.1f);
        lightingShader->setFloat("light.linear",     0.0f);
        lightingShader->setFloat("light.quadratic",  0.0009f);
        lightingShader->setFloat("material.shininess", 16.0f);

        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom),
            (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();
        lightingShader->setMat4("projection", projection);
        lightingShader->setMat4("view", view);

        myDisplay();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwSwapBuffers(window);
    glfwPollEvents();
    destroyGLPrimitives();
    destroyShader();
    glfwDestroyWindow(window);
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwTerminate();
    return 0;
}

void initGL(GLFWwindow** window)
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "boid", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate(); exit(-1);
    }
    glfwMakeContextCurrent(*window);
    glfwSetFramebufferSizeCallback(*window, framebuffer_size_callback);
    glfwSetCursorPosCallback(*window, mouse_callback);
    glfwSetMouseButtonCallback(*window, mouse_button_callback);
    glfwSetScrollCallback(*window, scroll_callback);
    glfwSetKeyCallback(*window, processInput);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl; exit(-1);
    }
    glEnable(GL_DEPTH_TEST);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(*window, true);
    ImGui_ImplOpenGL3_Init((char*)glGetString(GL_NUM_SHADING_LANGUAGE_VERSIONS));
}

void setupShader()
{
    lightingShader = new Shader("light_casters.vs", "light_casters.fs");
    lightingShader->use();
    lightingShader->setVec3("lightColor", lightColor);
}
void destroyShader() { delete lightingShader; }

void processInput(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) camera.ProcessKeyboard(FORWARD,  deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) camera.ProcessKeyboard(LEFT,     deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) camera.ProcessKeyboard(RIGHT,    deltaTime);
    if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS) {
        useCursor = !useCursor;
        glfwSetInputMode(window, GLFW_CURSOR,
            useCursor ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
    }
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (firstMouse) { lastX = (float)xpos; lastY = (float)ypos; firstMouse = false; }
    float xoffset = (float)(xpos - lastX) / SCR_WIDTH  * 10.0f;
    float yoffset = (float)(lastY - ypos) / SCR_HEIGHT * 10.0f;
    lastX = (float)xpos; lastY = (float)ypos;
    if (RightButtonDown) camera.ProcessMouseMovement(xoffset * 200, yoffset * 200);
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT)  LeftButtonDown  = (action == GLFW_PRESS) ? 1 : 0;
    if (button == GLFW_MOUSE_BUTTON_RIGHT) RightButtonDown = (action == GLFW_PRESS) ? 1 : 0;
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll((float)yoffset);
}

void createGLPrimitives()
{
    unitSphere   = new Sphere();
    groundPlane  = new Plane();
    unitCylinder = new Cylinder();
    unitCone     = new Cylinder(5.5f, 5.0f);
    birdObject1  = new Model(birdObjectPath1);
    birdObject2  = new Model(birdObjectPath2);
    hasTextures  = (birdObject1->textures_loaded.size() == 0) ? 0 : 1;
    wireBox      = new WireBox();
    wireBox->setBox(boxMin, boxMax);
}

void destroyGLPrimitives()
{
    delete unitSphere;
    delete groundPlane;
    delete unitCylinder;
    delete unitCone;
    delete birdObject1;
    delete birdObject2;
    delete wireBox;
}

void DrawFingerBase(glm::mat4 model)
{
    glm::mat4 Base = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.2f, 0.0f));
    Base = glm::scale(Base, glm::vec3(0.05f, 0.3f, 0.05f));
    glm::mat4 InBase = glm::inverse(Base);
    lightingShader->use();
    Base = model * Base;
    lightingShader->setMat4("model", Base);
    lightingShader->setVec3("ObjColor", glm::vec3(1.0f, 0.0f, 0.0f));
    lightingShader->setInt("hasTextures", false);
    unitCylinder->Draw();

    glm::mat4 Mat1 = glm::translate(InBase, glm::vec3(0.0f, 0.35f, 0.0f));
    Mat1 = glm::scale(Mat1, glm::vec3(0.07f, 0.07f, 0.07f));
    Mat1 = Base * Mat1;
    lightingShader->setMat4("model", Mat1);
    lightingShader->setVec3("ObjColor", glm::vec3(1.0f, 1.0f, 0.0f));
    unitSphere->Draw();
}

void DrawFingerTip(glm::mat4 model)
{
    glm::mat4 Base = glm::scale(glm::mat4(1.0f), glm::vec3(0.1f, 0.1f, 0.3f));
    Base = glm::translate(Base, glm::vec3(0.0f, 0.4f, 0.0f));
    lightingShader->use();
    Base = model * Base;
    lightingShader->setMat4("model", Base);
    lightingShader->setVec3("ObjColor", glm::vec3(0.2f, 0.4f, 0.8f));
    lightingShader->setInt("hasTextures", false);
    unitCone->Draw();
}

void DrawObject(glm::mat4 model)
{
    lightingShader->use();
    lightingShader->setMat4("model", model);
    lightingShader->setVec3("ObjColor", glm::vec3(1.0f, 1.0f, 1.0f));
    lightingShader->setInt("hasTextures", hasTextures);
    birdObject1->Draw(*lightingShader);
}

void DrawBoundingBox()
{
    lightingShader->use();
    lightingShader->setMat4("model", glm::mat4(1.0f));
    lightingShader->setVec3("ObjColor", glm::vec3(0.2f, 0.9f, 0.3f));
    lightingShader->setInt("hasTextures", false);
    wireBox->Draw();
}

void initBoid(int i)
{
    auto rnd = [](float lo, float hi) {
        return lo + (float)(rand() % 10000) / 10000.0f * (hi - lo);
    };
    boidPos[i] = glm::vec3(
        rnd(boxMin.x, boxMax.x),
        rnd(boxMin.y, boxMax.y),
        rnd(boxMin.z, boxMax.z));
    glm::vec3 dir(rnd(-1.0f,1.0f), rnd(-1.0f,1.0f), rnd(-1.0f,1.0f));
    float len = glm::length(dir);
    boidVel[i] = (len > 0.0001f ? dir / len : glm::vec3(1,0,0)) * rnd(2.0f, maxspeed);
}

glm::vec3 separation(int idx)
{
    glm::vec3 steer(0.0f);
    int count = 0;
    for (int i = 0; i < boidAmount; i++) {
        if (i == idx) continue;
        float d = glm::length(boidPos[i] - boidPos[idx]);
        if (d > 0.0f && d < sepRadius) {
            steer += glm::normalize(boidPos[idx] - boidPos[i]) / d;
            count++;
        }
    }
    if (count > 0) steer /= (float)count;
    return steer;
}

glm::vec3 alignment(int idx)
{
    glm::vec3 avg(0.0f);
    int count = 0;
    for (int i = 0; i < boidAmount; i++) {
        if (i == idx) continue;
        if (glm::length(boidPos[i] - boidPos[idx]) < aliRadius) {
            avg += boidVel[i];
            count++;
        }
    }
    if (count > 0) {
        avg /= (float)count;
        float len = glm::length(avg);
        if (len > 0.0f) avg = avg / len * maxspeed;
        avg -= boidVel[idx];
    }
    return avg;
}

glm::vec3 cohesion(int idx)
{
    glm::vec3 center(0.0f);
    int count = 0;
    for (int i = 0; i < boidAmount; i++) {
        if (i == idx) continue;
        if (glm::length(boidPos[i] - boidPos[idx]) < cohRadius) {
            center += boidPos[i];
            count++;
        }
    }
    if (count > 0) {
        center /= (float)count;
        glm::vec3 desired = center - boidPos[idx];
        float len = glm::length(desired);
        if (len > 0.0f) desired = desired / len * maxspeed;
        return desired - boidVel[idx];
    }
    return glm::vec3(0.0f);
}

glm::vec3 boundingForce(int idx)
{
    glm::vec3 steer(0.0f);
    glm::vec3 p = boidPos[idx];
    if (p.x < boxMin.x + boundMargin) steer.x += (boxMin.x + boundMargin - p.x);
    if (p.x > boxMax.x - boundMargin) steer.x -= (p.x - (boxMax.x - boundMargin));
    if (p.y < boxMin.y + boundMargin) steer.y += (boxMin.y + boundMargin - p.y);
    if (p.y > boxMax.y - boundMargin) steer.y -= (p.y - (boxMax.y - boundMargin));
    if (p.z < boxMin.z + boundMargin) steer.z += (boxMin.z + boundMargin - p.z);
    if (p.z > boxMax.z - boundMargin) steer.z -= (p.z - (boxMax.z - boundMargin));
    return steer;
}

void RenderImGui()
{
    ImGui::Begin("boid");
    ImGui::Checkbox("Cohesion",   &cohesionFlag);
    ImGui::Checkbox("Separation", &separationFlag);
    ImGui::Checkbox("Alignment",  &alignmentFlag);
    ImGui::Text("bird count: %d", boidAmount);
    if (ImGui::Button("add bird")) {
        int prev = boidAmount;
        boidAmount = glm::min(boidAmount + 50, 2000);
        for (int i = prev; i < boidAmount; i++) initBoid(i);
    }
    ImGui::SameLine();
    if (ImGui::Button("reduce bird"))
        boidAmount = glm::max(boidAmount - 50, 10);
    if (ImGui::Button("obstacle"))
        obstacleFlag = !obstacleFlag;
    if (ImGui::Button("reset"))
        for (int i = 0; i < boidAmount; i++) initBoid(i);
    ImGui::End();
}

Sphere::Sphere(int NumSegs)
{
    std::vector<glm::vec3> positions, normals;
    std::vector<unsigned int> indices;
    const float PI = 3.14159265359f;

    for (unsigned int y = 0; y <= (unsigned)NumSegs; ++y)
        for (unsigned int x = 0; x <= (unsigned)NumSegs; ++x) {
            float xs = (float)x/NumSegs, ys = (float)y/NumSegs;
            float xp = std::cos(xs*2*PI)*std::sin(ys*PI);
            float yp = std::cos(ys*PI);
            float zp = std::sin(xs*2*PI)*std::sin(ys*PI);
            positions.push_back({xp,yp,zp});
            normals  .push_back({xp,yp,zp});
        }
    bool oddRow = false;
    for (unsigned int y = 0; y < (unsigned)NumSegs; ++y) {
        if (!oddRow)
            for (unsigned int x = 0; x <= (unsigned)NumSegs; ++x) {
                indices.push_back(y*(NumSegs+1)+x);
                indices.push_back((y+1)*(NumSegs+1)+x);
            }
        else
            for (int x = NumSegs; x >= 0; --x) {
                indices.push_back((y+1)*(NumSegs+1)+x);
                indices.push_back(y*(NumSegs+1)+x);
            }
        oddRow = !oddRow;
    }
    IndexCount = (unsigned int)indices.size();
    std::vector<float> data;
    for (size_t i = 0; i < positions.size(); ++i) {
        data.push_back(positions[i].x); data.push_back(positions[i].y); data.push_back(positions[i].z);
        data.push_back(normals[i].x);   data.push_back(normals[i].y);   data.push_back(normals[i].z);
    }
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(data.size()*sizeof(float)), &data[0], GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)(indices.size()*sizeof(unsigned int)), &indices[0], GL_STATIC_DRAW);
    GLsizei stride = 6*sizeof(float);
    glEnableVertexAttribArray(0); glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,stride,(void*)0);
    glEnableVertexAttribArray(1); glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,stride,(void*)(3*sizeof(float)));
}

unsigned int loadTexture(char const* path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    int width, height, nrComponents;
    unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data) {
        GLenum format = GL_RED;
        if (nrComponents == 3) format = GL_RGB;
        else if (nrComponents == 4) format = GL_RGBA;
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, format==GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, format==GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        stbi_image_free(data);
    } else {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }
    return textureID;
}

Plane::Plane()
{
    float data[] = {
        -10,0,-10, 0,1,0, 0,0,
         10,0,-10, 0,1,0, 10,0,
         10,0, 10, 0,1,0, 10,10,
        -10,0, 10, 0,1,0, 0,10
    };
    unsigned int indices[] = { 0,1,3,2 };
    IndexCount = 4;
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(data), data, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    GLsizei stride = (GLsizei)(8 * sizeof(float));
    glEnableVertexAttribArray(0); glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,stride,(void*)0);
    glEnableVertexAttribArray(1); glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,stride,(void*)(3*sizeof(float)));
    glEnableVertexAttribArray(2); glVertexAttribPointer(2,2,GL_FLOAT,GL_FALSE,stride,(void*)(6*sizeof(float)));
    glBindVertexArray(0);
    floorTexture = loadTexture("./wood.png");
    lightingShader->use();
    lightingShader->setInt("texture1", floorTexture);
}

Cylinder::Cylinder(float bottomRadius, float topRadius, int NumSegs)
{
    radius[0] = bottomRadius; radius[1] = topRadius;
    std::vector<glm::vec3> base, positions, normals;
    std::vector<unsigned int> indices;
    const float PI = 3.14159265359f;
    float sectorStep = 2*PI/NumSegs;

    for (int i = 0; i <= NumSegs; ++i) {
        float a = i*sectorStep;
        base.push_back({std::sin(a), 0, std::cos(a)});
    }
    for (int i = 0; i < 2; ++i) {
        float h = -height/2.0f + i*height;
        for (int j = 0; j <= NumSegs; ++j) {
            positions.push_back({base[j].x*radius[i], h, base[j].z*radius[i]});
            normals  .push_back({base[j].x, h, base[j].z});
        }
    }
    int baseCenterIndex = (int)positions.size();
    int topCenterIndex  = baseCenterIndex + NumSegs + 1;
    for (int i = 0; i < 2; ++i) {
        float h = -height/2.0f + i*height, ny = -1.0f + i*2.0f;
        positions.push_back({0,h,0}); normals.push_back({0,ny,0});
        for (int j = 0; j < NumSegs; ++j) {
            positions.push_back({base[j].x*radius[i],h,base[j].z*radius[i]});
            normals  .push_back({0,ny,0});
        }
    }
    int k1=0, k2=NumSegs+1;
    for (int i = 0; i < NumSegs; ++i, ++k1, ++k2) {
        indices.push_back(k1); indices.push_back(k1+1); indices.push_back(k2);
        indices.push_back(k2); indices.push_back(k1+1); indices.push_back(k2+1);
    }
    for (int i=0, k=baseCenterIndex+1; i < NumSegs; ++i, ++k)
        if (i < NumSegs-1) { indices.push_back(baseCenterIndex); indices.push_back(k+1); indices.push_back(k); }
        else               { indices.push_back(baseCenterIndex); indices.push_back(baseCenterIndex+1); indices.push_back(k); }
    for (int i=0, k=topCenterIndex+1; i < NumSegs; ++i, ++k)
        if (i < NumSegs-1) { indices.push_back(topCenterIndex); indices.push_back(k); indices.push_back(k+1); }
        else               { indices.push_back(topCenterIndex); indices.push_back(k); indices.push_back(topCenterIndex+1); }

    IndexCount = (unsigned int)indices.size();
    std::vector<float> data;
    for (size_t i = 0; i < positions.size(); ++i) {
        data.push_back(positions[i].x); data.push_back(positions[i].y); data.push_back(positions[i].z);
        data.push_back(normals[i].x);   data.push_back(normals[i].y);   data.push_back(normals[i].z);
    }
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(data.size()*sizeof(float)), &data[0], GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)(indices.size()*sizeof(unsigned int)), &indices[0], GL_STATIC_DRAW);
    GLsizei stride = 6*sizeof(float);
    glEnableVertexAttribArray(0); glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,stride,(void*)0);
    glEnableVertexAttribArray(1); glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,stride,(void*)(3*sizeof(float)));
}
