#include <GL/glew.h>
#include <GLFW/glfw3.h>


#define _USE_MATH_DEFINES
#include <cmath> // za pi
#include <algorithm> // za max()
#include <iostream>
#include "../Header/Util.h"







int screenWidth = 1080;
int screenHeight = 1920;


float mapWidth = 2541.0f;
float mapHeight = 1832.0f;


float camX = 0.0f;
float camY = 0.0f;

float lastCamX = 0.0f;
float lastCamY = 0.0f;

float traveledDistance = 0.0f;


float uX = 0.0f; 
float uY = 0.0f; 
float uS = 1.0f;

unsigned mapTexture;

GLFWcursor* cursor;
GLFWcursor* cursorPressed;

bool rezimHodanja = true;


void preprocessTexture(unsigned& texture, const char* filepath) {
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    texture = loadImageToTexture(filepath);
    glBindTexture(GL_TEXTURE_2D, texture);

    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}


void formVAOs() {



}

// Projekat je dozvoljeno pisati počevši od ovog kostura
// Toplo se preporučuje razdvajanje koda po fajlovima (i eventualno potfolderima) !!!
// Srećan rad!
int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);


    // Formiranje prozora za prikaz sa datim dimenzijama i naslovom
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    screenWidth = mode->width;
    screenHeight = mode->height;
    // Uzimaju se širina i visina monitora
    // Moguće je igrati se sa aspect ratio-m dodavanjem frame buffer size callback-a
    GLFWwindow* window = glfwCreateWindow(screenWidth, screenHeight, "Vezba 3", monitor, NULL);
    if (window == NULL) return endProgram("Prozor nije uspeo da se kreira.");
    glfwMakeContextCurrent(window);

    if (glewInit() != GLEW_OK) return endProgram("GLEW nije uspeo da se inicijalizuje.");

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    preprocessTexture(mapTexture, "resources/novi-sad-map-0.jpg");
    //preprocessTexture(mapTexture, "resources/grass.png");
    cursor = loadImageToCursor("resources/bolji-compass.png");
    glfwSetCursor(window, cursor);

    unsigned int rectShader = createShader("rect.vert", "rect.frag");
    glUseProgram(rectShader);
    glUniform1i(glGetUniformLocation(rectShader, "uTex0"), 0);
    glUniform1i(glGetUniformLocation(rectShader, "uTex1"), 1);


    unsigned int basicShader = createShader("basic.vert", "basic.frag");

    unsigned int mapShader = createShader("map.vert", "map.frag");



    float vertices[] = {
         -0.2f, 0.2f, 0.0f, 0.0f, 1.0f, // gornje levo teme
         -0.2f, -0.2f, 0.0f, 1.0f, 0.0f, // donje levo teme
         0.2f, -0.2f, 1.0f, 0.0f, 0.0f, // donje desno teme
    };
    unsigned int VAOnight;
    size_t nightSize = sizeof(vertices);
    unsigned int VBOnight;
    glGenVertexArrays(1, &VAOnight);
    glGenBuffers(1, &VBOnight);

    glBindVertexArray(VAOnight);
    glBindBuffer(GL_ARRAY_BUFFER, VBOnight);
    glBufferData(GL_ARRAY_BUFFER, nightSize, vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Atribut 1 (boja):
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);



    float verticesRect[] = {
        // x, y,  u, v
        -1.0f,  1.0f,   0.0f, 1.0f,   // top left
        -1.0f, -1.0f,   0.0f, 0.0f,   // bottom left
         1.0f, -1.0f,   1.0f, 0.0f,   // bottom right
         1.0f,  1.0f,   1.0f, 1.0f    // top right
    };



    unsigned int VAOrect;
    size_t rectSize = sizeof(verticesRect);
    unsigned int VBOrect;
    glGenVertexArrays(1, &VAOrect);
    glGenBuffers(1, &VBOrect);

    glBindVertexArray(VAOrect);
    glBindBuffer(GL_ARRAY_BUFFER, VBOrect);
    glBufferData(GL_ARRAY_BUFFER, rectSize, verticesRect, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);



    glClearColor(0.2f, 0.8f, 0.6f, 1.0f);

    while (!glfwWindowShouldClose(window))
    {
        double initFrameTime = glfwGetTime();
        float speed = 0.01f;

        // Camera movement (world space)
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) camY += speed;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) camY -= speed;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) camX -= speed;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) camX += speed;

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            break;

        glClear(GL_COLOR_BUFFER_BIT);

        // Draw night triangle
        glUseProgram(basicShader);
        glBindVertexArray(VAOnight);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        // ---------------------- MAP DRAWING ----------------------
        glUseProgram(mapShader);

        // true texture size (IMPORTANT!)
        float texW = 2541.0f;
        float texH = 1832.0f;

        float texAspect = texW / texH;
        float screenAspect = (float)screenWidth / screenHeight;

        // Zoom level (0.0 = deepest zoom, 1.0 = whole map)
        float zoom = 0.3f;

        // Correct UV window size (fixes distortion)
        float viewW = zoom;
        float viewH = zoom * (screenAspect / texAspect);

        // Convert camera world coords → UV coords
        float camUvX = camX / texW;
        float camUvY = camY / texH;

        // NO CLAMPING — can go outside 0..1
        glUniform1f(glGetUniformLocation(mapShader, "uCamX"), camUvX);
        glUniform1f(glGetUniformLocation(mapShader, "uCamY"), camUvY);
        glUniform1f(glGetUniformLocation(mapShader, "uViewW"), viewW);
        glUniform1f(glGetUniformLocation(mapShader, "uViewH"), viewH);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, mapTexture);

        glBindVertexArray(VAOrect);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

        glfwSwapBuffers(window);
        glfwPollEvents();

        while (glfwGetTime() - initFrameTime < 1 / 75.0) {}
    }




    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}