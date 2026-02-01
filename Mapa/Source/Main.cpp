#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <ft2build.h>
#include FT_FREETYPE_H

#define _USE_MATH_DEFINES
#include <cmath> // za pi
#include <algorithm> // za max()
#include <iostream>
#include "../Header/Util.h"
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>

FT_Library ft;
FT_Face face;





int screenWidth = 1080;
int screenHeight = 1920;

float uX = 0.0f;
float uY = 0.0f;
float uS = 1.0f;
float mapWidth = 2541.0f;
float mapHeight = 1832.0f;
float camX = 0.0f;
float camY = 0.0f;
float lastCamX = 0.0f;
float lastCamY = 0.0f;
float traveledDistance = 0.0f;


float startX = uX;
float startY = uY;
float displacementDistance = 0.0f;


unsigned mapTexture;
unsigned pinTexture;
unsigned indexTexture;
unsigned cikicaTexture;
unsigned rulerTexture;
unsigned zero_texture;
unsigned one_texture;
unsigned two_texture;
unsigned three_texture;
unsigned four_texture;
unsigned five_texture;
unsigned six_texture;
unsigned seven_texture;
unsigned eight_texture;
unsigned nine_texture;


float camYaw = 0.0f;        // left/right
float camPitch = 0.0f;      // tilt (radians)
float camDist = 3.0f;      // zoom
glm::vec3 camTarget(0, 0, 0); // map center
float camSpeed = 0.1f;


GLFWcursor* cursor;
GLFWcursor* cursorPressed;

bool rezimHodanja = true;


struct MeasurePoint { float x, y; };

std::vector<MeasurePoint> measurePoints;
float totalMeasureDistance = 0.0f;
GLuint measureVAO, measureVBO;
void updateMeasureBuffer()
{
    glBindBuffer(GL_ARRAY_BUFFER, measureVBO);

    if (!measurePoints.empty())
        glBufferSubData(GL_ARRAY_BUFFER, 0, measurePoints.size() * sizeof(MeasurePoint), measurePoints.data());
}
void addMeasurePoint(float x, float y)
{
    MeasurePoint p{ x, y };

    if (!measurePoints.empty()) {
        auto& prev = measurePoints.back();
        float dx = p.x - prev.x;
        float dy = p.y - prev.y;
        totalMeasureDistance += sqrtf(dx * dx + dy * dy);
    }

    measurePoints.push_back(p);
    updateMeasureBuffer();
}
void deletePoint(int idx)
{
    if (idx < 0 || idx >= measurePoints.size()) return;

    // Remove its left segment
    if (idx > 0) {
        auto& a = measurePoints[idx - 1];
        auto& b = measurePoints[idx];
        totalMeasureDistance -= hypot(b.x - a.x, b.y - a.y);
    }

    // Remove its right segment
    if (idx < measurePoints.size() - 1) {
        auto& a = measurePoints[idx];
        auto& b = measurePoints[idx + 1];
        totalMeasureDistance -= hypot(b.x - a.x, b.y - a.y);
    }

    // Reconnect neighbors if it's in the middle
    if (idx > 0 && idx < measurePoints.size() - 1) {
        auto& a = measurePoints[idx - 1];
        auto& b = measurePoints[idx + 1];
        totalMeasureDistance += hypot(b.x - a.x, b.y - a.y);
    }

    measurePoints.erase(measurePoints.begin() + idx);
    updateMeasureBuffer();
}
void drawMeasureLines(GLuint lineShader)
{
    if (measurePoints.size() < 2) return;

    glUseProgram(lineShader);
    glUniform3f(glGetUniformLocation(lineShader, "color"), 1.0f, 1.0f, 1.0f);

    glBindVertexArray(measureVAO);

    glDrawArrays(GL_LINE_STRIP, 0, measurePoints.size());

    glBindVertexArray(0);
}

void drawMeasureDots(GLuint lineShader)
{
    glUseProgram(lineShader);
    glUniform3f(glGetUniformLocation(lineShader, "color"), 1, 1, 1);

    glBindVertexArray(measureVAO);
    glDrawArrays(GL_POINTS, 0, measurePoints.size());
}




