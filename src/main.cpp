#include <SDL2/SDL.h>
#include <SDL_events.h>
#include <SDL_render.h>
#include <cstdlib>
#include <glm/ext/quaternion_geometric.hpp>
#include <glm/geometric.hpp>
#include <string>
#include <glm/glm.hpp>
#include <vector>
#include "./fps.h"
#include "color.h"
#include "intersect.h"
#include "object.h"
#include "cube.h"
#include "light.h"
#include "camera.h"
#include "skybox.h"

#include "./materials/grass.h"
#include "./materials/stone.h"
#include "./materials/cobblestone.h"
#include "./materials/gold.h"
#include "./materials/obsidian.h"
#include "./materials/rail.h"
#include "./materials/coal.h"
#include "./materials/glass.h"
#include "./materials/chest.h"
#include "./materials/pumpkin.h"

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;
const float ASPECT_RATIO = static_cast<float>(SCREEN_WIDTH) / static_cast<float>(SCREEN_HEIGHT);
const int MAX_RECURSION = 3;
const float BIAS = 0.0001f;

SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;
std::vector<Object*> objects;
Light light = {glm::vec3(-10.0f, 10.0f, 20.0f), 1.0f, Color(255, 255, 255)};
Camera camera(glm::vec3(-3.0f, 2.0f, 10.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), 10.0f);

bool init() {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "Error: No se puedo inicializar SDL: " << SDL_GetError() << std::endl;
        return false;
    }

    window = SDL_CreateWindow("Proyecto 3: Raytracing", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (!window) {
        std::cerr << "Error: No se pudo crear una ventana SDL: " << SDL_GetError() << std::endl;
        return false;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        std::cerr << "Error: No se pudo crear SDL_Renderer: " << SDL_GetError() << std::endl;
        return false;
    }

    return true;
}

void point(glm::vec2 position, Color color) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderDrawPoint(renderer, position.x, position.y);
}

float castShadow(const glm::vec3& shadowOrigin, const glm::vec3& lightDir, Object* hitObject) {
    for (auto& obj : objects) {
        if (obj != hitObject) {
            Intersect shadowIntersect = obj->rayIntersect(shadowOrigin, lightDir);
            if (shadowIntersect.isIntersecting && shadowIntersect.dist > 0) {
                float shadowRatio = shadowIntersect.dist / glm::length(light.position - shadowOrigin);
                shadowRatio = glm::min(1.0f, shadowRatio);
                return 1.0f - shadowRatio;
            }
        }
    }
    return 1.0f;
}

Color castRay(const glm::vec3& rayOrigin, const glm::vec3& rayDirection, const short recursion = 0, Object* currentObj = nullptr) {
    float zBuffer = 99999;
    Object* hitObject = nullptr;
    Intersect intersect;

    for (const auto& object : objects) {
        Intersect i = object->rayIntersect(rayOrigin, rayDirection);
        if (i.isIntersecting && i.dist < zBuffer && currentObj != object) {
            zBuffer = i.dist;
            hitObject = object;
            intersect = i;
        }
    }

    if (!intersect.isIntersecting || recursion == MAX_RECURSION) {
        return Skybox::getColor(rayOrigin, rayDirection);
        //return Color(173, 216, 230);
    }


    glm::vec3 lightDir = glm::normalize(light.position - intersect.point);
    glm::vec3 viewDir = glm::normalize(rayOrigin - intersect.point);
    glm::vec3 reflectDir = glm::reflect(-glm::normalize(rayOrigin), intersect.normal); 
    //glm::vec3 reflectDir = glm::reflect(-lightDir, intersect.normal); 
    

    float shadowIntensity = castShadow(intersect.point, lightDir, hitObject);

    float diffuseLightIntensity = std::max(0.0f, glm::dot(intersect.normal, lightDir));
    float specReflection = glm::dot(viewDir, reflectDir);
    
    Material mat = hitObject->material;

    float specLightIntensity = std::pow(std::max(0.0f, glm::dot(viewDir, reflectDir)), mat.specularCoefficient);


    Color reflectedColor(0.0f, 0.0f, 0.0f);
    if (mat.reflectivity > 0) {
        glm::vec3 origin = intersect.point + intersect.normal * BIAS;
        reflectedColor = castRay(origin, reflectDir, recursion + 1, hitObject); 
    }

    Color refractedColor(0.0f, 0.0f, 0.0f);
    if (mat.transparency > 0) {
        glm::vec3 origin = intersect.point - intersect.normal * BIAS;
        glm::vec3 refractDir = glm::refract(rayDirection, intersect.normal, mat.refractionIndex);
        refractedColor = castRay(origin, refractDir, recursion + 1, hitObject); 
    }

    Color materialLight = intersect.hasColor ? intersect.color : mat.diffuse;

    Color diffuseLight = materialLight * light.intensity * diffuseLightIntensity * mat.albedo * shadowIntensity;
    Color specularLight = light.color * light.intensity * specLightIntensity * mat.specularAlbedo * shadowIntensity;
    Color color = (diffuseLight + specularLight) * (1.0f - mat.reflectivity - mat.transparency) + reflectedColor * mat.reflectivity + refractedColor * mat.transparency;
    return color;
} 

