#include <GL/glew.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/rotate_vector.hpp>

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

GLuint compileShader(GLenum type, const std::string& source) {
    GLuint shader = glCreateShader(type);
    const char* src = source.data();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLint length;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
        char* message = new char[length];
        glGetShaderInfoLog(shader, length, &length, message);
        throw std::runtime_error("Failed to compile shader!\n" + std::string(message));
    }

    return shader;
}

GLuint createShader(const std::string& vertex, const std::string& fragment) {
    GLuint program = glCreateProgram();

    std::vector<GLuint> shaders;
    shaders.push_back(compileShader(GL_VERTEX_SHADER, vertex));
    shaders.push_back(compileShader(GL_FRAGMENT_SHADER, fragment));

    for (GLuint shader : shaders) {
        glAttachShader(program, shader);
    }

    glLinkProgram(program);

    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        GLint length;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
        char* message = new char[length];
        glGetProgramInfoLog(program, length, &length, message);
        throw std::runtime_error("Failed to link shader program!\n" + std::string(message));
    }

    for (GLuint shader : shaders) {
        glDeleteShader(shader);
    }

    return program;
}

enum class Camera_Movement {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT,
    UP,
    DOWN
};

enum class Camera_View {
    FRONT,
    RIGHT,
    BACK,
    LEFT
};

const float CAM_YAW         = -90.0f;
const float CAM_PITCH       =  0.0f;
const float CAM_ROLL        =  0.0f;
const float CAM_SPEED       =  5.0f;
const float CAM_SENSITIVITY =  0.1f;
const float CAM_ZOOM        =  45.0f;

class Camera
{
public:
    // camera Attributes
    glm::vec3 Position;
    glm::vec3 Front;
    glm::vec3 Up;
    glm::vec3 Right;
    glm::vec3 WorldUp;
    // euler Angles
    float Yaw;
    float Pitch;
    float Roll;
    // camera options
    float MovementSpeed;
    float MouseSensitivity;
    float Zoom;

