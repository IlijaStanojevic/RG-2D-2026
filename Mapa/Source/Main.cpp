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


#include "../shader.hpp"
#include "../model.hpp"

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
float camPitch = -0.3f;      // tilt (radians)
float camDist = 3.0f;      // zoom
glm::vec3 camTarget(0, 0, 0); // map center
float camSpeed = 0.1f;

glm::mat4 gView(1.0f);
glm::mat4 gProj(1.0f);
glm::vec3 gCamPos(0.0f);



glm::vec3 humanoidPos = glm::vec3(0.0f, 0.0f, 0.0f);
float humanoidYaw = 0.0f;
float moveSpeed = 1.0f; 


bool depthEnabled = true;
bool cullEnabled = false;  
bool keyDepthPrev = false;
bool keyCullPrev = false;


glm::vec3 lightPos = glm::vec3(0.0f, 9.0f, 2.0f);
glm::vec3 lightColor = glm::vec3(0.6f, 0.6f, 0.6f);



GLFWcursor* cursor;
GLFWcursor* cursorPressed;

bool rezimHodanja = true;


struct MeasurePoint {
    float x, y, z;
};
std::vector<MeasurePoint> measurePoints;
float totalMeasureDistance = 0.0f;

GLuint measureVAO, measureVBO;

static float distOnMap(const MeasurePoint& a, const MeasurePoint& b) {
    float dx = b.x - a.x;
    float dy = b.y - a.y;
    return sqrtf(dx * dx + dy * dy)/10;
}




void updateMeasureBuffer()
{
    glBindBuffer(GL_ARRAY_BUFFER, measureVBO);

    if (!measurePoints.empty())
        glBufferSubData(GL_ARRAY_BUFFER, 0,
            (GLsizeiptr)(measurePoints.size() * sizeof(MeasurePoint)),
            measurePoints.data());
}

void addMeasurePoint(const MeasurePoint& p)
{
    if (!measurePoints.empty()) {
        const auto& prev = measurePoints.back();
        totalMeasureDistance += distOnMap(prev, p);
    }
    measurePoints.push_back(p);
    updateMeasureBuffer();
}
struct Ray {
    glm::vec3 origin;
    glm::vec3 dir; // normalized
};

static Ray makePickRay(double mouseX, double mouseY,
    int screenW, int screenH,
    const glm::mat4& proj, const glm::mat4& view,
    const glm::vec3& camPos)
{
    // NDC
    float x = (float)(2.0 * mouseX / screenW - 1.0);
    float y = (float)(1.0 - 2.0 * mouseY / screenH);

    glm::mat4 invPV = glm::inverse(proj * view);

    glm::vec4 nearNdc(x, y, -1.0f, 1.0f);
    glm::vec4 farNdc(x, y, 1.0f, 1.0f);

    glm::vec4 nearWorld = invPV * nearNdc;
    glm::vec4 farWorld = invPV * farNdc;
    nearWorld /= nearWorld.w;
    farWorld /= farWorld.w;

    glm::vec3 o = glm::vec3(nearWorld);          // or camPos
    glm::vec3 d = glm::normalize(glm::vec3(farWorld - nearWorld));

    return { o, d };
}
static float distPointToRay(const glm::vec3& p, const Ray& r)
{
    // distance from point to infinite ray line (clamp t>=0)
    glm::vec3 v = p - r.origin;
    float t = glm::dot(v, r.dir);
    if (t < 0.0f) t = 0.0f;
    glm::vec3 closest = r.origin + t * r.dir;
    return glm::length(p - closest);
}


static bool intersectPlane(const Ray& r, float planeZ, glm::vec3& hit)
{
    const float denom = r.dir.z;
    if (fabs(denom) < 1e-6f) return false;

    float t = (planeZ - r.origin.z) / denom;
    if (t < 0.0f) return false;

    hit = r.origin + t * r.dir;
    return true;
}