void setUp() {

    Material stone = {
        Color(0, 0, 0),
        0.85,
        0.0,
        0.50f,
        0.0f,
        0.0f
    };
    

    Material obsidian = {
        Color(0, 0, 0),
        0.9,
        0.1f,
        0.1f,
        0.1f,
        0.0f
    };

    Material gold = {
        Color(228, 190, 39),
        1.0f,
        10.0f,
        0.2f,
        0.4f,
        0.0f
    };

    Material glass = {
        Color(255, 255, 255),
        0.0f,
        10.0f,
        1425.0f,
        0.2f,
        1.0f,
        1.1f
    };

    Material coal = {
        Color(0, 0, 0),
        0.85,
        0.0,
        0.50f,
        0.0f,
        0.0f
    };


    // Grama
    objects.push_back(new Grass(glm::vec3(-2.5f, -0.5f, -4.5f), glm::vec3(2.5f, 0.5f, 1.5f), stone));

    // Piedra
    objects.push_back(new Stone(glm::vec3(-2.5f, -0.5f, 0.5f), glm::vec3(-1.5f, -5.5f, 1.5f), stone));
    objects.push_back(new Stone(glm::vec3(1.5f, -0.5f, 0.5f), glm::vec3(2.5f, -5.5f, 1.5f), stone));

    objects.push_back(new Stone(glm::vec3(-1.5f, -0.5f, 0.5f), glm::vec3(-0.5f, -1.0f, 1.5f), stone));
    objects.push_back(new Stone(glm::vec3(-1.5f, -1.0f, 0.5f), glm::vec3(-1.0f, -1.5f, 1.5f), stone));
    
    objects.push_back(new Stone(glm::vec3(0.5f, -0.5f, 0.5f), glm::vec3(1.5f, -1.0f, 1.5f), stone));
    objects.push_back(new Stone(glm::vec3(1.0f, -1.0f, 0.5f), glm::vec3(1.5f, -1.5f, 1.5f), stone));

    objects.push_back(new Stone(glm::vec3(-1.5f, -4.5f, 0.5f), glm::vec3(-0.5f, -4.0f, 1.5f), stone));
    objects.push_back(new Stone(glm::vec3(-1.5f, -4.0f, 0.5f), glm::vec3(-1.0f, -3.5f, 1.5f), stone));

    objects.push_back(new Stone(glm::vec3(0.5f, -4.5f, 0.5f), glm::vec3(1.5f, -4.0f, 1.5f), stone));
    objects.push_back(new Stone(glm::vec3(1.0f, -4.0f, 0.5f), glm::vec3(1.5f, -3.5f, 1.5f), stone));

    objects.push_back(new Stone(glm::vec3(-2.5f, -5.5f, -1.5f), glm::vec3(-0.5f, -4.5f, -3.5f), stone));
    objects.push_back(new Stone(glm::vec3(-2.5f, -0.5f, -4.5f), glm::vec3(-1.5f, -5.5f, -3.5f), stone));
    objects.push_back(new Stone(glm::vec3(1.5f, -0.5f, -4.5f), glm::vec3(2.5f, -5.5f, -3.5f), stone));

    objects.push_back(new Stone(glm::vec3(-2.5f, -5.5f, 0.5f), glm::vec3(2.5f, -4.5f, -0.5f), stone));
    objects.push_back(new Stone(glm::vec3(0.5f, -4.5f, -3.5f), glm::vec3(1.5f, -5.5f, -2.5f), stone));
    objects.push_back(new Stone(glm::vec3(1.5f, -4.5f, -2.5f), glm::vec3(2.5f, -5.5f, -1.5f), stone));
    objects.push_back(new Stone(glm::vec3(-2.5f, -5.5f, -0.5f), glm::vec3(-1.5f, -4.5f, -1.5f), stone));
    objects.push_back(new Stone(glm::vec3(-0.5f, -4.5f, 0.5f), glm::vec3(0.5f, -5.0f, 1.5f), stone));

    // Riel
    objects.push_back(new Rail(glm::vec3(-1.5f, -5.5f, -0.5f), glm::vec3(2.5f, -4.5f, -1.5f), stone));

    // Piedra labrada
    objects.push_back(new Cobblestone(glm::vec3(-1.5f, -3.5f, -4.5f), glm::vec3(-0.5f, -4.5f, -3.5f), stone));
    objects.push_back(new Cobblestone(glm::vec3(-0.5f, -3.5f, -4.5f), glm::vec3(0.5f, -4.5f, -3.5f), stone));
    objects.push_back(new Cobblestone(glm::vec3(0.5f, -0.5f, -4.5f), glm::vec3(1.5f, -1.5f, -3.5f), stone));
    objects.push_back(new Cobblestone(glm::vec3(-1.5f, -1.5f, -4.5f), glm::vec3(0.5f, -2.5f, -3.5f), stone));
    objects.push_back(new Cobblestone(glm::vec3(0.5f, -2.5f, -4.5f), glm::vec3(1.5f, -3.5f, -3.5f), stone));
    objects.push_back(new Cobblestone(glm::vec3(-0.5f, -0.5f, -4.5f), glm::vec3(0.5f, -1.5f, -3.5f), stone));
    objects.push_back(new Cobblestone(glm::vec3(-0.5f, -4.5f, -2.5f), glm::vec3(0.5f, -5.5f, -1.5f), stone));
    objects.push_back(new Cobblestone(glm::vec3(1.5f, -4.5f, -3.5f), glm::vec3(2.5f, -5.5f, -2.5f), stone));
    
    objects.push_back(new Cobblestone(glm::vec3(-2.5f, -0.5f, -3.5f), glm::vec3(-1.5f, -1.5f, -2.5f), stone));
    objects.push_back(new Cobblestone(glm::vec3(-2.5f, -0.5f, -0.5f), glm::vec3(-1.5f, -1.5f, 0.5f), stone));
    objects.push_back(new Cobblestone(glm::vec3(-2.5f, -3.5f, -0.5f), glm::vec3(-1.5f, -4.5f, 0.5f), stone));

    objects.push_back(new Cobblestone(glm::vec3(0.5f, 0.5f, -3.5f), glm::vec3(1.5f, 1.5f, -2.5f), stone));
    objects.push_back(new Cobblestone(glm::vec3(0.5f, 0.5f, -0.5f), glm::vec3(1.5f, 1.5f, 0.5f), stone));
    objects.push_back(new Cobblestone(glm::vec3(0.5f, 3.5f, -0.5f), glm::vec3(1.5f, 4.5f, 0.5f), stone));
    objects.push_back(new Cobblestone(glm::vec3(0.5f, 3.5f, -3.5f), glm::vec3(1.5f, 4.5f, -2.5f), stone));

    // Carbón
    objects.push_back(new Coal(glm::vec3(-0.5f, -4.5f, -3.5f), glm::vec3(0.5f, -5.5f, -2.5f), coal));
    objects.push_back(new Coal(glm::vec3(0.5f, -4.5f, -2.5f), glm::vec3(1.5f, -5.5f, -1.5f), coal));
    objects.push_back(new Coal(glm::vec3(0.5f, -3.5f, -4.5f), glm::vec3(1.5f, -4.5f, -3.5f), coal));
    objects.push_back(new Coal(glm::vec3(-1.5f, -2.5f, -4.5f), glm::vec3(-0.5f, -3.5f, -3.5f), coal));
    objects.push_back(new Coal(glm::vec3(-1.5f, -0.5f, -4.5f), glm::vec3(-0.5f, -1.5f, -3.5f), coal));

    // Cofre
    objects.push_back(new Chest(glm::vec3(-1.5f, -4.5f, -2.5f), glm::vec3(-0.5f, -3.5f, -3.5f), stone));

    // Obsidiana
    objects.push_back(new Obsidian(glm::vec3(0.5f, 0.5f, -2.5f), glm::vec3(1.5f, 1.5f, -0.5f), obsidian));
    objects.push_back(new Obsidian(glm::vec3(0.5f, 3.5f, -2.5f), glm::vec3(1.5f, 4.5f, -0.5f), obsidian));
    objects.push_back(new Obsidian(glm::vec3(0.5f, 1.5f, -3.5f), glm::vec3(1.5f, 3.5f, -2.5f), obsidian));
    objects.push_back(new Obsidian(glm::vec3(0.5f, 1.5f, -0.5f), glm::vec3(1.5f, 3.5f, 0.5f), obsidian));

    //Oro
    objects.push_back(new Gold(glm::vec3(-0.5f, -2.5f, -4.5f), glm::vec3(0.5f, -3.5f, -3.5f), gold));
    objects.push_back(new Gold(glm::vec3(0.5f, -1.5f, -4.5f), glm::vec3(1.5f, -2.5f, -3.5f), gold));

    // Vidrio
    objects.push_back(new Glass(glm::vec3(-2.5f, -1.5f, -2.5f), glm::vec3(-1.5f, -3.5f, -0.5f), glass));

    //Calabaza
    objects.push_back(new Pumpkin(glm::vec3(-1.5f, 0.5f, -0.5f), glm::vec3(-0.5f, 1.5f, 0.5f), stone));
}