    // constructor with vectors
    Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), float yaw = CAM_YAW, float pitch = CAM_PITCH, float roll = CAM_ROLL) : Front(glm::vec3(0.0f, 0.0f, -1.0f)), MovementSpeed(CAM_SPEED), MouseSensitivity(CAM_SENSITIVITY), Zoom(CAM_ZOOM)
    {
        Position = position;
        WorldUp = up;
        Yaw = yaw;
        Pitch = pitch;
        Roll = roll;
        updateCameraVectors();
    }
    // constructor with scalar values
    Camera(float posX, float posY, float posZ, float upX, float upY, float upZ, float yaw, float pitch, float roll) : Front(glm::vec3(0.0f, 0.0f, -1.0f)), MovementSpeed(CAM_SPEED), MouseSensitivity(CAM_SENSITIVITY), Zoom(CAM_ZOOM)
    {
        Position = glm::vec3(posX, posY, posZ);
        WorldUp = glm::vec3(upX, upY, upZ);
        Yaw = yaw;
        Pitch = pitch;
        Roll = roll;
        updateCameraVectors();
    }

    // returns the view matrix calculated using Euler Angles and the LookAt Matrix
    glm::mat4 GetViewMatrix()
    {
        return glm::lookAt(Position, Position + Front, Up);
    }

    // processes input received from any keyboard-like input system. Accepts input parameter in the form of camera defined ENUM (to abstract it from windowing systems)
    void ProcessKeyboard(Camera_Movement direction, float deltaTime)
    {
        float velocity = MovementSpeed * deltaTime;
        if (direction == Camera_Movement::FORWARD)
            Position += Front * velocity;
        if (direction == Camera_Movement::BACKWARD)
            Position -= Front * velocity;
        if (direction == Camera_Movement::LEFT)
            Position -= Right * velocity;
        if (direction == Camera_Movement::RIGHT)
            Position += Right * velocity;
        if (direction == Camera_Movement::UP)
            Position += Up * velocity;
        if (direction == Camera_Movement::DOWN)
            Position -= Up * velocity;
    }

    // processes input received from a mouse input system. Expects the offset value in both the x and y direction.
    void ProcessMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch = true)
    {
        xoffset *= MouseSensitivity;
        yoffset *= MouseSensitivity;

        Yaw   += cos(glm::radians(Roll)) * xoffset + sin(glm::radians(Roll)) * yoffset;
        Pitch -= cos(glm::radians(Roll)) * yoffset - sin(glm::radians(Roll)) * xoffset;

        // make sure that when pitch is out of bounds, screen doesn't get flipped
        if (constrainPitch)
        {
            if (Pitch > 89.0f)
                Pitch = 89.0f;
            if (Pitch < -89.0f)
                Pitch = -89.0f;
        }

        // update Front, Right and Up Vectors using the updated Euler angles
        updateCameraVectors();
    }

    // processes input received from a mouse scroll-wheel event. Only requires input on the vertical wheel-axis
    void ProcessMouseScroll(float yoffset)
    {
        Zoom -= (float)yoffset;
        if (Zoom < 1.0f)
            Zoom = 1.0f;
        if (Zoom > 45.0f)
            Zoom = 45.0f;
    }

    void SetUnitView(Camera_View view) {
        const float DISTANCE = 30.0f;
        Pitch = 0.0f;
        Roll = 0.0f;

        if (view == Camera_View::FRONT) {
            Position = glm::vec3(0.0f, 0.0f, DISTANCE);
            Yaw = -90.0f;
        } else if (view == Camera_View::RIGHT) {
            Position = glm::vec3(DISTANCE, 0.0f, 0.0f);
            Yaw = 180.0f;
        } else if (view == Camera_View::BACK) {
            Position = glm::vec3(0.0f, 0.0f, -DISTANCE);
            Yaw = 90.0f;
        } else if (view == Camera_View::LEFT) {
            Position = glm::vec3(-DISTANCE, 0.0f, 0.0f);
            Yaw = 0.0f;
        }
        updateCameraVectors();
    }

    void Rotate(Camera_Movement direction) {
        if (direction == Camera_Movement::LEFT) {
            Roll += 90.0f;
        } else if (direction == Camera_Movement::RIGHT) {
            Roll -= 90.0f;
        }
        updateCameraVectors();
    }

private:
    // calculates the front vector from the Camera's (updated) Euler Angles
    void updateCameraVectors()
    {
        // calculate the new Front vector
        glm::vec3 front;
        front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        front.y = sin(glm::radians(Pitch));
        front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        Front = glm::normalize(front);
        // also re-calculate the Right and Up vector
        Right = glm::normalize(glm::rotateZ(glm::cross(Front, WorldUp), glm::radians(Roll)));  // normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
        Up    = glm::normalize(glm::cross(Right, Front));
    }
};

Camera camera(glm::vec3(0.0f, 0.0f, 0.0f));
double dt = 0.0;
double t = 0.0;
int ni = 0;
int ws = 0;
int lastBreakIndex = -1;

bool cleared = false;
bool paused = false;
bool pass_break = false;

bool q_pressed = false;
bool e_pressed = false;
bool c_pressed = false;
bool p_pressed = false;
bool n_pressed = false;

constexpr int SPEED = 2;

