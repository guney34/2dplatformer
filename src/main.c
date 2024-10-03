#include <glad/glad.h> 
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "sprite.h"

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720

// basic shader that takes in a vec4 for each vert
// x, y: normalised position coordinates to screen
// w, z: normalised texture coordinates
const char *vertexShaderSource =
"#version 460 core\n"
"layout (location = 0) in vec4 vert;\n"
"out vec2 tex_coord;\n"
"void main()\n"
"{\n"
"   gl_Position = vec4(vert.xy, 0.0, 1.0);\n"
"   tex_coord = vert.zw;\n"
"}";

// takes in the texture coordinates passed from vert shader
// then calculates and sends colour data
const char *fragmentShaderSource =
"#version 460 core\n"
"out vec4 frag_colour;\n"
"in vec2 tex_coord;\n"
"uniform sampler2D sprite;\n"
"void main()\n"
"{\n"
"    frag_colour = texture(sprite, tex_coord);\n"
"}";

struct Player {
    float x, y;                       //screen-coordinates of bottom-left of sprite; centre coordinates would be better
    float vel_x, vel_y;               //velocity
    float acc_x, acc_y;               //acceleration
    float speed;                      //movement speed, added to vel_x if 'A' or(exclusive) 'D' is pressed
    bool is_facing_right;             //for flipping texture
    enum SpriteID animation_state;    //different set of sprites based on player input and status
} player;

struct Rect {
    float x, y;
    float width, height;
} tile; // floor tile

// key state
bool pressed_A = false;
bool pressed_D = false;
bool pressed_S = false;

static GLuint shaderProgram;
static GLuint VAOs[2], VBOs[2], EBOs[2]; // two sets of openGL objects; one for player and one for floor tile

static int img_width; //sprite sheet dimensions
static int img_height;

// basic player movement physics and logic for determining animation_state
void updatePlayer(void);

// callback function called whenever a key is pressed
static void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods);

// callback for window resize
static void windowSizeCallback(GLFWwindow *window, GLsizei width, GLsizei height);

// vertex and fragment shader initialisation
static void initShaders(void);

// updates the vertex data for VBOs[0]
static void drawPlayer(struct Sprite sprite, GLfloat x, GLfloat y, GLuint frame);
// updates the vertex data for VBOs[1]
static void drawTile(struct Sprite sprite, GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2);

int main(void)
{
    if (glfwInit() == GLFW_FALSE) { //initialise glfw window;
        fprintf(stderr, "Failed to initialize GLFW\n");
        return EXIT_FAILURE;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow *window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Platformer", NULL, NULL);
    if (window == NULL) {
        fprintf(stderr, "Failed to create GLFW window\n");
        glfwTerminate();
        return EXIT_FAILURE;
    }
    
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, windowSizeCallback);
    glfwSetKeyCallback(window, keyCallback);

    //manages function pointers for driver specific code
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        fprintf(stderr, "Failed to initialize GLAD\n");
        return EXIT_FAILURE;
    }

    // structure for creating rectangles; sets the order of the verts
    GLuint indices[] = {
        0, 1, 3,   // first triangle
        1, 2, 3    // second triangle
    };


    initShaders();

    //generate openGL objects and assign IDs
    glGenVertexArrays(2, VAOs);
    glGenBuffers(2, VBOs);
    glGenBuffers(2, EBOs);

    //player
    glBindVertexArray(VAOs[0]);
    glBindBuffer(GL_ARRAY_BUFFER, VBOs[0]);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBOs[0]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    //tile
    glBindVertexArray(VAOs[1]);
    glBindBuffer(GL_ARRAY_BUFFER, VBOs[1]);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBOs[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    //enable RGBA transparency 
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


    GLuint texture; // texture to store spritesheet in GPU memory
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D_ARRAY, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    //initial values for player object
    player.x = -0.5;
    player.y = -0.5;
    player.vel_x = 0.0f;
    player.vel_y = 0;
    player.acc_x = 0.0f;
    player.acc_y = -0.001f;
    player.speed = 0.005;
    player.is_facing_right = true;
    player.animation_state = PLAYER_IDLE;

    tile.x = -1;
    tile.y = -1;
    tile.width = 2;
    tile.height = 0.2;

    struct Sprite tile_sprite = getSprite(TILE_ROCK);
    
    int nrChannels;
    stbi_set_flip_vertically_on_load(true);
    GLubyte *img_data = stbi_load("assets/spritesheet.png", &img_width, &img_height, &nrChannels, 0);
    if (img_data == NULL) {
        fprintf(stderr, "Failed to load sprite\n");
        return EXIT_FAILURE;
    }
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img_width, img_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, img_data);
    glGenerateMipmap(GL_TEXTURE_2D);
    stbi_image_free(img_data);

    

    while (!glfwWindowShouldClose(window)) {

        //update ph
        updatePlayer();
        struct Sprite player_sprite = getSprite(player.animation_state);


        //render
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // 10 animation frames in 1 second
        GLuint time = (GLuint)(10*glfwGetTime()) % player_sprite.num_frames;
        drawPlayer(player_sprite, player.x, player.y, time);

        drawTile(tile_sprite, tile.x, tile.y, tile.x + tile.width, tile.y + tile.height);

        printf("%f, %f\n", player.x, player.y);
        glBindTexture(GL_TEXTURE_2D, texture);

        glUseProgram(shaderProgram);
        glBindVertexArray(VAOs[0]);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(VAOs[1]);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
}

