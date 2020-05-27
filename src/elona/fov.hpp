#pragma once

#include <array>



namespace elona
{

struct Position;
struct Character;

constexpr int fov_max = 15; // in diameter

extern std::array<std::array<int, 2>, fov_max + 2> fovlist;

// Returns wheather the PC can see  the position or the character.
bool is_in_fov(const Position&);
bool is_in_fov(const Character& chara);
int fov_los(int = 0, int = 0, int = 0, int = 0);
int get_route(int = 0, int = 0, int = 0, int = 0);
void init_fovlist();

int breath_list(const Position& source_pos);
int route_info(int&, int&, int = 0);

} // namespace elona