void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        camera.ProcessKeyboard(Camera_Movement::FORWARD, dt);
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        camera.ProcessKeyboard(Camera_Movement::BACKWARD, dt);
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        camera.ProcessKeyboard(Camera_Movement::LEFT, dt);
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        camera.ProcessKeyboard(Camera_Movement::RIGHT, dt);
    }
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        camera.ProcessKeyboard(Camera_Movement::UP, dt);
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
        camera.ProcessKeyboard(Camera_Movement::DOWN, dt);
    }
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
        ni = 0;
        t = 0.0;
        ws = 0;
        lastBreakIndex = -1;
        pass_break = false;
    }
    if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) {
        camera.SetUnitView(Camera_View::FRONT);
    }
    if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) {
        camera.SetUnitView(Camera_View::RIGHT);
    }
    if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS) {
        camera.SetUnitView(Camera_View::BACK);
    }
    if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS) {
        camera.SetUnitView(Camera_View::LEFT);
    }
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS && !q_pressed) {
        camera.Rotate(Camera_Movement::LEFT);
        q_pressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS && !e_pressed) {
        camera.Rotate(Camera_Movement::RIGHT);
        e_pressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_RELEASE) {
        q_pressed = false;
    }
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_RELEASE) {
        e_pressed = false;
    }
    if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS && !c_pressed) {
        cleared = true;
    }
    if (glfwGetKey(window, GLFW_KEY_C) == GLFW_RELEASE) {
        c_pressed = false;
    }
    if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) {
        t = std::max(0.0, t - 0.04);
        ws = std::min(ws, int(t * (float)SPEED / 0.1));
    }
    if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS) {
        t += 0.04;
    }
    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS && !p_pressed) {
        paused = !paused;
        p_pressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_RELEASE) {
        p_pressed = false;
    }
    if (glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS && !n_pressed) {
        pass_break = true;
        n_pressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_N) == GLFW_RELEASE) {
        n_pressed = false;
    }
}

double lastX = 400.0, lastY = 300.0;
bool firstMouse = true;
int SCREEN_WIDTH = 800, SCREEN_HEIGHT = 600;

void framebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
    SCREEN_WIDTH = width;
    SCREEN_HEIGHT = height;
}

void mouseCallback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    double xo = xpos - lastX;
    double yo = ypos - lastY;

    lastX = xpos;
    lastY = ypos;
    camera.ProcessMouseMovement((float)xo, (float)yo);
}

void scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    camera.ProcessMouseScroll((float)yoffset);
}

