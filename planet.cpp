#include <planet.h>
#include <iostream>
    
constexpr float G = 6.67 * 0.0001;
constexpr float k = 0.8;
float friction_k;

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

void Planet::update_pos(GravityBuffer &buffer) {
    for (int q = 1; q < buffer.cell_sizes.size(); ++q) {
        LiteMath::int2 pos_floor = LiteMath::int2(LiteMath::floor(pos / buffer.cell_sizes[q]));

        int x1 = (pos_floor.x - 1 >= 0 ? buffer.buffer_levels[q][pos_floor.y * buffer.buffer_sizes[q].x + pos_floor.x - 1] : 0);
        int x2 = (pos_floor.x + 1 < buffer.buffer_sizes[q].x ? buffer.buffer_levels[q][pos_floor.y * buffer.buffer_sizes[q].x + pos_floor.x + 1] : 0);
        int y1 = (pos_floor.y - 1 >= 0 ? buffer.buffer_levels[q][(pos_floor.y - 1) * buffer.buffer_sizes[q].x + pos_floor.x] : 0);
        int y2 = (pos_floor.y + 1 < buffer.buffer_sizes[q].y ? buffer.buffer_levels[q][(pos_floor.y + 1) * buffer.buffer_sizes[q].x + pos_floor.x] : 0);
        vel += LiteMath::float2(x2 - x1, y2 - y1) * G / buffer.cell_sizes[q] / buffer.cell_sizes[q];
    }
    vel *= friction_k;
    LiteMath::float2 new_pos = pos + vel;
    if (new_pos.x >= 0 && new_pos.x < buffer.buffer_sizes[0].x && new_pos.y >= 0 && new_pos.y < buffer.buffer_sizes[0].y) {
        buffer.update(pos, new_pos, m);
        pos = new_pos;
    } else {
        if (new_pos.x < 0 || new_pos.x >= buffer.buffer_sizes[0].x) {
            vel.x *= -1 * k;
        }
        if (new_pos.y < 0 || new_pos.y >= buffer.buffer_sizes[0].y) {
            vel.y *= -1 * k;
        }
        new_pos = pos + vel;
        buffer.update(pos, new_pos, m);
        pos = new_pos;
    }
}

GravityBuffer::GravityBuffer(std::vector<Planet> &planets, std::vector <int> cell_sizes, LiteMath::int2 screen_size) {
    this->cell_sizes = cell_sizes;
    buffer_sizes.resize(cell_sizes.size());
    buffer_levels.resize(cell_sizes.size());
    for (int i = 0; i < cell_sizes.size(); ++i) {
        buffer_sizes[i] = LiteMath::int2(screen_size.x / cell_sizes[i], screen_size.y / cell_sizes[i]);
        buffer_levels[i].resize(buffer_sizes[i].x * buffer_sizes[i].y, 0);
    }
    int planets_count = planets.size();
    
    for (int i = 0; i < cell_sizes.size(); ++i) {
        std::cout << buffer_sizes[i].x << ' ' << buffer_sizes[i].y << ' ' << buffer_levels[i].size() << std::endl;
    }
    #pragma omp parallel for 
    for (int i = 0; i < planets_count; ++i) {
        LiteMath::int2 pos_floor = LiteMath::int2(LiteMath::floor(planets[i].pos));
        buffer_levels[0][pos_floor.y * screen_size.x + pos_floor.x] = planets[i].m;
    }
    #pragma omp parallel for collapse(4)
    for (int q = 1; q < cell_sizes.size(); ++q) {
        for (int x = 0; x < buffer_sizes[q].x; ++x) {
            for (int y = 0; y < buffer_sizes[q].y; ++y) {
                float sum = 0;
                int x0 = x * cell_sizes[q];
                int y0 = y * cell_sizes[q];
                for (int x1 = 0; x1 < cell_sizes[q]; ++x1) {
                    for (int y1 = 0; y1 < cell_sizes[q]; ++y1) {
                        sum += buffer_levels[0][(y0 + y1) * buffer_sizes[q - 1].x + x0 + x1];
                    }
                }
                buffer_levels[q][y * buffer_sizes[q].x + x] = sum;
            }
        }
    }
}

GravityBuffer::~GravityBuffer() {}

void GravityBuffer::update(LiteMath::float2 prev_pos, LiteMath::float2 new_pos, int m) {
    for (int q = 0; q < cell_sizes.size(); ++q) {
        LiteMath::int2 pos1 = LiteMath::int2(LiteMath::floor(prev_pos / cell_sizes[q]));
        LiteMath::int2 pos2 = LiteMath::int2(LiteMath::floor(new_pos / cell_sizes[q]));
        if (pos1.x >= 0 && pos1.x < buffer_sizes[q].x && pos1.y >= 0 && pos1.y < buffer_sizes[q].y)
            buffer_levels[q][pos1.y * buffer_sizes[q].x + pos1.x] -= m;
        if (pos2.x >= 0 && pos2.x < buffer_sizes[q].x && pos2.y >= 0 && pos2.y < buffer_sizes[q].y)
            buffer_levels[q][pos2.y * buffer_sizes[q].x + pos2.x] += m;
    }
    
}

