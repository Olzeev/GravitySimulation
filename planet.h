#include <vector>
#include "utils/LiteMath.h"

class Planet;

class GravityBuffer{
    public:
    
    std::vector <int> cell_sizes;
    std::vector <LiteMath::int2> buffer_sizes;
    std::vector <std::vector <int>> buffer_levels;
    GravityBuffer(std::vector <Planet> &planets, std::vector<int> cell_sizes, LiteMath::int2 screen_size);
    ~GravityBuffer();

  
    //void build_gravity_buffer();
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
    void update_pos(GravityBuffer &buffer);
};

