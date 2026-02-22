#include <planet.h>
#include <iostream>
    
constexpr float G = 6.67 * 500;
constexpr float k = 0.1;

Planet::Planet(LiteMath::float2 pos, float m=1, float r=1): pos(pos), m(m), r(r) {}
Planet::Planet(){
    pos = LiteMath::float2(0, 0);
    vel = LiteMath::float2(0, 0);
    m = 1;
    r = 0;
}
Planet::~Planet() {}

bool Planet::check_in_sight(int width, int height) {
    return (pos.x >= 0 && pos.x <= width - 1 && pos.y >= 0 && pos.y <= height - 1);
}

void Planet::update_pos(CountBuffer &buffer) {
    LiteMath::int2 pos_floor = LiteMath::int2(LiteMath::floor(pos / buffer.cell_size));
    vel += buffer.gravity_buffer[pos_floor.x][pos_floor.y];
    LiteMath::float2 new_pos = pos + vel;
    if (new_pos.x >= 0 && new_pos.x < buffer.width * buffer.cell_size && new_pos.y >= 0 && new_pos.y < buffer.height * buffer.cell_size) {
        buffer.update(pos, new_pos, m);
        pos = new_pos;
    } else {
        if (new_pos.x < 0 || new_pos.x >= buffer.width) {
            vel.x *= -1 * k;
        }
        if (new_pos.y < 0 || new_pos.y >= buffer.height) {
            vel.y *= -1 * k;
        }
        new_pos = pos + vel;
        buffer.update(pos, new_pos, m);
        pos = new_pos;
    }
    
}


void CountBuffer::build_gravity_buffer() {
    #pragma omp parallel for collapse(4)
    for (int x = 0; x < width; ++x) {
        for (int y = 0; y < height; ++y) {
            gravity_buffer[x][y] = LiteMath::float2(0.0f);
            for (int x1 = 0; x1 < width; ++x1) {
                for (int y1 = 0; y1 < height; ++y1) {
                    if (x1 == x && y1 == y) continue;
                    LiteMath::float2 source(x1 * cell_size, y1 * cell_size);
                    LiteMath::float2 cur_point(x * cell_size, y * cell_size);
                    LiteMath::float2 dir = source - cur_point;
                    float length = LiteMath::length(dir) * 1000;
                    gravity_buffer[x][y] += G * dir * buffer[x1][y1] / length / length / length;
                }
            }
        }
    }
}

CountBuffer::CountBuffer(std::vector<Planet> planets, int SCREEN_WIDTH, int SCREEN_HEIGHT, int cell_size1) {
    width = SCREEN_WIDTH / cell_size1;
    height = SCREEN_HEIGHT / cell_size1;
    buffer.resize(width, std::vector<int> (height, 0));
    gravity_buffer.resize(width, std::vector<LiteMath::float2> (height, LiteMath::float2(0.0f)));
    cell_size = cell_size1;
    int planets_count = planets.size();
    #pragma omp parallel for collapse(4)
    for (int x = 0; x < width; ++x) {
        for (int y = 0; y < height; ++y) {
            for (int i = 0; i < planets_count; ++i) {
                if ((x * cell_size <= planets[i].pos.x && planets[i].pos.x <= (x + 1) * cell_size) 
                    && (y * cell_size <= planets[i].pos.y && planets[i].pos.y <= (y + 1) * cell_size)) {
                    buffer[x][y] += planets[i].m;
                }
            }
        }
    }
    build_gravity_buffer();
}

CountBuffer::~CountBuffer() {}

void CountBuffer::update(LiteMath::float2 prev_pos, LiteMath::float2 new_pos, int m) {
    LiteMath::int2 pos1 = LiteMath::int2(LiteMath::floor(prev_pos / cell_size));
    LiteMath::int2 pos2 = LiteMath::int2(LiteMath::floor(new_pos / cell_size));
    if (pos1.x >= 0 && pos1.x < width && pos1.y >= 0 && pos1.y < height)
        buffer[pos1.x][pos1.y] -= m;
    if (pos2.x >= 0 && pos2.x < width && pos2.y >= 0 && pos2.y < height)
        buffer[pos2.x][pos2.y] += m;
}

