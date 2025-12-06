#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <ft2build.h>
#include FT_FREETYPE_H

#define _USE_MATH_DEFINES
#include <cmath> // za pi
#include <algorithm> // za max()
#include <iostream>
#include "../Header/Util.h"



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





GLFWcursor* cursor;
GLFWcursor* cursorPressed;

bool rezimHodanja = true;


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
void center_callback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {

        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        // Normalizacija da bi se pozicija izražena u pikselima namapirala na OpenGL-ov prozor sa opsegom (-1, 1)
        float xposNorm = (xpos / screenWidth) * 2 - 1;
        float yposNorm = -((ypos / screenHeight) * 2 - 1);
        std::cout << xposNorm << std::endl;
        std::cout << yposNorm;
        if (
            xposNorm < 0.7 && xposNorm > 0.6 && // Da li je kursor između leve i desne ivice kvadrata
            yposNorm < 1.0 && yposNorm > 0.9 // Da li je kursor između donje i gornje ivice kvadrata, sa uračunatim pomeranjem ivica u spljeskanom stanju
            ) rezimHodanja = !rezimHodanja;
    }

    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
        glfwSetCursor(window, cursor);
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



    unsigned int basicShader = createShader("basic.vert", "basic.frag");

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



    float walkingMap[] = {
        // x, y,  u, v
        -10.0f,  10.0f,   0.0f, 1.0f,   // top left
        -10.0f, -10.0f,   0.0f, 0.0f,   // bottom left
         10.0f, -10.0f,   1.0f, 0.0f,   // bottom right
         10.0f,  10.0f,   1.0f, 1.0f    // top right
    };
    unsigned int VAOwalkingMap;
    size_t walkingMapSize = sizeof(walkingMap);
    unsigned int VBOwalkingMap;
    glGenVertexArrays(1, &VAOwalkingMap);
    glGenBuffers(1, &VBOwalkingMap);

    glBindVertexArray(VAOwalkingMap);
    glBindBuffer(GL_ARRAY_BUFFER, VBOwalkingMap);
    glBufferData(GL_ARRAY_BUFFER, walkingMapSize, walkingMap, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
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






    float pinVertices[] = {
        -0.1f,  0.1f,   0.0f, 1.0f,   // top left
        -0.1f, -0.1f,   0.0f, 0.0f,   // bottom left
         0.1f, -0.1f,   1.0f, 0.0f,   // bottom right
         0.1f,  0.1f,   1.0f, 1.0f    // top right
    };
    unsigned int VAOpin;
    size_t pinSize = sizeof(pinVertices);
    unsigned int VBOpin;
    glGenVertexArrays(1, &VAOpin);
    glGenBuffers(1, &VBOpin);

    glBindVertexArray(VAOpin);
    glBindBuffer(GL_ARRAY_BUFFER, VAOpin);
    glBufferData(GL_ARRAY_BUFFER, pinSize, pinVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);



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
    glBufferData(GL_ARRAY_BUFFER, pinSize, indexVertices, GL_STATIC_DRAW);

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
    glBufferData(GL_ARRAY_BUFFER, pinSize, iconVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    float digitVertices[5][16];

    float startX = -0.25f;
    float endX = 0.25f;
    float width = (endX - startX) / 5.0f;   // = 0.12

    float topY = 1.0f;
    float bottomY = 0.85f;

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


    glClearColor(0.2f, 0.8f, 0.6f, 1.0f);

    while (!glfwWindowShouldClose(window))
    {

        double initFrameTime = glfwGetTime();
        float speed = 0.001f;
        glfwSetMouseButtonCallback(window, center_callback);



        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) { // esc = close
            break;
        }
        if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) { // R = promena rezima
            rezimHodanja = !rezimHodanja;
        }


        glClear(GL_COLOR_BUFFER_BIT);
        if (rezimHodanja) {
            //std::cout << rezimHodanja;
            float prevX = uX;
            float prevY = uY;

            if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) uY -= speed;
            if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) uY += speed;
            if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) uX += speed;
            if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) uX -= speed;

            // distancefrom start
            float dx = uX ;
            float dy = uY ;
            displacementDistance = std::sqrt(dx * dx + dy * dy);


            //draw map zoomed
            glUseProgram(mapShader);
            glUniform1f(glGetUniformLocation(mapShader, "uX"), uX);
            glUniform1f(glGetUniformLocation(mapShader, "uY"), uY);
            glUniform1f(glGetUniformLocation(mapShader, "uS"), uS);

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

            // prepare array of 5 digit indices (0–9)
            int displayDigits[5];

            // ---- integer part (first 3 digits) ----
            int d = (int)(dist * 100.0f);  // shift decimals so we can extract easily

            int digit0 = (d / 100000) % 10;  // thousands
            int digit1 = (d / 10000) % 10;  // thousands
            int digit2 = (d / 1000) % 10;  // hundreds
            int digit3 = (d / 100) % 10;  // tens

            // ---- first 2 decimal digits ----
            int firstDecimal = ((int)(dist * 10)) % 10;   // tenths

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






            //draw pin
            glUseProgram(textureShader);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, pinTexture);
            glBindVertexArray(VAOpin);
            glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

        }
        else {
            //draw regular map
            glUseProgram(textureShader);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, mapTexture);
            glBindVertexArray(VAOrulerMap);
            glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
        }



        //draw index
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