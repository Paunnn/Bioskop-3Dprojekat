
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <string>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <map>

#include "../Header/Util.h"
#include "../Header/glm/glm.hpp"
#include "../Header/Camera.h"

const int ROWS = 5;
const int COLS = 10;
const int TOTAL_SEATS = ROWS * COLS;
const float TARGET_FPS = 75.0f;
const float FRAME_TIME = 1.0f / TARGET_FPS;

const float ROOM_WIDTH = 24.0f;
const float ROOM_DEPTH = 18.0f;
const float ROOM_HEIGHT = 12.0f;

const float SEAT_SIZE = 0.7f;
const float SEAT_SPACING_X = 1.3f;
const float SEAT_SPACING_Z = 1.6f;
const float ROW_HEIGHT_STEP = 0.5f;
const float STEP_BASE_Y = 0.2f;

const float AISLE_WIDTH = 1.5f;
const int AISLE_POSITION = COLS / 2;

const float SCREEN_WIDTH = 14.0f;
const float SCREEN_HEIGHT = 7.0f;

const int MAX_FRAME_TEXTURES = 25;
const float FRAME_SWITCH_TIME = 0.5f;
const float MOVIE_DURATION = 20.0f;

const int NUM_HUMANOID_TYPES = 15;

const glm::vec3 DOOR_POSITION(-ROOM_WIDTH / 2.0f + 1.5f, 0.0f, -ROOM_DEPTH / 2.0f + 0.5f);

enum SeatStatus { FREE, RESERVED, BOUGHT };
enum AppState { WAITING, ENTERING, MOVIE, LEAVING };
enum PersonState { WALKING_TO_AISLE, WALKING_IN_AISLE, WALKING_TO_SEAT, SEATED, WALKING_FROM_SEAT, WALKING_OUT_AISLE, EXITING, EXITED };

struct Seat {
    glm::vec3 position;
    SeatStatus status;
    int row, col;
    bool hasOccupant;

    Seat() : position(0.0f), status(FREE), row(0), col(0), hasOccupant(false) {}
    Seat(glm::vec3 pos, int r, int c) : position(pos), status(FREE), row(r), col(c), hasOccupant(false) {}
};

struct ModelMesh {
    GLuint VAO, VBO;
    int vertexCount;
    GLuint diffuseTexture;
    glm::vec3 diffuseColor;
};

struct Model3D {
    std::vector<ModelMesh> meshes;
    glm::vec3 boundsMin, boundsMax;
    float normalizeScale;
    glm::vec3 centerOffset;
};

struct Person {
    glm::vec3 position;
    glm::vec3 currentTarget;
    std::vector<glm::vec3> waypoints;
    int currentWaypointIndex;
    int assignedSeatIndex;
    int humanoidType;
    PersonState state;
    float entryDelay;
    float walkCycle;
    float facingAngle;
    bool active;

    Person() : position(0.0f), currentTarget(0.0f), currentWaypointIndex(0),
               assignedSeatIndex(-1), humanoidType(0), state(WALKING_TO_AISLE),
               entryDelay(0.0f), walkCycle(0.0f), facingAngle(0.0f), active(false) {}
};

std::vector<Seat> seats;
std::vector<Person> people;
std::vector<Model3D> loadedModels;
AppState currentState = WAITING;

float movieStartTime = -1.0f;
float stateStartTime = 0.0f;
int currentFrameIndex = 0;
float frameTimer = 0.0f;

bool depthTestEnabled = true;
bool cullingEnabled = true;

float doorOpenAmount = 0.0f;
bool doorOpening = false;
bool doorClosing = false;
const float DOOR_SPEED = 1.5f;

Camera camera(glm::vec3(0.0f, 2.0f, 10.0f));
float lastX = 400.0f;
float lastY = 400.0f;
bool firstMouse = true;

glm::vec3 mainLightPos(0.0f, ROOM_HEIGHT - 2.0f, 0.0f);
glm::vec3 lightColor(1.0f, 0.95f, 0.9f);
bool roomLightOn = true;

unsigned int basicShader = 0;
unsigned int screenShader = 0;
unsigned int overlayShader = 0;

unsigned int cubeVAO = 0, cubeVBO = 0;
unsigned int quadVAO = 0, quadVBO = 0;
unsigned int overlayVAO = 0, overlayVBO = 0;

unsigned int studentTexture = 0;
unsigned int crosshairTexture = 0;
std::vector<unsigned int> frameTextures;

void initModels();
Model3D loadOBJModel(const std::string& objPath);
void parseMTL(const std::string& mtlPath, const std::string& baseDir,
              std::map<std::string, glm::vec3>& colors,
              std::map<std::string, GLuint>& textures);
void initSeats();
void initGeometry();
bool initShaders();
void initTextures();

void processInput(GLFWwindow* window, float deltaTime);
void mouseCallback(GLFWwindow* window, double xpos, double ypos);
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

void updatePeople(float deltaTime);
void updateMovie(float deltaTime);
void createPeopleWaypoints();
void createExitWaypoints();

void renderScene();
void renderRoom();
void renderSeats();
void renderScreen();
void renderPeople();
void renderHumanoid(const Person& person);
void renderStudentOverlay();
void renderCrosshair();
void renderDecorations();
void renderDoor();

void drawCube(const glm::vec3& pos, const glm::vec3& scale, const glm::vec3& color);
void drawRotatedCube(const glm::vec3& pos, const glm::vec3& scale, const glm::vec3& color, float angleY);

bool rayBoxIntersection(const glm::vec3& rayOrigin, const glm::vec3& rayDir,
                        const glm::vec3& boxMin, const glm::vec3& boxMax, float& t);
int findSeatUnderCrosshair();
bool findNAdjacentSeats(int n, std::vector<int>& seatIndices);
glm::vec3 getAislePosition(int row);
float getSeatX(int col);

float cubeVertices[] = {

    -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
     0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 0.0f,
     0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
     0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
    -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f,
    -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,

    -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,
     0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
    -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 1.0f,
    -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,

    -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
    -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
    -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
    -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
    -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
    -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 1.0f,

     0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
     0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
     0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
     0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
     0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f,

    -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 0.0f,
     0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
     0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 1.0f,
     0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 1.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 0.0f,

    -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f,
     0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 1.0f,
    -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f,
    -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f
};

float quadVertices[] = {
    -0.5f, -0.5f,  0.0f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,
     0.5f, -0.5f,  0.0f,  0.0f,  0.0f,  1.0f,  1.0f, 0.0f,
     0.5f,  0.5f,  0.0f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
     0.5f,  0.5f,  0.0f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
    -0.5f,  0.5f,  0.0f,  0.0f,  0.0f,  1.0f,  0.0f, 1.0f,
    -0.5f, -0.5f,  0.0f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f
};

float overlayVertices[] = {
    -0.5f,  0.5f,  0.0f, 1.0f,
    -0.5f, -0.5f,  0.0f, 0.0f,
     0.5f, -0.5f,  1.0f, 0.0f,
     0.5f,  0.5f,  1.0f, 1.0f
};

float getStepHeightAtZ(float z) {
    for (int r = 0; r < ROWS; r++) {
        float rowZ = ROOM_DEPTH / 2.0f - 5.0f - r * SEAT_SPACING_Z;
        float halfSpacing = SEAT_SPACING_Z / 2.0f;
        if (z >= rowZ - halfSpacing && z <= rowZ + halfSpacing) {
            return STEP_BASE_Y + (ROWS - 1 - r) * ROW_HEIGHT_STEP;
        }
    }
    return 0.0f;
}

float getSeatX(int col) {
    float totalWidth = (COLS - 1) * SEAT_SPACING_X + AISLE_WIDTH;
    float startX = -totalWidth / 2.0f;
    float x = startX + col * SEAT_SPACING_X;
    if (col >= AISLE_POSITION) {
        x += AISLE_WIDTH;
    }
    return x;
}

glm::vec3 getAislePosition(int row) {
    float aisleX = (getSeatX(AISLE_POSITION - 1) + getSeatX(AISLE_POSITION)) / 2.0f;
    float y = STEP_BASE_Y + (ROWS - 1 - row) * ROW_HEIGHT_STEP + 0.2f;
    float z = ROOM_DEPTH / 2.0f - 5.0f - row * SEAT_SPACING_Z;
    return glm::vec3(aisleX, y, z);
}

