#include <vector>
#include "utils/LiteMath.h"

class Planet;

class CountBuffer{
    public:
    int width, height;
    int cell_size;
    std::vector <std::vector <int>> buffer;
    std::vector <std::vector <LiteMath::float2>> gravity_buffer;
    CountBuffer(std::vector <Planet>, int, int, int);
    ~CountBuffer();

  
    void build_gravity_buffer();
    void update(LiteMath::float2 prev_pos, LiteMath::float2 new_pos, int m);
};

class Planet{
    public:
    LiteMath::float2 pos;
    LiteMath::float2 vel;
    float m, r;
    Planet(LiteMath::float2, float, float);
    Planet();
    ~Planet();
    bool check_in_sight(int, int);
    void update_pos(CountBuffer &buffer);
};