void deletePoint(int idx)
{
    if (idx < 0 || idx >= (int)measurePoints.size()) return;

    if (idx > 0) {
        auto& a = measurePoints[idx - 1];
        auto& b = measurePoints[idx];
        totalMeasureDistance -= distOnMap(a, b);
    }
    if (idx < (int)measurePoints.size() - 1) {
        auto& a = measurePoints[idx];
        auto& b = measurePoints[idx + 1];
        totalMeasureDistance -= distOnMap(a, b);
    }
    if (idx > 0 && idx < (int)measurePoints.size() - 1) {
        auto& a = measurePoints[idx - 1];
        auto& b = measurePoints[idx + 1];
        totalMeasureDistance += distOnMap(a, b);
    }

    measurePoints.erase(measurePoints.begin() + idx);
    updateMeasureBuffer();
}

void drawMeasureLines(GLuint lineShader, const glm::mat4& mvp)
{
    if (measurePoints.size() < 2) return;

    glUseProgram(lineShader);
    glUniformMatrix4fv(glGetUniformLocation(lineShader, "uMVP"), 1, GL_FALSE, &mvp[0][0]);
    glUniform3f(glGetUniformLocation(lineShader, "uColor"), 1.0f, 1.0f, 1.0f);

    glBindVertexArray(measureVAO);

    // Make lines visible above the map without breaking depth:
    glEnable(GL_DEPTH_TEST);
    // optional: polygon offset for lines (helps a bit)
    glEnable(GL_POLYGON_OFFSET_LINE);
    glPolygonOffset(-1.0f, -1.0f);

    glDrawArrays(GL_LINE_STRIP, 0, (GLsizei)measurePoints.size());

    glDisable(GL_POLYGON_OFFSET_LINE);
    glBindVertexArray(0);
}