void render() {
    float fov = 3.1415/3;
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        for (int x = 0; x < SCREEN_WIDTH; x++) {

            // Develop render
            
            /*float random_value = static_cast<float>(std::rand())/static_cast<float>(RAND_MAX);
            if (random_value < 0.5) {
                continue;
            }*/



            float screenX = (2.0f * (x + 0.5f)) / SCREEN_WIDTH - 1.0f;
            float screenY = -(2.0f * (y + 0.5f)) / SCREEN_HEIGHT + 1.0f;
            screenX *= ASPECT_RATIO;
            screenX *= tan(fov/2.0f);
            screenY *= tan(fov/2.0f);


            glm::vec3 cameraDir = glm::normalize(camera.target - camera.position);

            glm::vec3 cameraX = glm::normalize(glm::cross(cameraDir, camera.up));
            glm::vec3 cameraY = glm::normalize(glm::cross(cameraX, cameraDir));
            glm::vec3 rayDirection = glm::normalize(
                cameraDir + cameraX * screenX + cameraY * screenY
            );
           
            Color pixelColor = castRay(camera.position, rayDirection);
            /* Color pixelColor = castRay(glm::vec3(0,0,20), glm::normalize(glm::vec3(screenX, screenY, -1.0f))); */

            point(glm::vec2(x, y), pixelColor);
        }
    }
}