void preprocessTexture(unsigned& texture, const char* filepath) {
    texture = loadImageToTexture(filepath); // Učitavanje teksture
    glBindTexture(GL_TEXTURE_2D, texture); // Vezujemo se za teksturu kako bismo je podesili

    // Generisanje mipmapa - predefinisani različiti formati za lakše skaliranje po potrebi (npr. da postoji 32 x 32 verzija slike, ali i 16 x 16, 256 x 256...)
    glGenerateMipmap(GL_TEXTURE_2D);

    // Podešavanje strategija za wrap-ovanje - šta da radi kada se dimenzije teksture i poligona ne poklapaju
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); // S - tekseli po x-osi
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT); // T - tekseli po y-osi

    // Podešavanje algoritma za smanjivanje i povećavanje rezolucije: nearest - bira najbliži piksel, linear - usrednjava okolne piksele
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}
void mouse_click_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (button != GLFW_MOUSE_BUTTON_LEFT || action != GLFW_PRESS) return;

    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);

    float x = (xpos / screenWidth) * 2.0f - 1.0f;
    float y = 1.0f - (ypos / screenHeight) * 2.0f;

    if (x < 0.7 && x > 0.6 && y < 1.0 && y > 0.9) {
        rezimHodanja = !rezimHodanja;
        return;
    }

    if (!rezimHodanja)
    {
        const float DELETE_RADIUS = 0.02f;

        for (int i = 0; i < measurePoints.size(); i++)
        {
            float dx = measurePoints[i].x - x;
            float dy = measurePoints[i].y - y;
            if (dx * dx + dy * dy < DELETE_RADIUS * DELETE_RADIUS) {
                deletePoint(i);
                return;
            }
        }

        addMeasurePoint(x, y);
    }
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

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);


    //preprocessTexture(mapTexture, "resources/novi-sad-map-0.jpg");
    preprocessTexture(mapTexture, "resources/novi sad bolji.jpg");
    preprocessTexture(pinTexture, "resources/pin.png");
    preprocessTexture(indexTexture, "resources/crveni index.png");
    preprocessTexture(cikicaTexture, "resources/cikica.png");
    preprocessTexture(rulerTexture, "resources/ruler.png");
    preprocessTexture(zero_texture, "resources/0.png");
    preprocessTexture(one_texture, "resources/1.png");
    preprocessTexture(two_texture, "resources/2.png");
    preprocessTexture(three_texture, "resources/3.png");
    preprocessTexture(four_texture, "resources/4.png");
    preprocessTexture(five_texture, "resources/5.png");
    preprocessTexture(six_texture, "resources/6.png");
    preprocessTexture(seven_texture, "resources/7.png");
    preprocessTexture(eight_texture, "resources/8.png");
    preprocessTexture(nine_texture, "resources/9.png");

    cursor = loadImageToCursor("resources/bolji-compass.png");
    glfwSetCursor(window, cursor);

    unsigned int textureShader = createShader("rect.vert", "rect.frag");
    glUseProgram(textureShader);
    glUniform1i(glGetUniformLocation(textureShader, "uTex0"), 0);
    glUniform1i(glGetUniformLocation(textureShader, "uTex1"), 1);

    unsigned int mapShader = createShader("map.vert", "map.frag");
    glUseProgram(mapShader);
    glUniform1i(glGetUniformLocation(mapShader, "uTex0"), 0);
    glUniform1i(glGetUniformLocation(mapShader, "uTex1"), 1);

    unsigned int lineShader = createShader("line.vert", "line.frag");

    unsigned int basicShader = createShader("basic.vert", "basic.frag");





    float walkingMap[] = {
        // x, y,  u, v
        -10.0f,  10.0f, 0.0f,   0.0f, 1.0f,   // top left
        -10.0f, -10.0f, 0.0f,   0.0f, 0.0f,   // bottom left
         10.0f, -10.0f, 0.0f,   1.0f, 0.0f,   // bottom right
         10.0f,  10.0f, 0.0f,   1.0f, 1.0f    // top right
    };
    unsigned int VAOwalkingMap;
    size_t walkingMapSize = sizeof(walkingMap);
    unsigned int VBOwalkingMap;
    glGenVertexArrays(1, &VAOwalkingMap);
    glGenBuffers(1, &VBOwalkingMap);

    glBindVertexArray(VAOwalkingMap);
    glBindBuffer(GL_ARRAY_BUFFER, VBOwalkingMap);
    glBufferData(GL_ARRAY_BUFFER, walkingMapSize, walkingMap, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    float rulerMap[] = {
        // x, y,  u, v
        -1.0f,  1.0f,   0.0f, 1.0f,   // top left
        -1.0f, -1.0f,   0.0f, 0.0f,   // bottom left
         1.0f, -1.0f,   1.0f, 0.0f,   // bottom right
         1.0f,  1.0f,   1.0f, 1.0f    // top right
    };



    unsigned int VAOrulerMap;
    size_t rulerMapSize = sizeof(rulerMap);
    unsigned int VBOrulerMap;
    glGenVertexArrays(1, &VAOrulerMap);
    glGenBuffers(1, &VBOrulerMap);

    glBindVertexArray(VAOrulerMap);
    glBindBuffer(GL_ARRAY_BUFFER, VBOrulerMap);
    glBufferData(GL_ARRAY_BUFFER, rulerMapSize, rulerMap, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);






    float cubeVertices[] = {
        // pos                 // color (red)
        // FRONT (+Z)
        -0.1f,-0.1f, 0.1f,      1,0,0,
         0.1f,-0.1f, 0.1f,      1,0,0,
         0.1f, 0.1f, 0.1f,      1,0,0,
         0.1f, 0.1f, 0.1f,      1,0,0,
        -0.1f, 0.1f, 0.1f,      1,0,0,
        -0.1f,-0.1f, 0.1f,      1,0,0,

        // BACK (-Z)
        -0.1f,-0.1f,-0.1f,      1,0,0,
        -0.1f, 0.1f,-0.1f,      1,0,0,
         0.1f, 0.1f,-0.1f,      1,0,0,
         0.1f, 0.1f,-0.1f,      1,0,0,
         0.1f,-0.1f,-0.1f,      1,0,0,
        -0.1f,-0.1f,-0.1f,      1,0,0,

        // LEFT (-X)
        -0.1f, 0.1f, 0.1f,      1,0,0,
        -0.1f, 0.1f,-0.1f,      1,0,0,
        -0.1f,-0.1f,-0.1f,      1,0,0,
        -0.1f,-0.1f,-0.1f,      1,0,0,
        -0.1f,-0.1f, 0.1f,      1,0,0,
        -0.1f, 0.1f, 0.1f,      1,0,0,

        // RIGHT (+X)
         0.1f, 0.1f, 0.1f,      1,0,0,
         0.1f, 0.1f,-0.1f,      1,0,0,
         0.1f,-0.1f,-0.1f,      1,0,0,
         0.1f,-0.1f,-0.1f,      1,0,0,
         0.1f,-0.1f, 0.1f,      1,0,0,
         0.1f, 0.1f, 0.1f,      1,0,0,

         // TOP (+Y)
         -0.1f, 0.1f,-0.1f,      1,0,0,
          0.1f, 0.1f,-0.1f,      1,0,0,
          0.1f, 0.1f, 0.1f,      1,0,0,
          0.1f, 0.1f, 0.1f,      1,0,0,
         -0.1f, 0.1f, 0.1f,      1,0,0,
         -0.1f, 0.1f,-0.1f,      1,0,0,

         // BOTTOM (-Y)
         -0.1f,-0.1f,-0.1f,      1,0,0,
         -0.1f,-0.1f, 0.1f,      1,0,0,
          0.1f,-0.1f, 0.1f,      1,0,0,
          0.1f,-0.1f, 0.1f,      1,0,0,
          0.1f,-0.1f,-0.1f,      1,0,0,
         -0.1f,-0.1f,-0.1f,      1,0,0,
    };
    unsigned int VAOcube, VBOcube;
    glGenVertexArrays(1, &VAOcube);
    glGenBuffers(1, &VBOcube);

    glBindVertexArray(VAOcube);

    glBindBuffer(GL_ARRAY_BUFFER, VBOcube);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);

    // location 0: position (vec3)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // location 1: color (vec3)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);



    float indexVertices[] = {
        //x    y
    0.7f,  1.0f,   0.0f, 1.0f,   // top left
    0.7f, 0.9f,   0.0f, 0.0f,   // bottom left
     1.0f, 0.9f,   1.0f, 0.0f,   // bottom right
     1.0f,  1.0f,   1.0f, 1.0f    // top right
    };
    unsigned int VAOindex;
    size_t indexSize = sizeof(indexVertices);
    unsigned int VBOindex;
    glGenVertexArrays(1, &VAOindex);
    glGenBuffers(1, &VBOindex);

    glBindVertexArray(VAOindex);
    glBindBuffer(GL_ARRAY_BUFFER, VAOindex);
    glBufferData(GL_ARRAY_BUFFER, indexSize, indexVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);



    float iconVertices[] = {
        //x    y
    0.6f,  1.0f,   0.0f, 1.0f,   // top left
    0.6f, 0.9f,   0.0f, 0.0f,   // bottom left
     0.7f, 0.9f,   1.0f, 0.0f,   // bottom right
     0.7f,  1.0f,   1.0f, 1.0f    // top right
    };
    unsigned int VAOicon;
    size_t iconSize = sizeof(iconVertices);
    unsigned int VBOicon;
    glGenVertexArrays(1, &VAOicon);
    glGenBuffers(1, &VBOicon);

    glBindVertexArray(VAOicon);
    glBindBuffer(GL_ARRAY_BUFFER, VAOicon);
    glBufferData(GL_ARRAY_BUFFER, iconSize, iconVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    float digitVertices[5][16];

    float startX = -1.0f;
    float endX = -0.7f;
    float width = (endX - startX) / 5.0f;   // = 0.12

    float topY = 1.0f;
    float bottomY = 0.9f;

    for (int i = 0; i < 5; i++)
    {
        float x0 = startX + i * width;
        float x1 = x0 + width;

        digitVertices[i][0] = x0; digitVertices[i][1] = topY;    digitVertices[i][2] = 0.0f; digitVertices[i][3] = 1.0f;
        digitVertices[i][4] = x0; digitVertices[i][5] = bottomY; digitVertices[i][6] = 0.0f; digitVertices[i][7] = 0.0f;
        digitVertices[i][8] = x1; digitVertices[i][9] = bottomY; digitVertices[i][10] = 1.0f; digitVertices[i][11] = 0.0f;
        digitVertices[i][12] = x1; digitVertices[i][13] = topY;  digitVertices[i][14] = 1.0f; digitVertices[i][15] = 1.0f;
    }


    unsigned int digitVAO[5];
    unsigned int digitVBO[5];
    for (int i = 0; i < 5; i++)
    {
        glGenVertexArrays(1, &digitVAO[i]);
        glGenBuffers(1, &digitVBO[i]);

        glBindVertexArray(digitVAO[i]);
        glBindBuffer(GL_ARRAY_BUFFER, digitVBO[i]);

        glBufferData(GL_ARRAY_BUFFER, sizeof(digitVertices[i]), digitVertices[i], GL_STATIC_DRAW);

        // Attribute 0: vec2 position
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        // Attribute 1: vec2 UV
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
        glEnableVertexAttribArray(1);
    }



    
    glGenVertexArrays(1, &measureVAO);
    glGenBuffers(1, &measureVBO);

    glBindVertexArray(measureVAO);
    glBindBuffer(GL_ARRAY_BUFFER, measureVBO);

    // allocate max size (e.g., 1000 points)
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 2 * 1000, NULL, GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);













    glClearColor(0.2f, 0.8f, 0.6f, 1.0f);

    while (!glfwWindowShouldClose(window))
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        double initFrameTime = glfwGetTime();
        float speed = 0.001f;
        glfwSetMouseButtonCallback(window, mouse_click_callback);

        glm::vec3 camPos;
        camPos.x = camTarget.x + camDist * cos(camPitch) * sin(camYaw);
        camPos.y = camTarget.y + camDist * sin(camPitch);
        camPos.z = camTarget.z + camDist * cos(camPitch) * cos(camYaw);



        glm::mat4 view = glm::lookAt(camPos, camTarget, glm::vec3(0, 1, 0));
        glm::mat4 proj = glm::perspective(glm::radians(60.0f), screenWidth / (float)screenHeight, 0.1f, 500.0f);
        glm::mat4 model = glm::mat4(1.0f);



        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) { // esc = close
            break;
        }
        if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) { // R = promena rezima
            rezimHodanja = !rezimHodanja;
        }


        glClear(GL_COLOR_BUFFER_BIT);
        if (rezimHodanja) {
            float prevX = uX;
            float prevY = uY;

            if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
                camTarget.y += camSpeed;

            if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
                camTarget.y -= camSpeed;

            if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
                camTarget.x -= camSpeed;

            if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
                camTarget.x += camSpeed;


            if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
                camPitch += camSpeed / 10;
            if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
                camPitch -= camSpeed / 10;
            if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
                camYaw -= camSpeed / 10;
            if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
                camYaw += camSpeed / 10;
            //if (camPitch < -1.5f) camPitch = -1.5f;
            //if (camPitch > 1.5f) camPitch = 1.5f;
            //if (camYaw < -1.5f) camYaw = -1.5f;
            //if (camYaw > 1.5f) camYaw = 1.5f;
            // Measure per-frame 
            float frameDX = uX - prevX;
            float frameDY = uY - prevY;
            float frameDist = sqrt(frameDX * frameDX + frameDY * frameDY);

            displacementDistance += frameDist;



            //draw map zoomed
            glUseProgram(mapShader);
            glUniformMatrix4fv(glGetUniformLocation(mapShader, "uModel"), 1, GL_FALSE, &model[0][0]);
            glUniformMatrix4fv(glGetUniformLocation(mapShader, "uView"), 1, GL_FALSE, &view[0][0]);
            glUniformMatrix4fv(glGetUniformLocation(mapShader, "uProj"), 1, GL_FALSE, &proj[0][0]);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, mapTexture);
            glBindVertexArray(VAOwalkingMap);
            glDrawArrays(GL_TRIANGLE_FAN, 0, 4);



            unsigned int digitTextures[10] = {
                zero_texture,
                one_texture,
                two_texture,
                three_texture,
                four_texture,
                five_texture,
                six_texture,
                seven_texture,
                eight_texture,
                nine_texture
            };





            float dist = displacementDistance;

            int displayDigits[5];

            
            int d = (int)(dist * 100.0f);  

            int digit0 = (d / 100000) % 10;  // deset hiljada
            int digit1 = (d / 10000) % 10;  // hiljada
            int digit2 = (d / 1000) % 10;  // sto
            int digit3 = (d / 100) % 10;  // deset

            
            int firstDecimal = ((int)(dist * 10)) % 10;   // decimala

            displayDigits[0] = digit0;
            displayDigits[1] = digit1;
            displayDigits[2] = digit2;
            displayDigits[3] = digit3;
            displayDigits[4] = firstDecimal;
            glUseProgram(textureShader);
            glActiveTexture(GL_TEXTURE0);

            for (int i = 0; i < 5; i++)
            {
                int d = displayDigits[i];  // the digit to show (0–9)
                glBindTexture(GL_TEXTURE_2D, digitTextures[d]);

                glBindVertexArray(digitVAO[i]);
                glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
            }