void drawMeasureDots(GLuint lineShader)
{
    glUseProgram(lineShader);
    glUniform3f(glGetUniformLocation(lineShader, "color"), 1, 1, 1);

    glBindVertexArray(measureVAO);
    glDrawArrays(GL_POINTS, 0, measurePoints.size());
}
void drawMeasurePins(Model& pin, Shader& shader,
    const glm::mat4& view, const glm::mat4& proj,
    const glm::vec3& lightPos, const glm::vec3& lightColor,
    float pinScale = 0.005f)
{
    const float PIN_LIFT = 0.15f;
    if (measurePoints.empty()) return;

    shader.use();
    shader.setVec3("uLightPos", lightPos);
    shader.setVec3("uLightColor", lightColor);
    shader.setMat4("uV", view);
    shader.setMat4("uP", proj);

    for (const auto& mp : measurePoints)
    {
        glm::mat4 m(1.0f);
        m = glm::translate(m, glm::vec3(mp.x, mp.y, mp.z + PIN_LIFT));
        m = glm::rotate(m, glm::radians(90.0f), glm::vec3(1, 0, 0));
        m = glm::scale(m, glm::vec3(pinScale));

        shader.setMat4("uM", m);
        pin.Draw(shader);
    }
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

    double mx, my;
    glfwGetCursorPos(window, &mx, &my);
    float x = (mx / screenWidth) * 2.0f - 1.0f;
    float y = 1.0f - (my / screenHeight) * 2.0f;
    if (x < 0.7 && x > 0.6 && y < 1.0 && y > 0.9) {
        rezimHodanja = !rezimHodanja;
        return;
    }

    if (rezimHodanja) return;

    // Build a pick ray using your current camera matrices
    Ray ray = makePickRay(mx, my, screenWidth, screenHeight, gProj, gView, gCamPos);

    // Intersect with the map plane
    glm::vec3 hit;
    const float MAP_Y = 0.0f;          // set this to your map's Y
    if (!intersectPlane(ray, MAP_Y, hit)) return;

    // Optional: clamp to map extents
    // if (!insideMapXZ(hit, mapMinX, mapMaxX, mapMinZ, mapMaxZ)) return;

    // Lift slightly so it renders on top (avoid z-fighting)
    hit.z += 0.01f;

    // Delete if close to an existing point
    const float DELETE_RADIUS_WORLD = 0.15f; // tune to your world scale
    int bestIdx = -1;
    float bestD = 1e9f;

    for (int i = 0; i < (int)measurePoints.size(); i++) {
        glm::vec3 p(measurePoints[i].x, measurePoints[i].y, measurePoints[i].z);
        float d = distPointToRay(p, ray);
        if (d < bestD) { bestD = d; bestIdx = i; }
    }

    if (bestIdx != -1 && bestD < DELETE_RADIUS_WORLD) {
        deletePoint(bestIdx);
        return;
    }

    addMeasurePoint(MeasurePoint{ hit.x, hit.y, hit.z });
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

    glDisable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);


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
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 3 * 1000, NULL, GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);


    Model humanoid("resources/humanoid.obj");
    Model pin("resources/pin2.obj");
    Shader unifiedShader("model.vert", "model.frag");






    glfwSetMouseButtonCallback(window, mouse_click_callback);



    glClearColor(0.2f, 0.8f, 0.6f, 1.0f);

    while (!glfwWindowShouldClose(window))
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        double initFrameTime = glfwGetTime();
        float speed = 0.001f;
        

        glm::vec3 camPos;
        gCamPos.x = camTarget.x + camDist * cos(camPitch) * sin(camYaw);
        gCamPos.y = camTarget.y + camDist * sin(camPitch);
        gCamPos.z = camTarget.z + camDist * cos(camPitch) * cos(camYaw);

        gView = glm::lookAt(gCamPos, camTarget, glm::vec3(0, 1, 0));
        gProj = glm::perspective(glm::radians(60.0f),
            screenWidth / (float)screenHeight,
            0.1f, 500.0f);

        glm::mat4 model = glm::mat4(1.0f);



        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) { // esc = close
            break;
        }
        if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) { // R = promena rezima
            rezimHodanja = !rezimHodanja;
        }
        bool k1 = glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS;
        if (k1 && !keyDepthPrev) {
            depthEnabled = !depthEnabled;
            if (depthEnabled) glEnable(GL_DEPTH_TEST);
            else glDisable(GL_DEPTH_TEST);
        }
        keyDepthPrev = k1;

        bool k2 = glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS;
        if (k2 && !keyCullPrev) {
            cullEnabled = !cullEnabled;
            if (cullEnabled) glEnable(GL_CULL_FACE);
            else glDisable(GL_CULL_FACE);
        }
        keyCullPrev = k2;

        glClear(GL_COLOR_BUFFER_BIT);
        if (rezimHodanja) {
            camDist = 3.0f;
            glm::vec3 prevHumanoidPos = humanoidPos;  // previous location

            static float lastTime = glfwGetTime();
            float currentTime = glfwGetTime();
            float dt = currentTime - lastTime;
            lastTime = currentTime;

            float step = moveSpeed * dt;

            if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
                humanoidPos.y += step;      
                humanoidYaw = 180.0f;
            }
            if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
                humanoidPos.y -= step;
                humanoidYaw = 0.0f;
            }
            if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
                humanoidPos.x -= step;
                humanoidYaw = -90.0f;
            }
            if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
                humanoidPos.x += step;
                humanoidYaw = 90.0f;
            }
            if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
                camTarget.y += camSpeed;

            if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
                camTarget.y -= camSpeed;

            if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
                camTarget.x -= camSpeed;

            if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
                camTarget.x += camSpeed;

            // X bounds
            if (camTarget.x < -7.0f)
                camTarget.x = -7.0f;

            if (camTarget.x > 7.0f)
                camTarget.x = 7.0f;

            if (camTarget.y < -8.0f)
                camTarget.y = -8.0f;

            if (camTarget.y > 8.0f)
                camTarget.y = 8.0f;


            glm::vec3 delta = humanoidPos - prevHumanoidPos;
            delta.z = 0.0f;

            displacementDistance += glm::length(delta);




            //draw map zoomed
            glUseProgram(mapShader);

            glUniformMatrix4fv(glGetUniformLocation(mapShader, "uModel"), 1, GL_FALSE, &model[0][0]);
            glUniformMatrix4fv(glGetUniformLocation(mapShader, "uView"), 1, GL_FALSE, &gView[0][0]);
            glUniformMatrix4fv(glGetUniformLocation(mapShader, "uProj"), 1, GL_FALSE, &gProj[0][0]);

            glUniform3fv(glGetUniformLocation(mapShader, "uLightPos"), 1, &lightPos[0]);
            glUniform3fv(glGetUniformLocation(mapShader, "uLightColor"), 1, &lightColor[0]);

            glUniform1i(glGetUniformLocation(mapShader, "uTex0"), 0);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, mapTexture);

            glBindVertexArray(VAOwalkingMap);
            glDrawArrays(GL_TRIANGLE_FAN, 0, 4);


            // draw humanoid
            unifiedShader.use();
            unifiedShader.setVec3("uLightPos", lightPos);
            unifiedShader.setVec3("uLightColor", lightColor);
            unifiedShader.setMat4("uP", gProj);
            unifiedShader.setMat4("uV", gView);
            // MOVE
            model = glm::translate(model, humanoidPos);

            // ORIENT (stand upright)
            model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1, 0, 0));
            // FACING
            model = glm::rotate(model, glm::radians(humanoidYaw), glm::vec3(0, 1, 0));
            // SCALE
            model = glm::scale(model, glm::vec3(0.1f));
            unifiedShader.setMat4("uM", model);
            humanoid.Draw(unifiedShader);

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

            //glUseProgram(basicShader);

            //// Send matrices
            //glUniformMatrix4fv(glGetUniformLocation(basicShader, "uModel"), 1, GL_FALSE, &model[0][0]);
            //glUniformMatrix4fv(glGetUniformLocation(basicShader, "uView"), 1, GL_FALSE, &view[0][0]);
            //glUniformMatrix4fv(glGetUniformLocation(basicShader, "uProj"), 1, GL_FALSE, &proj[0][0]);

            ////// Optional: give it its own model transform (recommended)
            ////glm::mat4 cubeModel = glm::mat4(1.0f);
            ////cubeModel = glm::translate(cubeModel, glm::vec3(0.0f, 0.0f, 0.0f));
            ////// cubeModel = glm::rotate(cubeModel, (float)glfwGetTime(), glm::vec3(0, 1, 0));
            ////glUniformMatrix4fv(glGetUniformLocation(basicShader, "uModel"), 1, GL_FALSE, &cubeModel[0][0]);

            //// Draw
            //glBindVertexArray(VAOcube);
            //glDrawArrays(GL_TRIANGLES, 0, 36);
            //glBindVertexArray(0);

        }
        else {
            // Draw full map
            //draw map zoomed
            camDist = 8.f;
            if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
                camTarget.y += camSpeed;

            if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
                camTarget.y -= camSpeed;

            if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
                camTarget.x -= camSpeed;

            if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
                camTarget.x += camSpeed;

            // X bounds
            if (camTarget.x < -2.0f)
                camTarget.x = -2.0f;

            if (camTarget.x > 2.0f)
                camTarget.x = 2.0f;

            if (camTarget.y < -4.0f)
                camTarget.y = -4.0f;

            if (camTarget.y > 4.0f)
                camTarget.y = 4.0f;





            glUseProgram(mapShader);

            glUniformMatrix4fv(glGetUniformLocation(mapShader, "uModel"), 1, GL_FALSE, &model[0][0]);
            glUniformMatrix4fv(glGetUniformLocation(mapShader, "uView"), 1, GL_FALSE, &gView[0][0]);
            glUniformMatrix4fv(glGetUniformLocation(mapShader, "uProj"), 1, GL_FALSE, &gProj[0][0]);

            glUniform3fv(glGetUniformLocation(mapShader, "uLightPos"), 1, &lightPos[0]);
            glUniform3fv(glGetUniformLocation(mapShader, "uLightColor"), 1, &lightColor[0]);

            glUniform1i(glGetUniformLocation(mapShader, "uTex0"), 0);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, mapTexture);

            glBindVertexArray(VAOwalkingMap);
            glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

            glm::mat4 mvp = gProj * gView * glm::mat4(1.0f);
            drawMeasureLines(lineShader, mvp);


            drawMeasurePins(pin, unifiedShader, gView, gProj, lightPos, lightColor, 0.05f);

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
        glUseProgram(textureShader);
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