int main() {
    srand((unsigned int)time(NULL));

    if (!glfwInit()) {
        return endProgram("GLFW initialization failed.");
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    GLFWwindow* window = glfwCreateWindow(mode->width, mode->height,
        "3D Bioskop - Cinema", monitor, NULL);

    if (!window) {
        return endProgram("Window creation failed.");
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(0);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, mouseCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetKeyCallback(window, keyCallback);

    if (glewInit() != GLEW_OK) {
        return endProgram("GLEW initialization failed.");
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    lastX = (float)mode->width / 2.0f;
    lastY = (float)mode->height / 2.0f;

    initModels();
    initSeats();
    initGeometry();
    if (!initShaders()) {
        return endProgram("Shader initialization failed.");
    }
    initTextures();

    float backRowZBound = ROOM_DEPTH / 2.0f - 5.0f + SEAT_SPACING_Z / 2.0f;
    camera.setRoomBounds(
        glm::vec3(-ROOM_WIDTH / 2.0f + 0.5f, 0.0f, -ROOM_DEPTH / 2.0f + 0.5f),
        glm::vec3(ROOM_WIDTH / 2.0f - 0.5f, ROOM_HEIGHT - 0.5f, backRowZBound)
    );

    camera.Position = glm::vec3(ROOM_WIDTH / 2.0f - 2.0f, 2.5f, 0.0f);

    glClearColor(0.02f, 0.02f, 0.05f, 1.0f);

    std::cout << "========================================" << std::endl;
    std::cout << "3D BIOSKOP - CONTROLS" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "WASD/Arrows: Move camera" << std::endl;
    std::cout << "Mouse: Look around" << std::endl;
    std::cout << "Left Click: Reserve/unreserve seat" << std::endl;
    std::cout << "1-9: Buy N adjacent seats" << std::endl;
    std::cout << "Enter: Start movie projection" << std::endl;
    std::cout << "F1: Toggle depth testing" << std::endl;
    std::cout << "F2: Toggle back-face culling" << std::endl;
    std::cout << "Escape: Exit" << std::endl;
    std::cout << "========================================" << std::endl;

    float lastTime = (float)glfwGetTime();
    float accumulator = 0.0f;

    while (!glfwWindowShouldClose(window)) {
        float currentTime = (float)glfwGetTime();
        float deltaTime = currentTime - lastTime;
        lastTime = currentTime;
        accumulator += deltaTime;

        processInput(window, deltaTime);

        if (accumulator >= FRAME_TIME) {
            accumulator -= FRAME_TIME;

            updatePeople(FRAME_TIME);
            if (currentState == MOVIE || currentState == ENTERING) {
                updateMovie(FRAME_TIME);
            }

            int width, height;
            glfwGetFramebufferSize(window, &width, &height);
            glViewport(0, 0, width, height);

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            renderScene();

            glfwSwapBuffers(window);
        }

        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &cubeVAO);
    glDeleteBuffers(1, &cubeVBO);
    glDeleteVertexArrays(1, &quadVAO);
    glDeleteBuffers(1, &quadVBO);
    glDeleteVertexArrays(1, &overlayVAO);
    glDeleteBuffers(1, &overlayVBO);

    if (basicShader) glDeleteProgram(basicShader);
    if (screenShader) glDeleteProgram(screenShader);
    if (overlayShader) glDeleteProgram(overlayShader);

    if (studentTexture) glDeleteTextures(1, &studentTexture);
    if (crosshairTexture) glDeleteTextures(1, &crosshairTexture);
    for (auto tex : frameTextures) {
        if (tex) glDeleteTextures(1, &tex);
    }

    for (auto& model : loadedModels) {
        for (auto& mesh : model.meshes) {
            glDeleteVertexArrays(1, &mesh.VAO);
            glDeleteBuffers(1, &mesh.VBO);
            if (mesh.diffuseTexture) glDeleteTextures(1, &mesh.diffuseTexture);
        }
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

void parseMTL(const std::string& mtlPath, const std::string& baseDir,
              std::map<std::string, glm::vec3>& colors,
              std::map<std::string, GLuint>& textures) {
    std::ifstream file(mtlPath);
    if (!file.is_open()) {
        std::cout << "Warning: Could not open MTL file: " << mtlPath << std::endl;
        return;
    }

    std::string currentMaterial;
    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string prefix;
        iss >> prefix;

        if (prefix == "newmtl") {
            iss >> currentMaterial;
            colors[currentMaterial] = glm::vec3(0.7f);
            textures[currentMaterial] = 0;
        } else if (prefix == "Kd" && !currentMaterial.empty()) {
            float r, g, b;
            iss >> r >> g >> b;
            colors[currentMaterial] = glm::vec3(r, g, b);
        } else if (prefix == "map_Kd" && !currentMaterial.empty()) {
            std::string texFile;
            iss >> texFile;
            std::string texPath = baseDir + "/" + texFile;
            GLuint tex = loadImageToTexture(texPath.c_str());
            if (tex) {
                glBindTexture(GL_TEXTURE_2D, tex);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
                glBindTexture(GL_TEXTURE_2D, 0);
                textures[currentMaterial] = tex;
            }
        }
    }
}

Model3D loadOBJModel(const std::string& objPath) {
    Model3D model;
    model.boundsMin = glm::vec3(1e30f);
    model.boundsMax = glm::vec3(-1e30f);

    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> texcoords;

    std::map<std::string, glm::vec3> matColors;
    std::map<std::string, GLuint> matTextures;

    std::map<std::string, std::vector<float>> matVertices;
    std::string currentMaterial = "__default";
    matColors[currentMaterial] = glm::vec3(0.7f);
    matTextures[currentMaterial] = 0;

    std::string baseDir = ".";
    size_t lastSlash = objPath.find_last_of("/\\");
    if (lastSlash != std::string::npos) {
        baseDir = objPath.substr(0, lastSlash);
    }

    std::ifstream file(objPath);
    if (!file.is_open()) {
        std::cout << "ERROR: Could not open OBJ file: " << objPath << std::endl;
        return model;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;

        std::istringstream iss(line);
        std::string prefix;
        iss >> prefix;

        if (prefix == "mtllib") {
            std::string mtlFile;
            iss >> mtlFile;
            std::string mtlPath = baseDir + "/" + mtlFile;
            parseMTL(mtlPath, baseDir, matColors, matTextures);
        } else if (prefix == "usemtl") {
            iss >> currentMaterial;
            if (matColors.find(currentMaterial) == matColors.end()) {
                matColors[currentMaterial] = glm::vec3(0.7f);
                matTextures[currentMaterial] = 0;
            }
        } else if (prefix == "v") {
            float x, y, z;
            iss >> x >> y >> z;
            positions.push_back(glm::vec3(x, y, z));
        } else if (prefix == "vn") {
            float x, y, z;
            iss >> x >> y >> z;
            normals.push_back(glm::vec3(x, y, z));
        } else if (prefix == "vt") {
            float u, v;
            iss >> u >> v;
            texcoords.push_back(glm::vec2(u, v));
        } else if (prefix == "f") {

            std::vector<std::string> tokens;
            std::string token;
            while (iss >> token) tokens.push_back(token);

            for (size_t i = 1; i + 1 < tokens.size(); i++) {
                std::string faceVerts[3] = { tokens[0], tokens[i], tokens[i + 1] };

                for (int fv = 0; fv < 3; fv++) {
                    int vi = 0, ti = 0, ni = 0;

                    std::string faceStr = faceVerts[fv];
                    for (char& c : faceStr) { if (c == '/') c = ' '; }

                    std::istringstream fiss(faceStr);

                    int slashCount = 0;
                    bool doubleSlash = false;
                    for (size_t k = 0; k + 1 < faceVerts[fv].size(); k++) {
                        if (faceVerts[fv][k] == '/') {
                            slashCount++;
                            if (faceVerts[fv][k + 1] == '/') doubleSlash = true;
                        }
                    }
                    if (faceVerts[fv].back() == '/') slashCount++;

                    fiss >> vi;
                    if (slashCount == 2 && doubleSlash) {

                        fiss >> ni;
                    } else if (slashCount == 2) {

                        fiss >> ti >> ni;
                    } else if (slashCount == 1) {

                        fiss >> ti;
                    }

                    glm::vec3 pos(0.0f);
                    glm::vec3 norm(0.0f, 1.0f, 0.0f);
                    glm::vec2 uv(0.0f);

                    if (vi > 0 && vi <= (int)positions.size()) pos = positions[vi - 1];
                    else if (vi < 0) pos = positions[positions.size() + vi];
                    if (ni > 0 && ni <= (int)normals.size()) norm = normals[ni - 1];
                    else if (ni < 0) norm = normals[normals.size() + ni];
                    if (ti > 0 && ti <= (int)texcoords.size()) uv = texcoords[ti - 1];
                    else if (ti < 0) uv = texcoords[texcoords.size() + ti];

                    auto& verts = matVertices[currentMaterial];
                    verts.push_back(pos.x); verts.push_back(pos.y); verts.push_back(pos.z);
                    verts.push_back(norm.x); verts.push_back(norm.y); verts.push_back(norm.z);
                    verts.push_back(uv.x); verts.push_back(uv.y);
                }
            }
        }
    }
    file.close();

    glm::vec3 rawMin(1e30f), rawMax(-1e30f);
    for (size_t i = 0; i < positions.size(); i++) {
        if (positions[i].x < rawMin.x) rawMin.x = positions[i].x;
        if (positions[i].y < rawMin.y) rawMin.y = positions[i].y;
        if (positions[i].z < rawMin.z) rawMin.z = positions[i].z;
        if (positions[i].x > rawMax.x) rawMax.x = positions[i].x;
        if (positions[i].y > rawMax.y) rawMax.y = positions[i].y;
        if (positions[i].z > rawMax.z) rawMax.z = positions[i].z;
    }
    float dx = rawMax.x - rawMin.x;
    float dy = rawMax.y - rawMin.y;
    float dz = rawMax.z - rawMin.z;

    int rotationType = 0;
    if (dz > dy * 1.1f && dz >= dx) {
        rotationType = 1;
    } else if (dx > dy * 1.1f && dx > dz) {
        rotationType = 2;
    }

    if (rotationType != 0) {

        for (auto& pair : matVertices) {
            std::vector<float>& verts = pair.second;
            for (size_t i = 0; i < verts.size(); i += 8) {
                float px = verts[i], py = verts[i+1], pz = verts[i+2];
                float nx = verts[i+3], ny = verts[i+4], nz = verts[i+5];
                if (rotationType == 1) {

                    verts[i+1] = -pz; verts[i+2] = py;
                    verts[i+4] = -nz; verts[i+5] = ny;
                } else {

                    verts[i] = -py; verts[i+1] = px;
                    verts[i+3] = -ny; verts[i+4] = nx;
                }
            }
        }
        std::cout << "  Auto-rotated to Y-up (type " << rotationType << ")" << std::endl;

        float sumY = 0.0f;
        int vertCount = 0;
        float tempMinY = 1e30f, tempMaxY = -1e30f;
        for (auto& pair : matVertices) {
            std::vector<float>& verts = pair.second;
            for (size_t i = 0; i < verts.size(); i += 8) {
                float py = verts[i+1];
                sumY += py;
                vertCount++;
                if (py < tempMinY) tempMinY = py;
                if (py > tempMaxY) tempMaxY = py;
            }
        }
        if (vertCount > 0) {
            float centroidY = sumY / (float)vertCount;
            float midpointY = (tempMinY + tempMaxY) * 0.5f;
            if (centroidY < midpointY) {

                std::cout << "  Flipping upside-down model (centroid below midpoint)" << std::endl;
                for (auto& pair : matVertices) {
                    std::vector<float>& verts = pair.second;
                    for (size_t i = 0; i < verts.size(); i += 8) {
                        verts[i+1] = -verts[i+1];
                        verts[i+4] = -verts[i+4];
                    }
                }
            }
        }
    }

    {

        float hMinX = 1e30f, hMaxX = -1e30f;
        float hMinY = 1e30f, hMaxY = -1e30f;
        float hMinZ = 1e30f, hMaxZ = -1e30f;
        for (auto& pair : matVertices) {
            std::vector<float>& verts = pair.second;
            for (size_t i = 0; i < verts.size(); i += 8) {
                if (verts[i]   < hMinX) hMinX = verts[i];
                if (verts[i]   > hMaxX) hMaxX = verts[i];
                if (verts[i+1] < hMinY) hMinY = verts[i+1];
                if (verts[i+1] > hMaxY) hMaxY = verts[i+1];
                if (verts[i+2] < hMinZ) hMinZ = verts[i+2];
                if (verts[i+2] > hMaxZ) hMaxZ = verts[i+2];
            }
        }
        float hDX = hMaxX - hMinX;
        float hDZ = hMaxZ - hMinZ;

        if (hDX > 0.001f && hDZ > 0.001f && hDX < hDZ * 0.65f) {
            std::cout << "  Rotating 90 deg (depth X->Z, dX=" << hDX << " dZ=" << hDZ << ")" << std::endl;
            for (auto& pair : matVertices) {
                std::vector<float>& verts = pair.second;
                for (size_t i = 0; i < verts.size(); i += 8) {
                    float px = verts[i], pz = verts[i+2];
                    float nx = verts[i+3], nz = verts[i+5];
                    verts[i] = pz; verts[i+2] = -px;
                    verts[i+3] = nz; verts[i+5] = -nx;
                }
            }

            float tmpMinX = hMinX, tmpMaxX = hMaxX;
            hMinX = hMinZ; hMaxX = hMaxZ;
            hMinZ = -tmpMaxX; hMaxZ = -tmpMinX;
            hDX = hMaxX - hMinX;
            hDZ = hMaxZ - hMinZ;
        }

        float headThreshY = hMinY + (hMaxY - hMinY) * 0.7f;
        float centerX = (hMinX + hMaxX) * 0.5f;
        float xMargin = hDX * 0.3f;

        float headMinZ = 1e30f, headMaxZ = -1e30f;
        for (auto& pair : matVertices) {
            std::vector<float>& verts = pair.second;
            for (size_t i = 0; i < verts.size(); i += 8) {
                if (verts[i+1] > headThreshY && fabsf(verts[i] - centerX) < xMargin) {
                    if (verts[i+2] < headMinZ) headMinZ = verts[i+2];
                    if (verts[i+2] > headMaxZ) headMaxZ = verts[i+2];
                }
            }
        }
        float headMidZ = (headMinZ + headMaxZ) * 0.5f;

        int frontVerts = 0, backVerts = 0;
        for (auto& pair : matVertices) {
            std::vector<float>& verts = pair.second;
            for (size_t i = 0; i < verts.size(); i += 8) {
                float vx = verts[i], vy = verts[i+1], vz = verts[i+2];
                if (vy > headThreshY && fabsf(vx - centerX) < xMargin) {
                    if (vz > headMidZ) frontVerts++;
                    else backVerts++;
                }
            }
        }

        std::cout << "  Facing check: head +Z=" << frontVerts << " -Z=" << backVerts << " (headMidZ=" << headMidZ << ")" << std::endl;

        if (backVerts > frontVerts * 1.5f && (frontVerts + backVerts) > 20) {
            std::cout << "  Flipping 180 deg (was facing -Z)" << std::endl;
            for (auto& pair : matVertices) {
                std::vector<float>& verts = pair.second;
                for (size_t i = 0; i < verts.size(); i += 8) {
                    verts[i]   = -verts[i];
                    verts[i+2] = -verts[i+2];
                    verts[i+3] = -verts[i+3];
                    verts[i+5] = -verts[i+5];
                }
            }
        }
    }

    model.boundsMin = glm::vec3(1e30f);
    model.boundsMax = glm::vec3(-1e30f);
    for (auto& pair : matVertices) {
        std::vector<float>& verts = pair.second;
        for (size_t i = 0; i < verts.size(); i += 8) {
            if (verts[i]   < model.boundsMin.x) model.boundsMin.x = verts[i];
            if (verts[i+1] < model.boundsMin.y) model.boundsMin.y = verts[i+1];
            if (verts[i+2] < model.boundsMin.z) model.boundsMin.z = verts[i+2];
            if (verts[i]   > model.boundsMax.x) model.boundsMax.x = verts[i];
            if (verts[i+1] > model.boundsMax.y) model.boundsMax.y = verts[i+1];
            if (verts[i+2] > model.boundsMax.z) model.boundsMax.z = verts[i+2];
        }
    }

    for (auto& pair : matVertices) {
        const std::string& matName = pair.first;
        std::vector<float>& verts = pair.second;
        if (verts.empty()) continue;

        ModelMesh mesh;
        mesh.vertexCount = (int)verts.size() / 8;
        mesh.diffuseColor = matColors.count(matName) ? matColors[matName] : glm::vec3(0.7f);
        mesh.diffuseTexture = matTextures.count(matName) ? matTextures[matName] : 0;

        glGenVertexArrays(1, &mesh.VAO);
        glGenBuffers(1, &mesh.VBO);
        glBindVertexArray(mesh.VAO);
        glBindBuffer(GL_ARRAY_BUFFER, mesh.VBO);
        glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(), GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        model.meshes.push_back(mesh);
    }

    float modelHeight = model.boundsMax.y - model.boundsMin.y;
    if (modelHeight > 0.001f) {
        model.normalizeScale = 1.7f / modelHeight;
    } else {
        model.normalizeScale = 1.0f;
    }
    model.centerOffset = glm::vec3(
        -(model.boundsMin.x + model.boundsMax.x) * 0.5f,
        -model.boundsMin.y,
        -(model.boundsMin.z + model.boundsMax.z) * 0.5f
    );

    int totalVerts = 0;
    for (auto& m : model.meshes) totalVerts += m.vertexCount;
    std::cout << "  Loaded: " << totalVerts << " vertices, " << model.meshes.size() << " material groups" << std::endl;

    return model;
}

void initModels() {

    const char* modelPaths[] = {
        "Resources/models/female_agent_model/female_agent_model.obj",
        "Resources/models/scientist_psx_style/scientist_psx_style.obj",
        "Resources/models/male_character_ps1-style/male_character_ps1-style.obj",
        "Resources/models/female_secretary_character_psx/female_secretary_character_psx.obj",
        "Resources/models/robi/robi.obj",
        "Resources/models/Youngster_bundle.fbx.obj",
        "Resources/models/oldman_ernest/oldman_ernest.obj",
        "Resources/models/police_3d_model/police_3d_model.obj",
        "Resources/models/agent.fbx.obj",
        "Resources/models/redneck_character_psx_style/redneck_character_psx_style.obj",
        "Resources/models/f31e42157dfc4d4c9e5462e50744585e/f31e42157dfc4d4c9e5462e50744585e.obj",
        "Resources/models/mitu_girl_model/mitu_girl_model.obj",
        "Resources/models/FBI.fbx.obj",
        "Resources/models/brawler.fbx.obj",
        "Resources/models/dutch_conductor_for_railway_ns_from_the_90s/dutch_conductor_for_railway_ns_from_the_90s.obj"
    };

    glm::vec3 fallbackColors[] = {
        glm::vec3(0.2f, 0.3f, 0.6f),
        glm::vec3(0.6f, 0.2f, 0.2f),
        glm::vec3(0.2f, 0.5f, 0.2f),
        glm::vec3(0.5f, 0.35f, 0.2f),
        glm::vec3(0.4f, 0.2f, 0.5f),
    };
    int fallbackIdx = 0;

    int numModels = sizeof(modelPaths) / sizeof(modelPaths[0]);
    std::cout << "Loading " << numModels << " 3D models..." << std::endl;

    for (int i = 0; i < numModels; i++) {
        std::cout << "Loading model " << (i + 1) << "/" << numModels << ": " << modelPaths[i] << std::endl;
        Model3D m = loadOBJModel(modelPaths[i]);
        if (!m.meshes.empty()) {

            bool allDefault = true;
            for (auto& mesh : m.meshes) {
                if (mesh.diffuseTexture != 0 ||
                    mesh.diffuseColor.x != 0.7f || mesh.diffuseColor.y != 0.7f || mesh.diffuseColor.z != 0.7f) {
                    allDefault = false;
                    break;
                }
            }
            if (allDefault) {
                glm::vec3 color = fallbackColors[fallbackIdx % 5];
                fallbackIdx++;
                for (auto& mesh : m.meshes) {
                    mesh.diffuseColor = color;
                }
                std::cout << "  Assigned fallback color to model without MTL." << std::endl;
            }
            loadedModels.push_back(m);
        } else {
            std::cout << "  WARNING: Model has no meshes, skipping." << std::endl;
        }
    }

    std::cout << "Successfully loaded " << loadedModels.size() << " models." << std::endl;
}

void initSeats() {
    seats.resize(TOTAL_SEATS);

    for (int row = 0; row < ROWS; row++) {
        for (int col = 0; col < COLS; col++) {
            int idx = row * COLS + col;
            float x = getSeatX(col);
            float y = 0.3f + STEP_BASE_Y + (ROWS - 1 - row) * ROW_HEIGHT_STEP;
            float z = ROOM_DEPTH / 2.0f - 5.0f - row * SEAT_SPACING_Z;

            seats[idx] = Seat(glm::vec3(x, y, z), row, col);
        }
    }
}

void initGeometry() {
    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &cubeVBO);
    glBindVertexArray(cubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    glGenVertexArrays(1, &overlayVAO);
    glGenBuffers(1, &overlayVBO);
    glBindVertexArray(overlayVAO);
    glBindBuffer(GL_ARRAY_BUFFER, overlayVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(overlayVertices), overlayVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

bool initShaders() {
    basicShader = createShader("Shaders/basic.vert", "Shaders/basic.frag");
    screenShader = createShader("Shaders/screen.vert", "Shaders/screen.frag");
    overlayShader = createShader("Shaders/overlay.vert", "Shaders/overlay.frag");
    return basicShader && screenShader && overlayShader;
}

void initTextures() {

    crosshairTexture = loadImageToTexture("Resources/camera.png");
    if (crosshairTexture) {
        glBindTexture(GL_TEXTURE_2D, crosshairTexture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glBindTexture(GL_TEXTURE_2D, 0);
        std::cout << "Loaded camera.png as crosshair icon." << std::endl;
    } else {

        unsigned char whitePixel[] = { 255, 255, 255, 255 };
        glGenTextures(1, &crosshairTexture);
        glBindTexture(GL_TEXTURE_2D, crosshairTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, whitePixel);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    studentTexture = loadImageToTexture("Resources/student.png");
    if (studentTexture) {
        glBindTexture(GL_TEXTURE_2D, studentTexture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    for (int i = 1; i <= MAX_FRAME_TEXTURES; i++) {
        char path[256];
        sprintf_s(path, sizeof(path), "Resources/frames/frame%02d.png", i);
        unsigned int tex = loadImageToTexture(path);
        if (tex) {
            glBindTexture(GL_TEXTURE_2D, tex);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glBindTexture(GL_TEXTURE_2D, 0);
            frameTextures.push_back(tex);
        }
    }

    std::cout << "Loaded " << frameTextures.size() << " movie frames." << std::endl;
}

void createPeopleWaypoints() {
    float delay = 0.0f;
    float frontRowZ = ROOM_DEPTH / 2.0f - 5.0f - (ROWS - 1) * SEAT_SPACING_Z;

    std::cout << "Creating waypoints for " << people.size() << " people..." << std::endl;

    for (auto& p : people) {
        p.waypoints.clear();
        p.entryDelay = delay;
        delay += 0.4f + (float)(rand() % 30) / 100.0f;

        Seat& seat = seats[p.assignedSeatIndex];
        float rowZ = seat.position.z;
        float walkZ = rowZ - SEAT_SPACING_Z * 0.35f;
        float stepY = STEP_BASE_Y + (ROWS - 1 - seat.row) * ROW_HEIGHT_STEP + 0.2f;

        glm::vec3 startPos = DOOR_POSITION + glm::vec3(0.0f, 0.1f, 0.5f);
        p.position = startPos;
        p.waypoints.push_back(startPos);

        float aisleX = getAislePosition(ROWS - 1).x;
        glm::vec3 aisleFront = glm::vec3(aisleX, 0.1f, frontRowZ - 1.0f);
        p.waypoints.push_back(aisleFront);

        glm::vec3 aisleAtRow = getAislePosition(seat.row);
        p.waypoints.push_back(aisleAtRow);

        p.waypoints.push_back(glm::vec3(aisleAtRow.x, stepY, walkZ));

        p.waypoints.push_back(glm::vec3(seat.position.x, stepY, walkZ));

        glm::vec3 seatPos = seat.position + glm::vec3(0.0f, 0.6f, 0.0f);
        p.waypoints.push_back(seatPos);

        p.currentWaypointIndex = 0;
        p.currentTarget = p.waypoints[0];
        p.state = WALKING_TO_AISLE;
        p.active = false;

        std::cout << "Person " << (&p - &people[0]) << " -> Seat[" << seat.row << "," << seat.col << "], Delay: " << p.entryDelay << "s" << std::endl;
    }

    std::cout << "Waypoints created successfully!" << std::endl;
}

void createExitWaypoints() {
    float frontRowZ = ROOM_DEPTH / 2.0f - 5.0f - (ROWS - 1) * SEAT_SPACING_Z;
    float aisleX = getAislePosition(0).x;

    for (int exitRow = ROWS - 1; exitRow >= 0; exitRow--) {
        float rowBaseDelay = (float)(ROWS - 1 - exitRow) * 0.5f;

        std::vector<int> rowPeople;
        for (int i = 0; i < (int)people.size(); i++) {
            if (people[i].state == EXITED) continue;
            if (seats[people[i].assignedSeatIndex].row == exitRow)
                rowPeople.push_back(i);
        }

        std::sort(rowPeople.begin(), rowPeople.end(), [&](int a, int b) {
            return fabsf(seats[people[a].assignedSeatIndex].position.x - aisleX) <
                   fabsf(seats[people[b].assignedSeatIndex].position.x - aisleX);
        });

        float personDelay = 0.0f;
        for (int idx : rowPeople) {
            Person& p = people[idx];
            Seat& seat = seats[p.assignedSeatIndex];
            float rowZ = seat.position.z;

            float walkZ = rowZ - SEAT_SPACING_Z * 0.35f;
            float stepY = STEP_BASE_Y + (ROWS - 1 - seat.row) * ROW_HEIGHT_STEP + 0.2f;

            p.waypoints.clear();
            p.entryDelay = rowBaseDelay + personDelay;
            personDelay += 0.15f + (float)(rand() % 10) / 100.0f;

            p.waypoints.push_back(glm::vec3(seat.position.x, stepY, walkZ));

            p.waypoints.push_back(glm::vec3(aisleX, stepY, walkZ));

            p.waypoints.push_back(getAislePosition(seat.row));

            p.waypoints.push_back(glm::vec3(aisleX, 0.1f, frontRowZ - 1.0f));

            p.waypoints.push_back(DOOR_POSITION + glm::vec3(0.0f, 0.1f, 0.5f));

            p.currentWaypointIndex = 0;
            p.currentTarget = p.waypoints[0];
            p.state = WALKING_FROM_SEAT;
            p.active = false;
        }
    }
}

void processInput(GLFWwindow* window, float deltaTime) {
    glm::vec3 oldPos = camera.Position;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
        camera.processKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
        camera.processKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
        camera.processKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
        camera.processKeyboard(RIGHT, deltaTime);

    float stepY = getStepHeightAtZ(camera.Position.z);
    float minCamY = stepY + camera.PlayerHeight;
    if (camera.Position.y < minCamY) {

        float oldStepY = getStepHeightAtZ(oldPos.z);
        float heightDiff = stepY - oldStepY;
        if (heightDiff > 0.6f) {

            camera.Position.x = oldPos.x;
            camera.Position.z = oldPos.z;
        }

        float currentStepY = getStepHeightAtZ(camera.Position.z);
        camera.Position.y = currentStepY + camera.PlayerHeight;
    }

    float stairFrontZ = ROOM_DEPTH / 2.0f - 5.0f - (ROWS - 1) * SEAT_SPACING_Z - SEAT_SPACING_Z / 2.0f;
    float stepHalfW = (ROOM_WIDTH - 1.0f) / 2.0f;
    if (camera.Position.z > stairFrontZ) {
        camera.Position.x = glm::clamp(camera.Position.x,
            -stepHalfW + camera.PlayerRadius, stepHalfW - camera.PlayerRadius);
    }
}

void mouseCallback(GLFWwindow* window, double xpos, double ypos) {
    float xposf = (float)xpos;
    float yposf = (float)ypos;

    if (firstMouse) {
        lastX = xposf;
        lastY = yposf;
        firstMouse = false;
    }

    float xoffset = xposf - lastX;
    float yoffset = lastY - yposf;
    lastX = xposf;
    lastY = yposf;

    camera.processMouseMovement(xoffset, yoffset);
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    if (currentState != WAITING) return;
    if (button != GLFW_MOUSE_BUTTON_LEFT || action != GLFW_PRESS) return;

    int seatIndex = findSeatUnderCrosshair();
    if (seatIndex >= 0) {
        if (seats[seatIndex].status == FREE) {
            seats[seatIndex].status = RESERVED;
            std::cout << "Seat [" << seats[seatIndex].row << "," << seats[seatIndex].col << "] reserved." << std::endl;
        } else if (seats[seatIndex].status == RESERVED) {
            seats[seatIndex].status = FREE;
            std::cout << "Seat [" << seats[seatIndex].row << "," << seats[seatIndex].col << "] unreserved." << std::endl;
        }
    }
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
        return;
    }

    if (action != GLFW_PRESS) return;

    if (key == GLFW_KEY_F1) {
        depthTestEnabled = !depthTestEnabled;
        if (depthTestEnabled) glEnable(GL_DEPTH_TEST);
        else glDisable(GL_DEPTH_TEST);
        std::cout << "Depth testing: " << (depthTestEnabled ? "ON" : "OFF") << std::endl;
    }

    if (key == GLFW_KEY_F2) {
        cullingEnabled = !cullingEnabled;
        if (cullingEnabled) glEnable(GL_CULL_FACE);
        else glDisable(GL_CULL_FACE);
        std::cout << "Back-face culling: " << (cullingEnabled ? "ON" : "OFF") << std::endl;
    }

    if (currentState == WAITING && key >= GLFW_KEY_1 && key <= GLFW_KEY_9) {
        int n = key - GLFW_KEY_0;
        std::vector<int> indices;
        if (findNAdjacentSeats(n, indices)) {
            for (int idx : indices) seats[idx].status = BOUGHT;
            std::cout << "Bought " << n << " ticket(s)." << std::endl;
        } else {
            std::cout << "Cannot find " << n << " adjacent free seats!" << std::endl;
        }
    }

    if (key == GLFW_KEY_ENTER && currentState == WAITING) {

        std::vector<int> occupiedSeats;
        for (int i = 0; i < TOTAL_SEATS; i++) {
            if (seats[i].status == RESERVED || seats[i].status == BOUGHT)
                occupiedSeats.push_back(i);
        }

        if (occupiedSeats.empty()) {
            std::cout << "No reserved seats!" << std::endl;
            return;
        }

        int numPeople = 1 + rand() % (int)occupiedSeats.size();

        for (int i = (int)occupiedSeats.size() - 1; i > 0; i--) {
            int j = rand() % (i + 1);
            std::swap(occupiedSeats[i], occupiedSeats[j]);
        }

        people.clear();
        for (int i = 0; i < numPeople; i++) {
            Person p;
            p.assignedSeatIndex = occupiedSeats[i];
            p.humanoidType = loadedModels.empty() ? 0 : rand() % (int)loadedModels.size();
            people.push_back(p);
            seats[occupiedSeats[i]].hasOccupant = true;
        }

        createPeopleWaypoints();
        currentState = ENTERING;
        stateStartTime = (float)glfwGetTime();
        roomLightOn = true;
        std::cout << "Movie starting! " << numPeople << " of " << occupiedSeats.size() << " viewers entering." << std::endl;
    }
}

void updatePeople(float deltaTime) {
    const float WALK_SPEED = (currentState == LEAVING) ? 4.5f : 2.5f;
    const float WAYPOINT_TOLERANCE = 0.2f;
    const float SEAT_TOLERANCE = 0.5f;

    float currentTime = (float)glfwGetTime() - stateStartTime;

    bool allSeated = true;
    bool allExited = true;
    int seatedCount = 0;
    int walkingCount = 0;
    int waitingCount = 0;

    if (currentState == ENTERING && doorOpenAmount < 1.0f) {
        doorOpenAmount += deltaTime * DOOR_SPEED;
        if (doorOpenAmount > 1.0f) doorOpenAmount = 1.0f;
    } else if (currentState == WAITING && doorOpenAmount > 0.0f) {
        doorOpenAmount -= deltaTime * DOOR_SPEED;
        if (doorOpenAmount < 0.0f) doorOpenAmount = 0.0f;
    } else if (currentState == LEAVING && doorOpenAmount < 1.0f) {
        doorOpenAmount += deltaTime * DOOR_SPEED;
        if (doorOpenAmount > 1.0f) doorOpenAmount = 1.0f;
    } else if (currentState == MOVIE && doorOpenAmount > 0.0f) {
        doorOpenAmount -= deltaTime * DOOR_SPEED;
        if (doorOpenAmount < 0.0f) doorOpenAmount = 0.0f;
    }

    for (auto& p : people) {
        if (p.state == EXITED) continue;

        if (p.state == SEATED) {
            allExited = false;
            seatedCount++;
            continue;
        }

        allSeated = false;
        allExited = false;

        if (!p.active && currentTime < p.entryDelay) {
            allSeated = false;
            waitingCount++;
            continue;
        }

        if (!p.active) {
            p.active = true;
            std::cout << "Person activated! Target: (" << p.currentTarget.x << ", " << p.currentTarget.y << ", " << p.currentTarget.z << ")" << std::endl;
        }

        walkingCount++;

        p.walkCycle += deltaTime * 8.0f;

        bool isLastWaypoint = (p.currentWaypointIndex >= (int)p.waypoints.size() - 1);

        glm::vec3 dir = p.currentTarget - p.position;

        float dx = dir.x;
        float dz = dir.z;
        float dist = sqrtf(dx * dx + dz * dz);

        float tolerance = isLastWaypoint ? SEAT_TOLERANCE : WAYPOINT_TOLERANCE;

        if (dist > tolerance) {
            dir = glm::normalize(dir);

            glm::vec3 pushForce(0.0f);
            for (size_t j = 0; j < people.size(); j++) {
                const Person& other = people[j];
                if (&other == &p) continue;
                if (other.state == SEATED || other.state == EXITED || !other.active) continue;
                float odx = p.position.x - other.position.x;
                float odz = p.position.z - other.position.z;
                float odist = sqrtf(odx * odx + odz * odz);
                if (odist < 0.6f && odist > 0.01f) {

                    float pushStrength = (0.6f - odist) * 3.0f;
                    pushForce.x += (odx / odist) * pushStrength;
                    pushForce.z += (odz / odist) * pushStrength;
                }
            }

            p.position = p.position + dir * WALK_SPEED * deltaTime + pushForce * deltaTime;
            p.facingAngle = atan2f(dir.x, dir.z);

            if (!isLastWaypoint) {

                float rowY = STEP_BASE_Y + 0.2f;
                for (int r = 0; r < ROWS; r++) {
                    float rowZ = ROOM_DEPTH / 2.0f - 5.0f - r * SEAT_SPACING_Z;
                    if (p.position.z > rowZ - SEAT_SPACING_Z / 2.0f) {
                        rowY = STEP_BASE_Y + (ROWS - 1 - r) * ROW_HEIGHT_STEP + 0.2f;
                        break;
                    }
                }
                p.position.y = rowY;
            }
        } else {

            if (isLastWaypoint) {

                if (currentState == ENTERING || currentState == MOVIE) {
                    p.state = SEATED;
                    p.position = seats[p.assignedSeatIndex].position + glm::vec3(0.0f, 0.0f, 0.0f);
                    p.facingAngle = 3.14159265f;
                    p.walkCycle = 0.0f;
                    std::cout << "Person SEATED at seat " << p.assignedSeatIndex << " [" << seats[p.assignedSeatIndex].row << "," << seats[p.assignedSeatIndex].col << "]" << std::endl;
                } else if (currentState == LEAVING) {
                    p.state = EXITED;
                    std::cout << "Person EXITED" << std::endl;
                }
            } else {

                p.currentWaypointIndex++;

                if (p.currentWaypointIndex < (int)p.waypoints.size()) {
                    p.currentTarget = p.waypoints[p.currentWaypointIndex];

                    if (p.currentWaypointIndex < (int)p.waypoints.size() - 1) {
                        std::cout << "Waypoint " << p.currentWaypointIndex << " reached. Next target: (" << p.currentTarget.x << ", " << p.currentTarget.y << ", " << p.currentTarget.z << ")" << std::endl;
                    } else {
                        std::cout << "Approaching seat [" << seats[p.assignedSeatIndex].row << "," << seats[p.assignedSeatIndex].col << "]..." << std::endl;
                    }
                }
            }
        }
    }

    static int frameCount = 0;
    if (frameCount++ % 60 == 0 && currentState == ENTERING) {
        std::cout << "Status: Seated=" << seatedCount << " Walking=" << walkingCount << " Waiting=" << waitingCount << " Total=" << people.size() << std::endl;
    }

    if (currentState == ENTERING && allSeated && people.size() > 0) {
        currentState = MOVIE;
        movieStartTime = (float)glfwGetTime();
        roomLightOn = false;
        std::cout << "=== ALL SEATED! Movie starting now ===" << std::endl;
    }

    if (currentState == LEAVING && allExited) {
        currentState = WAITING;
        roomLightOn = true;
        people.clear();
        for (auto& s : seats) {
            s.status = FREE;
            s.hasOccupant = false;
        }
        std::cout << "All viewers left. Ready for next show." << std::endl;
    }
}

void updateMovie(float deltaTime) {

    if (currentState != MOVIE && currentState != ENTERING) return;

    if (currentState == MOVIE && movieStartTime > 0) {
        float elapsed = (float)glfwGetTime() - movieStartTime;

        frameTimer += deltaTime;
        if (frameTimer >= FRAME_SWITCH_TIME) {
            frameTimer = 0.0f;
            currentFrameIndex++;
            if (!frameTextures.empty()) {
                currentFrameIndex = currentFrameIndex % (int)frameTextures.size();
            }
        }

        if (elapsed >= MOVIE_DURATION) {
            std::cout << "Movie ended. Viewers leaving..." << std::endl;
            currentState = LEAVING;
            stateStartTime = (float)glfwGetTime();
            roomLightOn = true;
            createExitWaypoints();
        }
    }
}

void renderScene() {
    int width, height;
    GLFWwindow* window = glfwGetCurrentContext();
    glfwGetFramebufferSize(window, &width, &height);
    float aspect = (float)width / (float)height;

    glm::mat4 projection = glm::perspective(glm::radians(camera.Fov), aspect, 0.1f, 100.0f);
    glm::mat4 view = camera.getViewMatrix();

    glUseProgram(basicShader);
    glUniformMatrix4fv(glGetUniformLocation(basicShader, "uProjection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(glGetUniformLocation(basicShader, "uView"), 1, GL_FALSE, glm::value_ptr(view));

    glm::vec3 effectiveLightPos = mainLightPos;
    glm::vec3 effectiveLightColor = roomLightOn ? lightColor : glm::vec3(0.1f);

    if (currentState == MOVIE) {

        effectiveLightPos = glm::vec3(0.0f, ROOM_HEIGHT / 2.0f - 1.0f, -ROOM_DEPTH / 2.0f + 1.5f);
        effectiveLightColor = glm::vec3(0.4f, 0.4f, 0.5f);
    }

    glUniform3fv(glGetUniformLocation(basicShader, "uLightPos"), 1, glm::value_ptr(effectiveLightPos));
    glUniform3fv(glGetUniformLocation(basicShader, "uLightColor"), 1, glm::value_ptr(effectiveLightColor));
    glUniform3fv(glGetUniformLocation(basicShader, "uViewPos"), 1, glm::value_ptr(camera.Position));
    glUniform1i(glGetUniformLocation(basicShader, "uUseLighting"), 1);
    glUniform1i(glGetUniformLocation(basicShader, "uUseTexture"), 0);
    glUniform1f(glGetUniformLocation(basicShader, "uAlpha"), 1.0f);

    renderRoom();
    renderDecorations();
    renderDoor();
    renderSeats();
    renderPeople();

    glUseProgram(screenShader);
    glUniformMatrix4fv(glGetUniformLocation(screenShader, "uProjection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(glGetUniformLocation(screenShader, "uView"), 1, GL_FALSE, glm::value_ptr(view));
    renderScreen();

    renderCrosshair();
    renderStudentOverlay();
}

void renderRoom() {
    glUseProgram(basicShader);

    glm::vec3 wallColor(0.18f, 0.12f, 0.1f);
    glm::vec3 floorColor(0.15f, 0.08f, 0.05f);
    glm::vec3 carpetColor(0.4f, 0.1f, 0.12f);

    drawCube(glm::vec3(0.0f, -0.5f, 0.0f), glm::vec3(ROOM_WIDTH + 2.0f, 1.0f, ROOM_DEPTH + 2.0f), floorColor);

    float aisleX = getAislePosition(0).x;
    drawCube(glm::vec3(aisleX, 0.06f, 0.0f), glm::vec3(AISLE_WIDTH + 0.5f, 0.05f, ROOM_DEPTH - 2.0f), carpetColor);

    drawCube(glm::vec3(0.0f, ROOM_HEIGHT + 0.5f, 0.0f), glm::vec3(ROOM_WIDTH + 2.0f, 1.0f, ROOM_DEPTH + 2.0f), wallColor);

    float wallThickness = 1.5f;

    drawCube(glm::vec3(0.0f, ROOM_HEIGHT / 2.0f, -ROOM_DEPTH / 2.0f - wallThickness / 2.0f),
             glm::vec3(ROOM_WIDTH + 2.0f, ROOM_HEIGHT, wallThickness), wallColor);

    drawCube(glm::vec3(0.0f, ROOM_HEIGHT / 2.0f, ROOM_DEPTH / 2.0f + wallThickness / 2.0f),
             glm::vec3(ROOM_WIDTH + 2.0f, ROOM_HEIGHT, wallThickness), wallColor);

    drawCube(glm::vec3(-ROOM_WIDTH / 2.0f - wallThickness / 2.0f, ROOM_HEIGHT / 2.0f, 0.0f),
             glm::vec3(wallThickness, ROOM_HEIGHT, ROOM_DEPTH + 2.0f), wallColor);

    drawCube(glm::vec3(ROOM_WIDTH / 2.0f + wallThickness / 2.0f, ROOM_HEIGHT / 2.0f, 0.0f),
             glm::vec3(wallThickness, ROOM_HEIGHT, ROOM_DEPTH + 2.0f), wallColor);

    glDisable(GL_CULL_FACE);

    glm::vec3 stepColor1(0.65f, 0.42f, 0.28f);
    glm::vec3 stepColor2(0.52f, 0.34f, 0.22f);
    glm::vec3 riserColor(0.80f, 0.55f, 0.30f);
    glm::vec3 edgeColor(1.0f, 0.85f, 0.35f);

    float stepWidth = ROOM_WIDTH - 1.0f;

    for (int row = 0; row < ROWS; row++) {
        float stepTopY = STEP_BASE_Y + (ROWS - 1 - row) * ROW_HEIGHT_STEP;
        float rowZ = ROOM_DEPTH / 2.0f - 5.0f - row * SEAT_SPACING_Z;
        float frontZ = rowZ - SEAT_SPACING_Z / 2.0f;
        float backZ = rowZ + SEAT_SPACING_Z / 2.0f;
        glm::vec3 stepColor = (row % 2 == 0) ? stepColor1 : stepColor2;

        float blockBottom = 0.03f;
        float blockH = stepTopY - blockBottom;
        if (blockH > 0.01f) {
            float blockCenterY = blockBottom + blockH / 2.0f;
            drawCube(glm::vec3(0.0f, blockCenterY, rowZ),
                     glm::vec3(stepWidth, blockH, SEAT_SPACING_Z), stepColor);
        }

        float treadH = 0.12f;
        drawCube(glm::vec3(0.0f, stepTopY + treadH / 2.0f, rowZ),
                 glm::vec3(stepWidth + 0.3f, treadH, SEAT_SPACING_Z + 0.1f), stepColor * 1.15f);

        if (row < ROWS - 1) {
            float nextStepTopY = STEP_BASE_Y + (ROWS - 1 - (row + 1)) * ROW_HEIGHT_STEP;
            float rH = stepTopY - nextStepTopY;
            drawCube(glm::vec3(0.0f, nextStepTopY + rH / 2.0f, frontZ),
                     glm::vec3(stepWidth + 0.3f, rH + 0.01f, 0.18f), riserColor);
        } else {

            drawCube(glm::vec3(0.0f, stepTopY / 2.0f + 0.02f, frontZ),
                     glm::vec3(stepWidth + 0.3f, stepTopY + 0.02f, 0.18f), riserColor);
        }

        drawCube(glm::vec3(0.0f, stepTopY + treadH + 0.01f, frontZ + 0.12f),
                 glm::vec3(stepWidth + 0.3f, 0.07f, 0.16f), edgeColor);
    }

    float backRowZ = ROOM_DEPTH / 2.0f - 5.0f + SEAT_SPACING_Z / 2.0f;
    float backStepTopY = STEP_BASE_Y + (ROWS - 1) * ROW_HEIGHT_STEP;
    drawCube(glm::vec3(0.0f, backStepTopY / 2.0f, backRowZ),
             glm::vec3(stepWidth + 0.3f, backStepTopY, 0.18f), riserColor);

    float fillDepth = ROOM_DEPTH / 2.0f - backRowZ;
    drawCube(glm::vec3(0.0f, ROOM_HEIGHT / 2.0f, backRowZ + fillDepth / 2.0f),
             glm::vec3(stepWidth + 0.3f, ROOM_HEIGHT, fillDepth), wallColor);

    float stairsFrontZ = ROOM_DEPTH / 2.0f - 5.0f - (ROWS - 1) * SEAT_SPACING_Z - SEAT_SPACING_Z / 2.0f;
    float stairsZLen = ROOM_DEPTH / 2.0f - stairsFrontZ;
    float stairsZMid = (ROOM_DEPTH / 2.0f + stairsFrontZ) / 2.0f;
    float sideGap = (ROOM_WIDTH - stepWidth) / 2.0f;

    drawCube(glm::vec3(-stepWidth / 2.0f - sideGap / 2.0f, ROOM_HEIGHT / 2.0f, stairsZMid),
             glm::vec3(sideGap + 0.3f, ROOM_HEIGHT, stairsZLen), wallColor);

    drawCube(glm::vec3(stepWidth / 2.0f + sideGap / 2.0f, ROOM_HEIGHT / 2.0f, stairsZMid),
             glm::vec3(sideGap + 0.3f, ROOM_HEIGHT, stairsZLen), wallColor);

    if (cullingEnabled) glEnable(GL_CULL_FACE);
}

void renderDecorations() {
    glUseProgram(basicShader);

    glm::vec3 curtainColor(0.5f, 0.08f, 0.1f);
    float curtainWidth = 2.0f;
    float screenZ = -ROOM_DEPTH / 2.0f + 0.3f;

    drawCube(glm::vec3(-SCREEN_WIDTH / 2.0f - curtainWidth / 2.0f - 0.5f, ROOM_HEIGHT / 2.0f, screenZ),
             glm::vec3(curtainWidth, ROOM_HEIGHT - 2.0f, 0.3f), curtainColor);

    drawCube(glm::vec3(SCREEN_WIDTH / 2.0f + curtainWidth / 2.0f + 0.5f, ROOM_HEIGHT / 2.0f, screenZ),
             glm::vec3(curtainWidth, ROOM_HEIGHT - 2.0f, 0.3f), curtainColor);

    drawCube(glm::vec3(0.0f, ROOM_HEIGHT - 1.5f, screenZ),
             glm::vec3(SCREEN_WIDTH + curtainWidth * 2 + 2.0f, 1.5f, 0.4f), curtainColor);

    glm::vec3 exitSignColor(0.8f, 0.1f, 0.1f);
    if (roomLightOn) exitSignColor = glm::vec3(1.0f, 0.2f, 0.2f);
    drawCube(DOOR_POSITION + glm::vec3(0.0f, 3.2f, 0.2f), glm::vec3(1.2f, 0.4f, 0.1f), exitSignColor);

    glm::vec3 sconceLightColor = roomLightOn ? glm::vec3(1.0f, 0.9f, 0.7f) : glm::vec3(0.3f, 0.25f, 0.2f);
    for (int i = 0; i < 3; i++) {
        float z = ROOM_DEPTH / 4.0f - i * ROOM_DEPTH / 3.0f;
        drawCube(glm::vec3(-ROOM_WIDTH / 2.0f + 0.3f, ROOM_HEIGHT * 0.6f, z), glm::vec3(0.2f, 0.4f, 0.15f), sconceLightColor);
        drawCube(glm::vec3(ROOM_WIDTH / 2.0f - 0.3f, ROOM_HEIGHT * 0.6f, z), glm::vec3(0.2f, 0.4f, 0.15f), sconceLightColor);
    }

    glm::vec3 fixtureMetal(0.3f, 0.25f, 0.2f);
    glm::vec3 bulbColor = roomLightOn ? glm::vec3(1.0f, 0.95f, 0.8f) : glm::vec3(0.15f, 0.12f, 0.1f);

    drawCube(glm::vec3(0.0f, ROOM_HEIGHT - 0.15f, 0.0f), glm::vec3(1.8f, 0.1f, 1.8f), fixtureMetal);

    glUniform1i(glGetUniformLocation(basicShader, "uUseLighting"), 0);

    drawCube(glm::vec3(0.0f, ROOM_HEIGHT - 0.25f, 0.0f), glm::vec3(1.5f, 0.08f, 1.5f), bulbColor);

    if (roomLightOn) {
        drawCube(glm::vec3(0.0f, ROOM_HEIGHT - 0.3f, 0.0f), glm::vec3(0.8f, 0.06f, 0.8f), glm::vec3(1.0f, 1.0f, 0.95f));
    }

    glUniform1i(glGetUniformLocation(basicShader, "uUseLighting"), 1);

    drawCube(glm::vec3(0.0f, ROOM_HEIGHT - 0.22f, 0.0f), glm::vec3(1.7f, 0.04f, 0.08f), fixtureMetal);
    drawCube(glm::vec3(0.0f, ROOM_HEIGHT - 0.22f, 0.0f), glm::vec3(0.08f, 0.04f, 1.7f), fixtureMetal);
}

void renderDoor() {
    glUseProgram(basicShader);

    glm::vec3 doorPos = DOOR_POSITION;
    glm::vec3 doorFrameColor(0.45f, 0.30f, 0.20f);
    glm::vec3 doorColor(0.65f, 0.40f, 0.25f);
    glm::vec3 doorHandleColor(0.85f, 0.75f, 0.45f);

    drawCube(doorPos + glm::vec3(-0.65f, 1.25f, 0.1f), glm::vec3(0.18f, 2.5f, 0.4f), doorFrameColor);

    drawCube(doorPos + glm::vec3(0.65f, 1.25f, 0.1f), glm::vec3(0.18f, 2.5f, 0.4f), doorFrameColor);

    drawCube(doorPos + glm::vec3(0.0f, 2.55f, 0.1f), glm::vec3(1.5f, 0.2f, 0.4f), doorFrameColor);

    float doorSlide = doorOpenAmount * 0.7f;

    drawCube(doorPos + glm::vec3(-0.3f - doorSlide, 1.2f, 0.15f),
             glm::vec3(0.55f, 2.3f, 0.12f), doorColor);

    drawCube(doorPos + glm::vec3(0.3f + doorSlide, 1.2f, 0.15f),
             glm::vec3(0.55f, 2.3f, 0.12f), doorColor);

    if (doorOpenAmount < 0.9f) {
        drawCube(doorPos + glm::vec3(-0.08f - doorSlide, 1.1f, 0.25f),
                 glm::vec3(0.12f, 0.06f, 0.08f), doorHandleColor);
        drawCube(doorPos + glm::vec3(0.08f + doorSlide, 1.1f, 0.25f),
                 glm::vec3(0.12f, 0.06f, 0.08f), doorHandleColor);
    }

    glm::vec3 signColor = roomLightOn ? glm::vec3(1.0f, 0.3f, 0.3f) : glm::vec3(0.5f, 0.1f, 0.1f);
    drawCube(doorPos + glm::vec3(0.0f, 2.8f, 0.15f), glm::vec3(1.2f, 0.3f, 0.1f), signColor);

    glm::vec3 matColor(0.35f, 0.2f, 0.15f);
    drawCube(doorPos + glm::vec3(0.0f, 0.02f, 0.6f), glm::vec3(1.8f, 0.03f, 0.8f), matColor);
}

void renderSeats() {
    glUseProgram(basicShader);

    bool wasCulling = cullingEnabled;
    if (wasCulling) {
        glDisable(GL_CULL_FACE);
    }

    for (const auto& seat : seats) {
        glm::vec3 fabricColor, frameColor;

        switch (seat.status) {
            case FREE:     fabricColor = glm::vec3(0.15f, 0.25f, 0.5f); break;
            case RESERVED: fabricColor = glm::vec3(0.7f, 0.6f, 0.1f); break;
            case BOUGHT:   fabricColor = glm::vec3(0.6f, 0.15f, 0.15f); break;
        }
        frameColor = glm::vec3(0.2f, 0.15f, 0.1f);

        drawCube(seat.position + glm::vec3(0.0f, 0.05f, 0.0f),
                 glm::vec3(SEAT_SIZE + 0.1f, 0.1f, SEAT_SIZE + 0.1f), frameColor);

        drawCube(seat.position + glm::vec3(0.0f, SEAT_SIZE / 4.0f + 0.05f, -0.05f),
                 glm::vec3(SEAT_SIZE - 0.05f, SEAT_SIZE / 2.5f, SEAT_SIZE - 0.1f), fabricColor);

        drawCube(seat.position + glm::vec3(0.0f, SEAT_SIZE * 0.7f, SEAT_SIZE / 2.0f - 0.08f),
                 glm::vec3(SEAT_SIZE - 0.05f, SEAT_SIZE * 0.9f, 0.12f), fabricColor * 0.9f);

        glm::vec3 armrestColor(0.15f, 0.1f, 0.08f);
        drawCube(seat.position + glm::vec3(-SEAT_SIZE / 2.0f - 0.08f, SEAT_SIZE * 0.4f, 0.0f),
                 glm::vec3(0.1f, 0.08f, SEAT_SIZE * 0.7f), armrestColor);
        drawCube(seat.position + glm::vec3(SEAT_SIZE / 2.0f + 0.08f, SEAT_SIZE * 0.4f, 0.0f),
                 glm::vec3(0.1f, 0.08f, SEAT_SIZE * 0.7f), armrestColor);
    }

    if (wasCulling) {
        glEnable(GL_CULL_FACE);
    }
}

void renderScreen() {
    glUseProgram(screenShader);

    glm::mat4 model(1.0f);
    model = glm::translate(model, glm::vec3(0.0f, ROOM_HEIGHT / 2.0f - 1.0f, -ROOM_DEPTH / 2.0f + 0.15f));
    model = glm::scale(model, glm::vec3(SCREEN_WIDTH, SCREEN_HEIGHT, 1.0f));

    glUniformMatrix4fv(glGetUniformLocation(screenShader, "uModel"), 1, GL_FALSE, glm::value_ptr(model));

    if (currentState == MOVIE && !frameTextures.empty()) {
        glUniform1i(glGetUniformLocation(screenShader, "uUseTexture"), 1);
        glUniform1f(glGetUniformLocation(screenShader, "uEmissionStrength"), 0.8f);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, frameTextures[currentFrameIndex]);
        glUniform1i(glGetUniformLocation(screenShader, "uTexture"), 0);
    } else if (currentState == MOVIE) {
        glUniform1i(glGetUniformLocation(screenShader, "uUseTexture"), 0);
        glUniform1f(glGetUniformLocation(screenShader, "uEmissionStrength"), 0.6f);
        float t = (float)glfwGetTime();
        glUniform3f(glGetUniformLocation(screenShader, "uEmissionColor"),
            0.5f + 0.5f * sinf(t * 2.0f),
            0.5f + 0.5f * sinf(t * 2.5f + 1.0f),
            0.5f + 0.5f * sinf(t * 3.0f + 2.0f));
    } else {
        glUniform1i(glGetUniformLocation(screenShader, "uUseTexture"), 0);
        glUniform1f(glGetUniformLocation(screenShader, "uEmissionStrength"), 0.05f);
        glUniform3f(glGetUniformLocation(screenShader, "uEmissionColor"), 0.85f, 0.85f, 0.85f);
    }

    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

void renderPeople() {
    if (people.empty()) return;

    int visibleCount = 0;
    for (const auto& p : people) {
        if (p.state != EXITED && p.active) visibleCount++;
    }

    static int renderFrameCount = 0;
    if (renderFrameCount++ % 120 == 0 && visibleCount > 0) {
        std::cout << "Rendering " << visibleCount << " people" << std::endl;
    }

    bool wasCulling = cullingEnabled;
    if (wasCulling) {
        glDisable(GL_CULL_FACE);
    }

    for (const auto& p : people) {
        if (p.state == EXITED || !p.active) continue;
        renderHumanoid(p);
    }

    if (wasCulling) {
        glEnable(GL_CULL_FACE);
    }
}

void renderHumanoid(const Person& person) {
    if (person.humanoidType < 0 || person.humanoidType >= (int)loadedModels.size()) return;

    Model3D& model = loadedModels[person.humanoidType];

    glm::mat4 modelMat(1.0f);
    modelMat = glm::translate(modelMat, person.position);
    modelMat = glm::rotate(modelMat, person.facingAngle, glm::vec3(0.0f, 1.0f, 0.0f));

    modelMat = glm::scale(modelMat, glm::vec3(model.normalizeScale));
    modelMat = glm::translate(modelMat, model.centerOffset);

    for (auto& mesh : model.meshes) {
        glUniformMatrix4fv(glGetUniformLocation(basicShader, "uModel"), 1, GL_FALSE, glm::value_ptr(modelMat));

        if (mesh.diffuseTexture) {
            glUniform1i(glGetUniformLocation(basicShader, "uUseTexture"), 1);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, mesh.diffuseTexture);
            glUniform1i(glGetUniformLocation(basicShader, "uTexture"), 0);
        } else {
            glUniform1i(glGetUniformLocation(basicShader, "uUseTexture"), 0);
            glUniform3fv(glGetUniformLocation(basicShader, "uColor"), 1, glm::value_ptr(mesh.diffuseColor));
        }

        glBindVertexArray(mesh.VAO);
        glDrawArrays(GL_TRIANGLES, 0, mesh.vertexCount);
    }
    glBindVertexArray(0);
}

void renderCrosshair() {
    if (currentState != WAITING) return;

    glDisable(GL_DEPTH_TEST);
    glUseProgram(overlayShader);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, crosshairTexture);
    glUniform1i(glGetUniformLocation(overlayShader, "uTexture"), 0);
    glUniform1f(glGetUniformLocation(overlayShader, "uAlpha"), 0.85f);

    glBindVertexArray(overlayVAO);

    glUniform2f(glGetUniformLocation(overlayShader, "uPos"), 0.0f, 0.0f);
    glUniform2f(glGetUniformLocation(overlayShader, "uSize"), 0.05f, 0.05f);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    glBindVertexArray(0);
    if (depthTestEnabled) glEnable(GL_DEPTH_TEST);
}

void renderStudentOverlay() {
    if (!studentTexture) return;

    glDisable(GL_DEPTH_TEST);
    glUseProgram(overlayShader);

    glUniform2f(glGetUniformLocation(overlayShader, "uPos"), 0.78f, -0.78f);
    glUniform2f(glGetUniformLocation(overlayShader, "uSize"), 0.35f, 0.35f);
    glUniform1f(glGetUniformLocation(overlayShader, "uAlpha"), 0.6f);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, studentTexture);
    glUniform1i(glGetUniformLocation(overlayShader, "uTexture"), 0);

    glBindVertexArray(overlayVAO);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glBindVertexArray(0);

    if (depthTestEnabled) glEnable(GL_DEPTH_TEST);
}

void drawCube(const glm::vec3& pos, const glm::vec3& scaleVec, const glm::vec3& color) {
    glm::mat4 model(1.0f);
    model = glm::translate(model, pos);
    model = glm::scale(model, scaleVec);

    glUniformMatrix4fv(glGetUniformLocation(basicShader, "uModel"), 1, GL_FALSE, glm::value_ptr(model));
    glUniform3fv(glGetUniformLocation(basicShader, "uColor"), 1, glm::value_ptr(color));
    glUniform1i(glGetUniformLocation(basicShader, "uUseTexture"), 0);

    glBindVertexArray(cubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

void drawRotatedCube(const glm::vec3& pos, const glm::vec3& scaleVec, const glm::vec3& color, float angleY) {
    glm::mat4 model(1.0f);
    model = glm::translate(model, pos);
    model = glm::rotate(model, angleY, glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::scale(model, scaleVec);

    glUniformMatrix4fv(glGetUniformLocation(basicShader, "uModel"), 1, GL_FALSE, glm::value_ptr(model));
    glUniform3fv(glGetUniformLocation(basicShader, "uColor"), 1, glm::value_ptr(color));
    glUniform1i(glGetUniformLocation(basicShader, "uUseTexture"), 0);

    glBindVertexArray(cubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

bool rayBoxIntersection(const glm::vec3& rayOrigin, const glm::vec3& rayDir,
                        const glm::vec3& boxMin, const glm::vec3& boxMax, float& t) {
    float tmin = -1e30f;
    float tmax = 1e30f;

    for (int i = 0; i < 3; i++) {
        if (fabsf(rayDir[i]) < 0.0001f) {
            if (rayOrigin[i] < boxMin[i] || rayOrigin[i] > boxMax[i]) return false;
        } else {
            float t1 = (boxMin[i] - rayOrigin[i]) / rayDir[i];
            float t2 = (boxMax[i] - rayOrigin[i]) / rayDir[i];
            if (t1 > t2) std::swap(t1, t2);
            tmin = std::max(tmin, t1);
            tmax = std::min(tmax, t2);
            if (tmin > tmax) return false;
        }
    }

    if (tmax < 0) return false;
    t = (tmin > 0) ? tmin : tmax;
    return true;
}

int findSeatUnderCrosshair() {
    glm::vec3 rayOrigin = camera.Position;
    glm::vec3 rayDir = camera.getRayDirection();

    int closestSeat = -1;
    float closestT = 1e30f;

    for (int i = 0; i < (int)seats.size(); i++) {
        glm::vec3 seatPos = seats[i].position;
        glm::vec3 boxMin = seatPos - glm::vec3(SEAT_SIZE / 2.0f, 0.0f, SEAT_SIZE / 2.0f);
        glm::vec3 boxMax = seatPos + glm::vec3(SEAT_SIZE / 2.0f, SEAT_SIZE, SEAT_SIZE / 2.0f);

        float t;
        if (rayBoxIntersection(rayOrigin, rayDir, boxMin, boxMax, t)) {
            if (t < closestT && t > 0) {
                closestT = t;
                closestSeat = i;
            }
        }
    }

    return closestSeat;
}

bool findNAdjacentSeats(int n, std::vector<int>& seatIndices) {
    seatIndices.clear();

    for (int row = 0; row < ROWS; row++) {
        for (int col = COLS - n; col >= 0; col--) {

            bool crossesAisle = false;
            for (int i = 0; i < n - 1; i++) {
                if (col + i == AISLE_POSITION - 1) crossesAisle = true;
            }
            if (crossesAisle) continue;

            bool found = true;
            std::vector<int> temp;
            for (int i = 0; i < n; i++) {
                int idx = row * COLS + col + i;
                if (seats[idx].status != FREE) {
                    found = false;
                    break;
                }
                temp.push_back(idx);
            }
            if (found) {
                seatIndices = temp;
                return true;
            }
        }
    }
    return false;
}