int main() {
    glfwInit();
    GLFWwindow* window = glfwCreateWindow(800, 600, "vis", nullptr, nullptr);
    glfwMakeContextCurrent(window);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSetCursorPosCallback(window, mouseCallback);
    glfwSetScrollCallback(window, scrollCallback);

    const std::string vertexShader =
    "#version 330 core\n"
    "layout (location = 0) in vec3 vPos;\n"
    "layout(std140) uniform Matrices {\n"
    "    mat4 view;\n"
    "    mat4 projection;\n"
    "};\n"
    "out vec3 fPos;\n"
    "void main() {\n"
    "    gl_Position = projection * view * vec4(vPos, 1.0);\n"
    "    fPos = vPos;\n"
    "}";

    const std::string fragmentShader =
    "#version 330 core\n"
    "out vec4 fColor;\n"
    "uniform vec3 cameraView;\n"
    "uniform vec3 cameraPos;\n"
    "in vec3 fPos;\n"
    "float sigmoid(float x) {\n"
    "    return 1.0 / (1.0 + (exp(-x) * 15.0));\n"
    "}\n"
    "vec3 hsv2rgb(vec3 c) {\n"
    "    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);\n"
    "    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);\n"
    "    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);\n"
    "}\n"
    "void main() {\n"
    "    vec3 diff = fPos - cameraPos;\n"
    "    float distanceScale = dot(cameraView, diff) / length(cameraView) / 10.0;\n"
    "    vec3 color = hsv2rgb(vec3(sigmoid(distanceScale), 0.8, 0.8));\n"
    "    fColor = vec4(color, 0.75);\n"
    "}";

    const std::string axisVertexShader =
    "#version 330 core\n"
    "layout (location = 0) in vec3 vPos;\n"
    "layout(std140) uniform Matrices {\n"
    "    mat4 view;\n"
    "    mat4 projection;\n"
    "};\n"
    "void main() {\n"
    "    gl_Position = projection * view * vec4(vPos, 1.0);\n"
    "}";

    const std::string axisFragmentShader =
    "#version 330 core\n"
    "out vec4 fColor;\n"
    "void main() {\n"
    "    fColor = vec4(1.0);\n"
    "}";

    const std::vector<float> axisVertices = {
        0.0f, 0.0f, 0.0f,  10000.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f,  0.0f, 10000.0f, 0.0f,
        0.0f, 0.0f, 0.0f,  0.0f, 0.0f, 10000.0f
    };

    std::vector<float> vertices;
    std::vector<int> breaks;
    {
        std::ifstream file("./filtered.txt");
        std::string str;
        int i = 0;
        while (std::getline(file, str)) {
            if (str == "BREAK") {
                breaks.push_back(i / 3);
            } else {
                float x = std::stof(str) * 100.0f;
                vertices.push_back(x);
            }
            ++i;
        }
    }

    glewInit();

    GLuint vao, vbo;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);

    GLuint avao, avbo;
    glGenVertexArrays(1, &avao);
    glBindVertexArray(avao);

    glGenBuffers(1, &avbo);
    glBindBuffer(GL_ARRAY_BUFFER, avbo);
    glBufferData(GL_ARRAY_BUFFER, axisVertices.size() * sizeof(float), axisVertices.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);

    GLuint shaderProgram = createShader(vertexShader, fragmentShader);
    GLuint axisShaderProgram = createShader(axisVertexShader, axisFragmentShader);

    glm::mat4 view = camera.GetViewMatrix();

    GLuint ubo;
    glGenBuffers(1, &ubo);
    glBindBuffer(GL_UNIFORM_BUFFER, ubo);
    glBufferData(GL_UNIFORM_BUFFER, 2 * sizeof(glm::mat4), nullptr, GL_DYNAMIC_DRAW);
    glBindBufferRange(GL_UNIFORM_BUFFER, 0, ubo, 0, sizeof(glm::mat4));
    glBindBuffer(GL_UNIFORM_BUFFER, ubo);

    GLuint ubi = glGetUniformBlockIndex(shaderProgram, "Matrices");
    glUniformBlockBinding(shaderProgram, ubi, 0);

    ubi = glGetUniformBlockIndex(axisShaderProgram, "Matrices");
    glUniformBlockBinding(axisShaderProgram, ubi, 0);

    glLineWidth(3.0);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    double ct = glfwGetTime();
    int prevNi;
    double bt = 0.0;
    bool broken = false;
    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT);

        double nt = glfwGetTime();
        dt = nt - ct;
        ct = nt;

        prevNi = ni;
        ni = std::min(int(vertices.size() / 3), int(t * (float)SPEED / 0.1));
        if (breaks.size() > 0 && breaks[lastBreakIndex + 1] >= prevNi && breaks[lastBreakIndex + 1] <= ni) {
            if (!broken) {
                bt = t;
                broken = true;
            }
            if (pass_break) {
                pass_break = false;
                broken = false;
                ++lastBreakIndex;
            } else {
                ni = prevNi;
                t = bt;
            }
        }

        processInput(window);

        glm::mat4 proj = glm::perspective(glm::radians(camera.Zoom), (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT, 0.1f, 1000.0f);
        glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::mat4), sizeof(glm::mat4), glm::value_ptr(proj));

        view = camera.GetViewMatrix();
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), glm::value_ptr(view));

        glUseProgram(axisShaderProgram);

        glBindVertexArray(avao);
        glBindBuffer(GL_ARRAY_BUFFER, avbo);
        glDrawArrays(GL_LINES, 0, axisVertices.size() / 3);

        glUseProgram(shaderProgram);

        glUniform3fv(glGetUniformLocation(shaderProgram, "cameraView"), 1, glm::value_ptr(camera.Front));
        glUniform3fv(glGetUniformLocation(shaderProgram, "cameraPos"), 1, glm::value_ptr(camera.Position));

        constexpr const int PERSISTENCE = 50;
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        if (cleared) {
            ws = ni;
            cleared = false;
        }
        glDrawArrays(GL_LINE_STRIP, std::max(ws, ni - PERSISTENCE), std::min(ni - ws, PERSISTENCE));
        //glDrawArrays(GL_LINE_STRIP, 0, vertices.size() / 3);

        t += dt;

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    return 0;
}