void updatePlayer(void)
{
    if (player.vel_y > 0) {                                     // if player is ascending, use JUMP animation
        player.animation_state = PLAYER_JUMP;
    } else if (pressed_A == true && pressed_D == false) {       // if 'A' is pressed, player moves at movement speed to the left
        if (pressed_S == true) {
            player.animation_state = PLAYER_CROUCH_WALK;        // at half speed if crouched
            player.vel_x = -0.5f*player.speed;
        } else {
            player.animation_state = PLAYER_RUN;
            player.vel_x = -player.speed;
        }
    } else if (pressed_A == false && pressed_D == true) {       // same for other direction
        if (pressed_S == true) {
            player.animation_state = PLAYER_CROUCH_WALK;
            player.vel_x = 0.5f*player.speed;
        } else {
            player.animation_state = PLAYER_RUN;
            player.vel_x = player.speed;
        }
    } else if (pressed_S == true) {
        player.animation_state = PLAYER_CROUCH;                 // crouch animation, zeros x velocity
        player.vel_x = 0;
    } else {
        player.animation_state = PLAYER_IDLE;                   // idle animation, zeros x velocity
        player.vel_x = 0;
    }
    
    // basic floor tile collision
    // if the player is not ascending and is touching the tile, set the velocity to zero
    if (player.y <= tile.y + tile.height && player.vel_y <= 0) { 
        player.vel_y = 0;
        player.y = tile.y + tile.height;
    } 

    player.x += player.vel_x;     // update position every frame by vel_x
    player.vel_y += player.acc_y; //downward acceleration is constant to mimick gravity
    player.y += player.vel_y;     
}

static void drawTile(struct Sprite sprite, GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2)
{
    float tex_x = sprite.x;
    float tex_y = img_height - sprite.y; //convert top-left coords to bottom-left

    float right_x = (tex_x + sprite.width)/(float)img_width;
    float left_x = (tex_x)/(float)img_width;

    float top_y = (tex_y)/(float)img_height;
    float bottom_y = (tex_y - sprite.height)/(float)img_height;


    GLfloat vertices[] = {
        //pos   //texture coords
        x2, y2, right_x,    top_y,    // top right
        x2, y1, right_x, bottom_y,    // bottom right
        x1, y1, left_x,  bottom_y,    // bottom left
        x1, y2, left_x,     top_y     // top left 
    };

    glBindBuffer(GL_ARRAY_BUFFER, VBOs[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
}

static void drawPlayer(struct Sprite sprite, GLfloat x, GLfloat y, GLuint frame)
{
    float tex_x = sprite.x;
    float tex_y = img_height - sprite.y; //convert top-left coords to bottom-left

    float right_x = (tex_x + (frame+1) * sprite.width)/(float)img_width;
    float left_x = (tex_x + frame * sprite.width)/(float)img_width;

    float top_y = (tex_y)/(float)img_height;
    float bottom_y = (tex_y - sprite.height)/(float)img_height;

    //flip texture if player is facing left
    if (player.is_facing_right == false) {
        float temp = right_x;
        right_x = left_x;
        left_x = temp;
    }

    GLfloat vertices[] = {
        //pos   //texture coords
        x+1, y+1,  right_x,    top_y,    // top right
        x+1, y,    right_x, bottom_y,    // bottom right
        x,   y,    left_x,  bottom_y,    // bottom left
        x,   y+1,  left_x,     top_y     // top left 
    };

    glBindBuffer(GL_ARRAY_BUFFER, VBOs[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
}

static void windowSizeCallback(GLFWwindow *window, GLsizei width, GLsizei height)
{
    glViewport(0, 0, width, height);
}

static void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    switch (key) {
        case GLFW_KEY_A:
            if (action == GLFW_RELEASE)
                pressed_A = false;
            else if (action == GLFW_PRESS) {
                pressed_A = true;
                player.is_facing_right = false;
            }
            break;
        case GLFW_KEY_D:
            if (action == GLFW_RELEASE)
                pressed_D = false;
            else if (action == GLFW_PRESS) {
                pressed_D = true;
                player.is_facing_right = true;
            }
            break;
        case GLFW_KEY_S:
            if (action == GLFW_RELEASE)
                pressed_S = false;
            else if (action == GLFW_PRESS) {
                pressed_S = true;
            }
            break;
        case GLFW_KEY_SPACE:
            if (action == GLFW_RELEASE)
                player.vel_y += 0.05f; //add y vel when player 'jumps'
            break;
    }
}

void initShaders(void)
{
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}
