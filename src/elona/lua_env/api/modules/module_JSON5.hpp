#pragma once

#include "../common.hpp"



namespace elona
{
namespace lua
{

/**
 * @luadoc
 *
 * Parse/stringify JSON5 format.
 */
namespace LuaApiJSON5
{

std::pair<sol::object, sol::optional<std::string>> parse(
    const std::string& source,
    sol::this_state state);
std::string stringify(sol::object value, sol::table opts);

void bind(sol::table&);

} // namespace LuaApiJSON5

} // namespace lua
} // namespace elona
