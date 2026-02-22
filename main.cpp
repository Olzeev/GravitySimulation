#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION


#include "utils/public_image.h"

#include <cstdio>
#include <cstring>
#include <SDL_keycode.h>
#include <cstdint>
#include <iostream>
#include <fstream>
#include <SDL.h>
#include <chrono>
#include "utils/LiteMath.h"

#include <vector>
#include <cstdlib>
#include <planet.h>

#define PI 3.14159265359

using LiteMath::float2;
using LiteMath::float3;
using LiteMath::float4;
using LiteMath::int2;
using LiteMath::int3;
using LiteMath::int4;
using LiteMath::uint2;
using LiteMath::uint3;
using LiteMath::uint4;


int SCREEN_WIDTH, SCREEN_HEIGHT;
int max_m;
float gradient_k;
extern float friction_k;

uint32_t float3_to_RGBA8(float3 c)
{
  uint8_t r = (uint8_t)(std::clamp(c.x,0.0f,1.0f)*255.0f);
  uint8_t g = (uint8_t)(std::clamp(c.y,0.0f,1.0f)*255.0f);
  uint8_t b = (uint8_t)(std::clamp(c.z,0.0f,1.0f)*255.0f);
  return 0xFF000000 | (r<<16) | (g<<8) | b;
}

LiteMath::float3 gradient1(float t) {
  LiteMath::float3 res;
  res.x = -fabs(t - 0.25) * 6 + 1.5;
  res.y = -fabs(t - 0.5) * 6 + 2;
  res.z = -fabs(t - 0.75) * 6 + 1.5;
  //td::cout << res.x << ' ' << res.y << ' ' << res.z << std::endl;
  return res;
}

LiteMath::float3 gradient2(float t) {
  LiteMath::float3 res;
  res.x = t;
  res.z = t / 2;
  res.y = t / 2;

  //td::cout << res.x << ' ' << res.y << ' ' << res.z << std::endl;
  return res;
}

void render(std::vector <Planet> &planets, int planets_count, GravityBuffer gravity_buffer, uint32_t *out_image, int W, int H)
{
  #pragma omp parallel
  for (int x = 0; x < gravity_buffer.buffer_sizes[0].x; ++x) {
    for (int y = 0; y < gravity_buffer.buffer_sizes[0].y; ++y) {
      
      if (gravity_buffer.buffer_levels[0][y * gravity_buffer.buffer_sizes[0].x + x] != 0) {
        float sum = 0;
        for (int i = 0; i < 1/*gravity_buffer.cell_sizes.size()*/; ++i) {
          LiteMath::int2 floor_pos = LiteMath::int2(x / gravity_buffer.cell_sizes[i], y / gravity_buffer.cell_sizes[i]);
          sum += (float)gravity_buffer.buffer_levels[i][floor_pos.y * gravity_buffer.buffer_sizes[i].x + floor_pos.x] / gravity_buffer.cell_sizes[i] / gravity_buffer.cell_sizes[i];
        }
        
        sum /= gravity_buffer.cell_sizes.size();
        sum *= gradient_k;
        out_image[y * W + x] = float3_to_RGBA8(gradient2(sum));
      } else
        out_image[y * W + x] = float3_to_RGBA8(LiteMath::float3(0.0f));
    }
  }
}