int main(int argc, char* argv[]) {

     if (!init()) {
        return 1;
    }

    ImageLoader::loadImage("grass", "../assets/grass.png", 800.0f, 800.0f);
    ImageLoader::loadImage("grass_side", "../assets/grass_side.png", 800.0f, 800.0f);
    ImageLoader::loadImage("dirt", "../assets/dirt.png", 800.0f, 800.0f);
    ImageLoader::loadImage("gold", "../assets/gold_block.png", 800.0f, 800.0f);
    ImageLoader::loadImage("stone", "../assets/stone.png", 800.0f, 800.0f);
    ImageLoader::loadImage("cobblestone", "../assets/cobblestone.png", 800.0f, 800.0f);
    ImageLoader::loadImage("obsidian", "../assets/obsidian.png", 800.0f, 800.0f);
    ImageLoader::loadImage("rail", "../assets/rail.png", 800.0f, 800.0f);
    ImageLoader::loadImage("coal", "../assets/coal.png", 800.0f, 800.0f);
    ImageLoader::loadImage("glass", "../assets/glass.png", 800.0f, 800.0f);
    ImageLoader::loadImage("chest_front", "../assets/chest_front.png", 800.0f, 800.0f);
    ImageLoader::loadImage("chest_top", "../assets/chest_top.png", 800.0f, 800.0f);
    ImageLoader::loadImage("chest_side", "../assets/chest_side.png", 800.0f, 800.0f);
    ImageLoader::loadImage("pumpkin_front", "../assets/pumpkin_front.png", 800.0f, 800.0f);
    ImageLoader::loadImage("pumpkin_side", "../assets/pumpkin_side.png", 800.0f, 800.0f);
    ImageLoader::loadImage("pumpkin_top", "../assets/pumpkin_top.png", 800.0f, 800.0f);
    
    // Cargar Skybox
    ImageLoader::loadImage("skybox1", "../assets/skybox1.png", 1080.0f, 1080.0f);
    ImageLoader::loadImage("skybox2", "../assets/skybox2.png", 1080.0f, 1080.0f);
    ImageLoader::loadImage("skybox3", "../assets/skybox3.png", 1080.0f, 1080.0f);
    ImageLoader::loadImage("skybox4", "../assets/skybox4.png", 1080.0f, 1080.0f);
    ImageLoader::loadImage("skybox_ground", "../assets/skybox_ground.png", 1080.0f, 1080.0f);
    ImageLoader::loadImage("skybox_sky", "../assets/skybox_sky.png", 1080.0f, 1080.0f);

    bool running = true;
    SDL_Event event;

    setUp();

    while (running) {
        //startFPS();
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }

            if (event.type == SDL_KEYDOWN) {
                switch(event.key.keysym.sym) {
                    case SDLK_d:
                        camera.rotate(1.0f, 0.0f);
                        break;
                    case SDLK_a:
                        camera.rotate(-1.0f, 0.0f);
                        break;
                    case SDLK_w:
                        camera.zoom(1.0f);
                        break;
                    case SDLK_s:
                        camera.zoom(-1.0f);
                        break;
                    case SDLK_UP:
                        camera.moveY(1.0f);
                        break;
                    case SDLK_DOWN:
                        camera.moveY(-1.0f);
                        break;
                    case SDLK_RIGHT:
                        camera.moveX(1.0f);
                        break;
                    case SDLK_LEFT:
                        camera.moveX(-1.0f);
                        break;
                 }
            }


        }

        // Clear the screen
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        render();

        // Present the renderer
        SDL_RenderPresent(renderer);
        //endFPS(window);

    }
        // Cleanup
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();

    return 0;
}