;

            glUseProgram(basicShader);

            // Send matrices
            glUniformMatrix4fv(glGetUniformLocation(basicShader, "uModel"), 1, GL_FALSE, &model[0][0]);
            glUniformMatrix4fv(glGetUniformLocation(basicShader, "uView"), 1, GL_FALSE, &view[0][0]);
            glUniformMatrix4fv(glGetUniformLocation(basicShader, "uProj"), 1, GL_FALSE, &proj[0][0]);

            // Optional: give it its own model transform (recommended)
            glm::mat4 cubeModel = glm::mat4(1.0f);
            cubeModel = glm::translate(cubeModel, glm::vec3(0.0f, 0.0f, 0.0f));
            // cubeModel = glm::rotate(cubeModel, (float)glfwGetTime(), glm::vec3(0, 1, 0));
            glUniformMatrix4fv(glGetUniformLocation(basicShader, "uModel"), 1, GL_FALSE, &cubeModel[0][0]);

            // Draw
            glBindVertexArray(VAOcube);
            glDrawArrays(GL_TRIANGLES, 0, 36);
            glBindVertexArray(0);

        }
        else {
            // Draw full map
            glUseProgram(textureShader);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, mapTexture);
            glBindVertexArray(VAOrulerMap);
            glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

            drawMeasureLines(lineShader);
            drawMeasureDots(lineShader);

            // Draw total measured distance 
            float dist = totalMeasureDistance;

            int displayDigits[5];

            int large = (int)(dist * 100.0f);
            displayDigits[0] = (large / 10000) % 10;
            displayDigits[1] = (large / 1000) % 10;
            displayDigits[2] = (large / 100) % 10;
            displayDigits[3] = (large / 10) % 10;
            displayDigits[4] = large % 10;

            unsigned int digitTextures[10] = {
                zero_texture, one_texture, two_texture, three_texture, four_texture,
                five_texture, six_texture, seven_texture, eight_texture, nine_texture
            };

            glUseProgram(textureShader);
            glActiveTexture(GL_TEXTURE0);

            for (int i = 0; i < 5; i++) {
                int d = displayDigits[i];
                glBindTexture(GL_TEXTURE_2D, digitTextures[d]);
                glBindVertexArray(digitVAO[i]);
                glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
            }
        }



        //draw index
        glUseProgram(textureShader);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, indexTexture);
        glBindVertexArray(VAOindex);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);



        if (rezimHodanja) {
            glBindTexture(GL_TEXTURE_2D, cikicaTexture);
        }
        else {
            glBindTexture(GL_TEXTURE_2D, rulerTexture);
        }


        // draw icon
        glActiveTexture(GL_TEXTURE0);
        glBindVertexArray(VAOicon);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);


        glfwSwapBuffers(window);
        glfwPollEvents();
        //std::cout << displacementDistance;
        while (glfwGetTime() - initFrameTime < 1 / 75.0) {} // fps limiter

        
    }


    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}