// You must include the command line parameters for your main function to be recognized by SDL
int main(int argc, char **args)
{

  std::cin >> SCREEN_WIDTH >> SCREEN_HEIGHT;
  // Pixel buffer (RGBA format)
  std::vector<uint32_t> pixels(SCREEN_WIDTH * SCREEN_HEIGHT, 0xFFFFFFFF); // Initialize with white pixels

  srand(time(0));

  // Initialize SDL. SDL_Init will return -1 if it fails.
  if (SDL_Init(SDL_INIT_EVERYTHING) < 0)
  {
    std::cerr << "Error initializing SDL: " << SDL_GetError() << std::endl;
    return 1;
  }

  // Create our window
  SDL_Window *window = SDL_CreateWindow("SDF Viewer", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                        SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);

  // Make sure creating the window succeeded
  if (!window)
  {
    std::cerr << "Error creating window: " << SDL_GetError() << std::endl;
    return 1;
  }

  // Create a renderer
  SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
  if (!renderer)
  {
    std::cerr << "Renderer could not be created! SDL_Error: " << SDL_GetError() << std::endl;
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 1;
  }

  // Create a texture
  SDL_Texture *texture = SDL_CreateTexture(
      renderer,
      SDL_PIXELFORMAT_ARGB8888,    // 32-bit RGBA format
      SDL_TEXTUREACCESS_STREAMING, // Allows us to update the texture
      SCREEN_WIDTH,
      SCREEN_HEIGHT);

  if (!texture)
  {
    std::cerr << "Texture could not be created! SDL_Error: " << SDL_GetError() << std::endl;
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 1;
  }
  SDL_ShowCursor(SDL_DISABLE);
  SDL_SetRelativeMouseMode(SDL_TRUE);

  SDL_Event ev;
  bool running = true;


  int planet_count = 10000;
  std::cout << "Planet count:\n";
  std::cin >> planet_count;
  std::vector <Planet> planets(planet_count);
  std::cout << "Max m\n";
  std::cin >> max_m;
  LiteMath::float2 c1 = LiteMath::float2(200, 200);
  LiteMath::float2 c2 = LiteMath::float2(SCREEN_WIDTH - 200, SCREEN_HEIGHT - 200);
  LiteMath::float2 c3 = LiteMath::float2(200, SCREEN_HEIGHT - 200);
  LiteMath::float2 c4 = LiteMath::float2(SCREEN_WIDTH - 200, 200);
  for (int i = 0; i < planet_count; ++i) {
    
    float des = float(rand()) / RAND_MAX;
    float sigma = 10000;
    float r = float(rand()) / RAND_MAX * 100;
    float phi = 2 * LiteMath::M_PI * float(rand()) / RAND_MAX;
    if (des < 0.25) {
      planets[i].pos = c1 + r * LiteMath::float2(cos(phi), sin(phi));
    } else if (des < 0.5){
      planets[i].pos = c2 + r * LiteMath::float2(cos(phi), sin(phi));
    } else {
      planets[i].pos = c3 + r * LiteMath::float2(cos(phi), sin(phi));
    }
    

    //planets[i].pos = LiteMath::float2(float(rand()) / RAND_MAX * SCREEN_WIDTH, float(rand()) / RAND_MAX * SCREEN_HEIGHT);
    //planets[i].m = 1;
  }
  std::cout << "Gradient koef:\n";
  std::cin >> gradient_k;
  std::cout << "Friction koef:\n";
  std::cin >> friction_k;
  std::cout << "Buffer levels\n";
  int levels_count;
  std::cin >> levels_count;
  std::vector <int> cell_sizes(levels_count);
  for (int i = 0; i < levels_count; ++i) {
    std::cin >> cell_sizes[i];
  }
  GravityBuffer gravity_buffer(planets, cell_sizes, LiteMath::int2(SCREEN_WIDTH, SCREEN_HEIGHT));

  for (int i = 0; i < gravity_buffer.cell_sizes.size(); ++i) {
    std::cout << gravity_buffer.buffer_sizes[i].x << ' ' << gravity_buffer.buffer_sizes[i].y << '\n'; 
  }

  auto time = std::chrono::high_resolution_clock::now();
  auto prev_time = time;
  float time_from_start = 0;
  uint32_t frameNum = 0;

  const Uint8* keys = SDL_GetKeyboardState(NULL);

  // Main loop
  while (running)
  {
    //update camera or scene
    prev_time = time;
    time = std::chrono::high_resolution_clock::now();

    //get delta time in seconds
    float dt = std::chrono::duration<float, std::milli>(time - prev_time).count() / 1000.0f;
    time_from_start += dt;
    frameNum++;

    if (frameNum % 10 == 0)
      printf("Render time: %f ms\n", 1000.0f*dt);
    // Process keyboard input
    while (SDL_PollEvent(&ev) != 0)
    {
      // check event type
      switch (ev.type)
      {
      case SDL_QUIT:
        // shut down
        running = false;
        break;
      case SDL_KEYDOWN:
        // test keycode
        switch (ev.key.keysym.sym)
        {
          //ESC to exit 
          case SDLK_ESCAPE:
            running = false;
            break;
        }
        break;
      }
    }
    
    //count_buffer.build_gravity_buffer();
    
    for (int i = 0; i < planet_count; ++i) {
      planets[i].update_pos(gravity_buffer);
    }

    
    
    // Render the scene
    render(planets, planet_count, gravity_buffer,
      pixels.data(), SCREEN_WIDTH, SCREEN_HEIGHT);

    // Update the texture with the pixel buffer
    SDL_UpdateTexture(texture, nullptr, pixels.data(), SCREEN_WIDTH * sizeof(uint32_t));

    // Clear the renderer
    SDL_RenderClear(renderer);

    // Copy the texture to the renderer
    SDL_RenderCopy(renderer, texture, nullptr, nullptr);

    // Update the screen
    SDL_RenderPresent(renderer);
  }

  // Destroy the window. This will also destroy the surface
  SDL_DestroyWindow(window);

  // Quit SDL
  SDL_Quit();

  // End the program
  return 0;
}