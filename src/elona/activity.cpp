#include "activity.hpp"

#include "ability.hpp"
#include "animation.hpp"
#include "area.hpp"
#include "audio.hpp"
#include "building.hpp"
#include "calc.hpp"
#include "character.hpp"
#include "character_status.hpp"
#include "command.hpp"
#include "config.hpp"
#include "crafting.hpp"
#include "dmgheal.hpp"
#include "draw.hpp"
#include "enchantment.hpp"
#include "fish.hpp"
#include "food.hpp"
#include "fov.hpp"
#include "i18n.hpp"
#include "input.hpp"
#include "item.hpp"
#include "itemgen.hpp"
#include "map.hpp"
#include "map_cell.hpp"
#include "message.hpp"
#include "optional.hpp"
#include "random_event.hpp"
#include "save.hpp"
#include "status_ailment.hpp"
#include "text.hpp"
#include "ui.hpp"
#include "variables.hpp"
#include "world.hpp"



namespace elona
{

namespace
{

int digx = 0;
int digy = 0;
int performance_tips;



// Returns true if stop.
bool prompt_stop_activity(const Character& doer)
{
    txt(i18n::s.get(
        "core.activity.cancel.prompt",
        i18n::s.get_enum(
            "core.ui.action", static_cast<int>(doer.activity.type))));
    return static_cast<bool>(yes_no());
}



void search_material_spot()
{
    if (cell_data.at(cdata.player().position.x, cdata.player().position.y)
            .feats == 0)
    {
        return;
    }
    cell_featread(cdata.player().position.x, cdata.player().position.y);
    if (feat(1) < 24 || 28 < feat(1))
    {
        return;
    }
    atxspot = 11;
    int atxlv = game_data.current_dungeon_level;
    if (map_data.type == mdata_t::MapType::dungeon)
    {
        atxspot = 9;
    }
    if (map_data.type == mdata_t::MapType::dungeon_tower)
    {
        atxspot = 12;
    }
    if (map_data.type == mdata_t::MapType::dungeon_forest)
    {
        atxspot = 10;
    }
    if (map_data.type == mdata_t::MapType::dungeon_castle)
    {
        atxspot = 12;
    }
    if (map_data.type == mdata_t::MapType::world_map)
    {
        atxlv = cdata.player().level / 2 + rnd(10);
        if (atxlv > 30)
        {
            atxlv = 30 + rnd(rnd_capped(atxlv - 30) + 1);
        }
        if (4 <= game_data.stood_world_map_tile &&
            game_data.stood_world_map_tile < 9)
        {
            atxspot = 10;
        }
        if (264 <= game_data.stood_world_map_tile &&
            game_data.stood_world_map_tile < 363)
        {
            atxspot = 11;
        }
        if (9 <= game_data.stood_world_map_tile &&
            game_data.stood_world_map_tile < 13)
        {
            atxspot = 10;
        }
        if (13 <= game_data.stood_world_map_tile &&
            game_data.stood_world_map_tile < 17)
        {
            atxspot = 11;
        }
    }
    cell_featread(cdata.player().position.x, cdata.player().position.y);
    if (feat(1) == 27)
    {
        atxlv += sdata(161, 0) / 3;
        atxspot = 16;
    }
    if (feat(1) == 26)
    {
        atxspot = 13;
    }
    if (feat(1) == 25)
    {
        atxspot = 14;
    }
    if (feat(1) == 28)
    {
        atxspot = 15;
    }
    if (rnd(7) == 0)
    {
        for (int cnt = 0; cnt < 1; ++cnt)
        {
            i = 5;
            if (atxspot == 14)
            {
                if (sdata(163, 0) < rnd_capped(atxlv * 2 + 1) || rnd(10) == 0)
                {
                    txt(i18n::s.get("core.activity.material.digging.fails"));
                    break;
                }
                i = 1;
                chara_gain_skill_exp(cdata.player(), 163, 40);
            }
            if (atxspot == 13)
            {
                if (sdata(185, 0) < rnd_capped(atxlv * 2 + 1) || rnd(10) == 0)
                {
                    txt(i18n::s.get("core.activity.material.fishing.fails"));
                    break;
                }
                i = 2;
                chara_gain_skill_exp(cdata.player(), 185, 40);
            }
            if (atxspot == 15)
            {
                if (sdata(180, 0) < rnd_capped(atxlv * 2 + 1) || rnd(10) == 0)
                {
                    txt(i18n::s.get("core.activity.material.searching.fails"));
                    break;
                }
                i = 3;
                chara_gain_skill_exp(cdata.player(), 180, 30);
            }
            matgetmain(random_material(atxlv, 0), 1, i);
        }
    }
    if (rnd(50 + trait(159) * 20) == 0)
    {
        s = i18n::s.get("core.activity.material.searching.no_more");
        if (feat(1) == 26)
        {
            s = i18n::s.get("core.activity.material.fishing.no_more");
        }
        if (feat(1) == 25)
        {
            s = i18n::s.get("core.activity.material.digging.no_more");
        }
        if (feat(1) == 28)
        {
            s = i18n::s.get("core.activity.material.harvesting.no_more");
        }
        txt(s);
        cdata.player().activity.finish();
        cell_data.at(cdata.player().position.x, cdata.player().position.y)
            .feats = 0;
    }
}



int calc_performance_tips(const Character& performer, const Character& audience)
{
    // Quality factor
    const auto Q = performer.quality_of_performance;
    // Instrument factor
    const auto I = performer.activity.item->param1;

    const auto max = sdata(183, performer.index) * 100;

    const auto m = Q * Q * (100 + I / 5) / 100 / 1000 + rnd(10);
    auto ret = clamp(audience.gold * clamp(m, 1, 100) / 125, 0, max);

    if (audience.index < 16)
    {
        ret = rnd(clamp(ret, 1, 100)) + 1;
    }
    if (is_shopkeeper(audience.role))
    {
        ret /= 5;
    }
    return clamp(ret, 0, audience.gold);
}



void activity_perform_generate_item(
    const Character& performer,
    const Character& audience,
    const Item& instrument)
{
    const std::vector<int> fsetperform{
        18000,
        20000,
        32000,
        34000,
        52000,
        57000,
        57000,
        57000,
        60000,
        64000,
        64000,
        77000,
    };

    const auto x =
        clamp(performer.position.x - 1 + rnd(3), 0, map_data.width - 1);
    const auto y =
        clamp(performer.position.y - 1 + rnd(3), 0, map_data.height - 1);

    cell_check(x, y);
    if (cellaccess == 0)
        return;
    if (!fov_los(audience.position.x, audience.position.y, x, y))
        return;

    if (enchantment_find(instrument, 49))
    {
        flt(calcobjlv(performer.quality_of_performance / 8),
            calcfixlv((rnd(4) == 0) ? Quality::miracle : Quality::great));
    }
    else
    {
        flt(calcobjlv(performer.quality_of_performance / 10),
            calcfixlv(Quality::good));
    }

    flttypemajor = choice(fsetperform);
    int item_id = 0;
    if (game_data.executing_immediate_quest_type == 1009)
    {
        if (rnd(150) == 0)
        {
            item_id = 241;
        }
        if (rnd(150) == 0)
        {
            item_id = 622;
        }
        if (audience.level > 15)
        {
            if (rnd(1000) == 0)
            {
                item_id = 725;
            }
        }
        if (audience.level > 10)
        {
            if (rnd(800) == 0)
            {
                item_id = 726;
            }
        }
    }
    else
    {
        if (rnd(10) == 0)
        {
            item_id = 724;
        }
        if (rnd(250) == 0)
        {
            item_id = 55;
        }
    }

    if (const auto item = itemcreate_extra_inv(item_id, x, y, 1))
    {
        // NOTE: may cause Lua creation callbacks to run twice.
        item->modify_number(-1);
        cell_refresh(item->position.x, item->position.y);
        ThrowingObjectAnimation(
            item->position, performer.position, item->image, item->color)
            .play();
        item->modify_number(1);
        cell_refresh(item->position.x, item->position.y);
        ++performance_tips;
    }
}



std::pair<bool, int> activity_perform_proc_audience(
    Character& performer,
    Character& audience)
{
    const auto performer_skill = sdata(183, performer.index);
    const auto& instrument = *performer.activity.item;

    if (audience.state() != Character::State::alive)
    {
        return std::make_pair(false, 0);
    }
    if (game_data.date.hours() >= audience.time_interest_revive)
    {
        audience.interest = 100;
    }
    if (is_in_fov(performer))
    {
        if (audience.vision_flag != msync)
        {
            return std::make_pair(false, 0);
        }
    }
    else if (
        dist(
            performer.position.x,
            performer.position.y,
            audience.position.x,
            audience.position.y) > 3)
    {
        return std::make_pair(false, 0);
    }
    if (audience.interest <= 0)
    {
        return std::make_pair(false, 0);
    }
    if (audience.sleep > 0)
    {
        return std::make_pair(false, 0);
    }

    const auto x = audience.position.x;
    const auto y = audience.position.y;
    if (cell_data.at(x, y).chara_index_plus_one == 0)
    {
        return std::make_pair(false, 0); // TODO: unreachable?
    }
    if (audience.index == performer.index)
    {
        return std::make_pair(false, 0);
    }
    if (audience.relationship == -3)
    {
        if (audience.hate == 0)
        {
            if (is_in_fov(audience))
            {
                txt(i18n::s.get("core.activity.perform.gets_angry", audience));
            }
        }
        audience.hate = 30;
        return std::make_pair(false, 0);
    }
    if (performer.index == 0)
    {
        audience.interest -= rnd(15);
        audience.time_interest_revive = game_data.date.hours() + 12;
    }
    if (audience.interest <= 0)
    {
        if (is_in_fov(performer))
        {
            txt(i18n::s.get("core.activity.perform.dialog.disinterest"),
                Message::color{ColorIndex::cyan});
        }
        audience.interest = 0;
        return std::make_pair(false, 0);
    }
    if (performer_skill < audience.level)
    {
        if (rnd(3) == 0)
        {
            performer.quality_of_performance -= audience.level / 2;
            if (is_in_fov(performer))
            {
                txt(i18n::s.get("core.activity.perform.dialog.angry"),
                    Message::color{ColorIndex::cyan});
                txt(i18n::s.get("core.activity.perform.throws_rock", audience));
            }
            dmg = rnd_capped(audience.level + 1) + 1;
            if (audience.id == CharaId::loyter)
            {
                dmg = audience.level * 2 + rnd(100);
            }
            damage_hp(performer, dmg, -8);
            if (performer.state() == Character::State::empty)
            {
                return std::make_pair(true, 0);
            }
            return std::make_pair(false, 0);
        }
    }

    int gold = 0;
    if (rnd(3) == 0)
    {
        const auto tips = calc_performance_tips(performer, audience);
        audience.gold -= tips;
        earn_gold(performer, tips);
        gold = tips;
    }
    if (audience.level > performer_skill)
    {
        return std::make_pair(false, gold);
    }
    if (rnd_capped(performer_skill + 1) > rnd_capped(audience.level * 2 + 1))
    {
        if (game_data.executing_immediate_quest_type == 1009)
        {
            if (audience.index >= 57)
            {
                audience.impression += rnd(3);
                calcpartyscore();
            }
        }
        const auto p = rnd_capped(audience.level + 1) + 1;
        if (rnd(2) == 0)
        {
            performer.quality_of_performance += p;
        }
        else if (rnd(2) == 0)
        {
            performer.quality_of_performance -= p;
        }
    }
    if (enchantment_find(instrument, 60))
    {
        if (rnd(15) == 0)
        {
            status_ailment_damage(audience, StatusAilment::drunk, 500);
        }
    }
    if (rnd_capped(performer_skill + 1) > rnd_capped(audience.level * 5 + 1))
    {
        if (rnd(3) == 0)
        {
            if (is_in_fov(performer))
            {
                txt(i18n::s.get(
                        "core.activity.perform.dialog.interest",
                        audience,
                        performer),
                    Message::color{ColorIndex::cyan});
            }
            performer.quality_of_performance += audience.level + 5;
            if (performer.index == 0 && audience.index >= 16)
            {
                if (rnd_capped(performance_tips * 2 + 2) == 0)
                {
                    activity_perform_generate_item(
                        performer, audience, instrument);
                }
            }
        }
    }
    return std::make_pair(false, gold);
}



void activity_perform_start(Character& performer, Item& instrument)
{
    if (is_in_fov(performer))
    {
        txt(i18n::s.get("core.activity.perform.start", performer, instrument));
    }
    performer.activity.type = Activity::Type::perform;
    performer.activity.turn = 61;
    performer.activity.item = ItemRef::from_ref(instrument);
    performer.quality_of_performance = 40;
    performer.tip_gold = 0;
    if (performer.index == 0)
    {
        performance_tips = 0;
    }
}



void activity_perform_doing(Character& performer)
{
    if (performer.activity.turn % 10 == 0)
    {
        if (is_in_fov(performer))
        {
            if (rnd(10) == 0)
            {
                txt(i18n::s.get("core.activity.perform.sound.random"),
                    Message::color{ColorIndex::blue});
            }
            txt(i18n::s.get("core.activity.perform.sound.cha"),
                Message::color{ColorIndex::blue});
        }
    }
    if (performer.activity.turn % 20 == 0)
    {
        int gold = 0;
        make_sound(
            performer.position.x,
            performer.position.y,
            5,
            1,
            1,
            performer.index);
        for (auto&& audience : cdata.all())
        {
            const auto result =
                activity_perform_proc_audience(performer, audience);
            if (result.first)
                break;
            gold += result.second;
        }
        if (gold != 0)
        {
            performer.tip_gold += gold;
            if (is_in_fov(performer))
            {
                snd("core.getgold1");
            }
        }
    }
}



int calc_performance_quality_level(int quality)
{
    if (quality < 0)
        return 0;
    else if (quality < 20)
        return 1;
    else if (quality < 40)
        return 2;
    else if (quality == 40)
        return 3;
    else if (quality < 60)
        return 4;
    else if (quality < 80)
        return 5;
    else if (quality < 100)
        return 6;
    else if (quality < 120)
        return 7;
    else if (quality < 150)
        return 8;
    else
        return 9;
}



void activity_perform_end(Character& performer)
{
    if (performer.index == 0)
    {
        const auto quality_level =
            calc_performance_quality_level(performer.quality_of_performance);
        txt(i18n::s.get_enum("core.activity.perform.quality", quality_level));
    }

    if (performer.quality_of_performance > 40)
    {
        performer.quality_of_performance = performer.quality_of_performance *
            (100 + performer.activity.item->param1 / 5) / 100;
    }
    if (performer.tip_gold != 0)
    {
        if (is_in_fov(performer))
        {
            txt(i18n::s.get(
                "core.activity.perform.tip", performer, performer.tip_gold));
        }
    }

    performer.activity.finish();

    const auto experience =
        performer.quality_of_performance - sdata(183, performer.index) + 50;
    if (experience > 0)
    {
        chara_gain_skill_exp(performer, 183, experience, 0, 0);
    }
}



void activity_eating_start(Character& eater, Item& food)
{
    eater.activity.type = Activity::Type::eat;
    eater.activity.turn = 8;
    eater.activity.item = ItemRef::from_ref(food);
    if (is_in_fov(eater))
    {
        snd("core.eat1");
        if (food.own_state == 1 && eater.index < 16)
        {
            txt(i18n::s.get("core.activity.eat.start.in_secret", eater, food));
        }
        else
        {
            txt(i18n::s.get("core.activity.eat.start.normal", eater, food));
        }
        if (food.id == ItemId::corpse && food.subname == 344)
        {
            txt(i18n::s.get("core.activity.eat.start.mammoth"));
        }
    }
}



void activity_others_start(Character& doer, optional_ref<Item> activity_item)
{
    doer.activity.type = Activity::Type::others;
    doer.activity.item = ItemRef::from_opt(activity_item);

    switch (game_data.activity_about_to_start)
    {
    case 105:
        assert(activity_item);
        txt(i18n::s.get("core.activity.steal.start", *activity_item));
        doer.activity.turn = 2 + clamp(activity_item->weight / 500, 0, 50);
        break;
    case 100:
        if (map_data.type == mdata_t::MapType::player_owned ||
            map_is_town_or_guild())
        {
            txt(i18n::s.get("core.activity.sleep.start.other"));
            doer.activity.turn = 5;
        }
        else
        {
            txt(i18n::s.get("core.activity.sleep.start.global"));
            doer.activity.turn = 20;
        }
        break;
    case 101:
        assert(activity_item);
        txt(i18n::s.get("core.activity.construct.start", *activity_item));
        doer.activity.turn = 25;
        break;
    case 102:
        assert(activity_item);
        txt(i18n::s.get("core.activity.pull_hatch.start", *activity_item));
        doer.activity.turn = 10;
        break;
    case 103:
        assert(activity_item);
        txt(i18n::s.get("core.activity.dig", *activity_item));
        doer.activity.turn = 10 +
            clamp(activity_item->weight /
                      (1 + sdata(10, 0) * 10 + sdata(180, 0) * 40),
                  1,
                  100);
        break;
    case 104:
        assert(activity_item);
        if (game_data.weather == 0 || game_data.weather == 3)
        {
            if (game_data.time_when_textbook_becomes_available >
                game_data.date.hours())
            {
                txt(i18n::s.get("core.activity.study.start.bored"));
                doer.activity.finish();
                return;
            }
        }
        game_data.time_when_textbook_becomes_available =
            game_data.date.hours() + 48;
        if (activity_item->id == ItemId::textbook)
        {
            txt(i18n::s.get(
                "core.activity.study.start.studying",
                the_ability_db.get_text(activity_item->param1, "name")));
        }
        else
        {
            txt(i18n::s.get("core.activity.study.start.training"));
        }
        if (game_data.weather != 0 && game_data.weather != 3)
        {
            if (game_data.current_map == mdata_t::MapId::shelter_ ||
                map_can_use_bad_weather_in_study())
            {
                txt(i18n::s.get("core.activity.study.start.weather_is_bad"));
            }
        }
        doer.activity.turn = 50;
        break;
    default: break;
    }

    update_screen();
}



void activity_others_doing(Character& doer, optional_ref<Item> activity_item)
{
    switch (game_data.activity_about_to_start)
    {
    case 103:
        if (rnd(5) == 0)
        {
            chara_gain_skill_exp(cdata.player(), 180, 20, 4);
        }
        if (rnd(6) == 0)
        {
            if (rnd(55) > sdata.get(10, doer.index).original_level + 25)
            {
                chara_gain_skill_exp(doer, 10, 50);
            }
        }
        if (rnd(8) == 0)
        {
            if (rnd(55) > sdata.get(11, doer.index).original_level + 28)
            {
                chara_gain_skill_exp(doer, 11, 50);
            }
        }
        if (rnd(10) == 0)
        {
            if (rnd(55) > sdata.get(15, doer.index).original_level + 30)
            {
                chara_gain_skill_exp(doer, 15, 50);
            }
        }
        if (rnd(4) == 0)
        {
            txt(i18n::s.get("core.activity.harvest.sound"),
                Message::color{ColorIndex::cyan});
        }
        break;
    case 104: {
        assert(activity_item);
        int p = 25;
        if (game_data.weather != 0 && game_data.weather != 3)
        {
            if (game_data.current_map == mdata_t::MapId::shelter_)
            {
                p = 5;
            }
            if (map_can_use_bad_weather_in_study())
            {
                p = 5;
                game_data.date.minute += 30;
            }
        }
        if (activity_item->id == ItemId::textbook)
        {
            if (rnd(p) == 0)
            {
                chara_gain_skill_exp(doer, activity_item->param1, 25);
            }
        }
        else if (rnd(p) == 0)
        {
            chara_gain_skill_exp(doer, randattb(), 25);
        }
        break;
    }
    case 105:
        assert(activity_item);
        if (activity_item->id == ItemId::iron_maiden)
        {
            if (rnd(15) == 0)
            {
                doer.activity.finish();
                txt(i18n::s.get("core.activity.iron_maiden"));
                damage_hp(doer, 9999, -18);
                return;
            }
        }
        if (activity_item->id == ItemId::guillotine)
        {
            if (rnd(15) == 0)
            {
                doer.activity.finish();
                txt(i18n::s.get("core.activity.guillotine"));
                damage_hp(doer, 9999, -19);
                return;
            }
        }
        f = 0;
        f2 = 0;
        tg = inv_getowner(*activity_item);
        if (tg != -1)
        {
            if (cdata[tg].original_relationship == -3)
            {
                f2 = 1;
            }
        }
        i = sdata(300, 0) * 5 + sdata(12, 0) + 25;
        if (game_data.date.hour >= 19 || game_data.date.hour < 7)
        {
            i = i * 15 / 10;
        }
        if (activity_item->quality == Quality::great)
        {
            i = i * 8 / 10;
        }
        if (activity_item->quality >= Quality::miracle)
        {
            i = i * 5 / 10;
        }
        make_sound(cdata.player().position.x, cdata.player().position.y, 5, 8);
        for (int cnt = 16; cnt < ELONA_MAX_CHARACTERS; ++cnt)
        {
            if (cdata[cnt].state() != Character::State::alive)
            {
                continue;
            }
            if (cdata[cnt].sleep != 0)
            {
                continue;
            }
            if (dist(
                    cdata[cnt].position.x,
                    cdata[cnt].position.y,
                    cdata.player().position.x,
                    cdata.player().position.y) > 5)
            {
                continue;
            }
            if (f2 == 1)
            {
                if (cnt != tg)
                {
                    continue;
                }
            }
            p = rnd_capped(i + 1) *
                (80 + (is_in_fov(cdata[cnt]) == 0) * 50 +
                 dist(
                     cdata[cnt].position.x,
                     cdata[cnt].position.y,
                     cdata.player().position.x,
                     cdata.player().position.y) *
                     20) /
                100;
            if (cnt < 57)
            {
                p = p * 2 / 3;
            }
            if (rnd_capped(sdata(13, cnt) + 1) > p)
            {
                if (is_in_fov(cdata[cnt]))
                {
                    txt(i18n::s.get(
                        "core.activity.steal.notice.in_fov", cdata[cnt]));
                }
                else
                {
                    txt(i18n::s.get(
                        "core.activity.steal.notice.out_of_fov", cdata[cnt]));
                }
                if (cdata[cnt].role == Role::guard)
                {
                    txt(i18n::s.get("core.activity.steal.notice.dialog.guard"));
                    chara_modify_impression(cdata[cnt], -5);
                }
                else
                {
                    txt(i18n::s.get("core.activity.steal.notice.dialog.other"));
                    chara_modify_impression(cdata[cnt], -5);
                }
                cdata[cnt].emotion_icon = 520;
                f = 1;
            }
        }
        if (f)
        {
            txt(i18n::s.get("core.activity.steal.notice.you_are_found"));
            modify_karma(cdata.player(), -5);
            p = inv_getowner(*activity_item);
            if (tg != -1)
            {
                if (cdata[p].id != CharaId::ebon)
                {
                    if (cdata[tg].sleep == 0)
                    {
                        cdata[tg].relationship = -2;
                        hostileaction(0, tg);
                        chara_modify_impression(cdata[tg], -20);
                    }
                }
            }
            go_hostile();
        }
        if (tg != -1)
        {
            if (cdata[tg].state() != Character::State::alive)
            {
                if (f != 1)
                {
                    txt(i18n::s.get("core.activity.steal.target_is_dead"));
                    f = 1;
                }
            }
            if (cdata[tg].role == Role::user)
            {
                if (f != 1)
                {
                    txt(i18n::s.get("core.activity.steal.cannot_be_stolen"));
                    f = 1;
                }
            }
            if (dist(
                    doer.position.x,
                    doer.position.y,
                    cdata[tg].position.x,
                    cdata[tg].position.y) >= 3)
            {
                if (f != 1)
                {
                    txt(i18n::s.get("core.activity.steal.you_lose_the_target"));
                    f = 0;
                }
            }
        }
        if (activity_item->number() <= 0)
        {
            f = 1;
        }
        if (activity_item->is_precious())
        {
            if (f != 1)
            {
                txt(i18n::s.get("core.activity.steal.cannot_be_stolen"));
                f = 1;
            }
        }
        if (activity_item->weight >= sdata(10, 0) * 500)
        {
            if (f != 1)
            {
                txt(i18n::s.get("core.activity.steal.it_is_too_heavy"));
                f = 1;
            }
        }
        if (itemusingfind(*activity_item, true) != -1)
        {
            if (f != 1)
            {
                txt(i18n::s.get("core.action.someone_else_is_using"));
                f = 1;
            }
        }
        if (f)
        {
            txt(i18n::s.get("core.activity.steal.abort"));
            doer.activity.finish();
        }
        break;
    default: break;
    }
}



void activity_others_end_steal(Item& steal_target)
{
    tg = inv_getowner(steal_target);
    if ((tg != -1 && cdata[tg].state() != Character::State::alive) ||
        steal_target.number() <= 0)
    {
        txt(i18n::s.get("core.activity.steal.abort"));
        return;
    }
    in = 1;
    if (steal_target.id == ItemId::gold_piece)
    {
        in = steal_target.number();
    }
    const auto slot = inv_get_free_slot(0);
    if (!slot)
    {
        txt(i18n::s.get("core.action.pick_up.your_inventory_is_full"));
        return;
    }
    steal_target.is_quest_target() = false;
    if (steal_target.body_part != 0)
    {
        const auto item_owner = inv_getowner(steal_target);
        if (item_owner != -1)
        {
            p = steal_target.body_part;
            cdata[item_owner].equipment_slots[p - 100].unequip();
        }
        steal_target.body_part = 0;
        chara_refresh(cdata[item_owner]);
    }
    auto& stolen_item = *slot;
    item_copy(steal_target, stolen_item);
    stolen_item.set_number(in);
    stolen_item.is_stolen() = true;
    stolen_item.own_state = 0;
    steal_target.modify_number(-in);
    if (steal_target.number() <= 0)
    {
        cell_refresh(steal_target.position.x, steal_target.position.y);
    }
    txt(i18n::s.get("core.activity.steal.succeed", stolen_item));
    const auto item_weight = stolen_item.weight;
    if (stolen_item.id == ItemId::gold_piece)
    {
        snd("core.getgold1");
        earn_gold(cdata.player(), in);
        stolen_item.remove();
    }
    else
    {
        item_stack(0, stolen_item, true);
        sound_pick_up();
    }
    refresh_burden_state();
    chara_gain_skill_exp(
        cdata.player(), 300, clamp(item_weight / 25, 0, 450) + 50);
    if (cdata.player().karma >= -30)
    {
        if (rnd(3) == 0)
        {
            txt(i18n::s.get("core.activity.steal.guilt"));
            modify_karma(cdata.player(), -1);
        }
    }
}



void activity_others_end_sleep(optional_ref<Item> bed)
{
    txt(i18n::s.get("core.activity.sleep.finish"));
    sleep_start(bed);
}



void activity_others_end_build_shelter(Item& shelter)
{
    snd("core.build1");
    txt(i18n::s.get("core.activity.construct.finish", shelter));
    item_build_shelter(shelter);
}



void activity_others_end_enter_shelter(const Item& shelter)
{
    txt(i18n::s.get("core.activity.pull_hatch.finish", shelter));
    chatteleport = 1;
    game_data.previous_map2 = game_data.current_map;
    game_data.previous_dungeon_level = game_data.current_dungeon_level;
    game_data.previous_x = cdata.player().position.x;
    game_data.previous_y = cdata.player().position.y;
    game_data.destination_map = static_cast<int>(mdata_t::MapId::shelter_);
    game_data.destination_dungeon_level = shelter.count;
    levelexitby = 2;
    snd("core.exitmap1");
}



void activity_others_end_harvest(Item& crop)
{
    txt(i18n::s.get(
        "core.activity.harvest.finish", crop, cnvweight(crop.weight)));
    in = crop.number();
    pick_up_item(cdata.player().index, crop, none);
}



void activity_others_end_study(const Item& item)
{
    if (item.id == ItemId::textbook)
    {
        txt(i18n::s.get(
            "core.activity.study.finish.studying",
            the_ability_db.get_text(item.param1, "name")));
    }
    else
    {
        txt(i18n::s.get("core.activity.study.finish.training"));
    }
}



void activity_others_end(Character& doer, optional_ref<Item> activity_item)
{
    switch (game_data.activity_about_to_start)
    {
    case 105:
        assert(activity_item);
        activity_others_end_steal(*activity_item);
        break;
    case 100: activity_others_end_sleep(activity_item); break;
    case 101:
        assert(activity_item);
        activity_others_end_build_shelter(*activity_item);
        break;
    case 102:
        assert(activity_item);
        activity_others_end_enter_shelter(*activity_item);
        break;
    case 103:
        assert(activity_item);
        activity_others_end_harvest(*activity_item);
        break;
    case 104:
        assert(activity_item);
        activity_others_end_study(*activity_item);
        break;
    default: break;
    }
    doer.activity.finish();
}

} // namespace



void rowact_check(Character& chara)
{
    if (chara.activity)
    {
        if (chara.activity.type != Activity::Type::travel)
        {
            chara.stops_activity_if_damaged = 1;
        }
    }
}



void rowact_item(const Item& item)
{
    for (auto&& chara : cdata.all())
    {
        if (chara.state() != Character::State::alive)
            continue;
        if (chara.activity.turn <= 0)
            continue;

        if (chara.activity.type == Activity::Type::eat ||
            chara.activity.type == Activity::Type::read)
        {
            if (chara.activity.item == item)
            {
                chara.activity.finish();
                txt(i18n::s.get("core.activity.cancel.item", chara));
            }
        }
    }
}



void activity_handle_damage(Character& chara)
{
    bool stop = false;
    if (chara.index == 0)
    {
        if (chara.activity.type != Activity::Type::eat &&
            chara.activity.type != Activity::Type::read &&
            chara.activity.type != Activity::Type::travel)
        {
            stop = true;
        }
        else
        {
            screenupdate = -1;
            update_screen();
            stop = prompt_stop_activity(chara);
        }
    }
    if (stop)
    {
        if (is_in_fov(chara))
        {
            txt(i18n::s.get(
                "core.activity.cancel.normal",
                chara,
                i18n::s.get_enum(
                    u8"core.ui.action",
                    static_cast<int>(chara.activity.type))));
        }
        chara.activity.finish();
    }
    screenupdate = -1;
    update_screen();
    chara.stops_activity_if_damaged = 0;
}



optional<TurnResult> activity_proc(Character& chara)
{
    --chara.activity.turn;

    const auto auto_turn = [&](int delay) {
        if (chara.index == 0)
        {
            elona::auto_turn(delay);
        }
    };

    switch (chara.activity.type)
    {
    case Activity::Type::fish:
        auto_turn(g_config.animation_wait() * 2);
        spot_fishing(chara, chara.activity.item.as_opt());
        break;
    case Activity::Type::dig_wall:
        auto_turn(g_config.animation_wait() * 0.75);
        spot_mining_or_wall(chara);
        break;
    case Activity::Type::search_material:
        auto_turn(g_config.animation_wait() * 0.75);
        spot_material(chara);
        break;
    case Activity::Type::dig_ground:
        auto_turn(g_config.animation_wait() * 0.75);
        spot_digging(chara);
        break;
    case Activity::Type::sleep:
        auto_turn(g_config.animation_wait() / 4);
        do_rest(chara);
        break;
    case Activity::Type::eat:
        assert(chara.activity.item);
        auto_turn(g_config.animation_wait() * 5);
        return do_eat_command(chara, *chara.activity.item);
    case Activity::Type::read:
        assert(chara.activity.item);
        auto_turn(g_config.animation_wait() * 1.25);
        return do_read_command(chara, *chara.activity.item);
    case Activity::Type::sex:
        auto_turn(g_config.animation_wait() * 2.5);
        activity_sex(chara, none);
        break;
    case Activity::Type::others:
        switch (game_data.activity_about_to_start)
        {
        case 103: auto_turn(g_config.animation_wait() * 2); break;
        case 104: auto_turn(g_config.animation_wait() * 2); break;
        case 105: auto_turn(g_config.animation_wait() * 2.5); break;
        default: auto_turn(g_config.animation_wait()); break;
        }
        activity_others(chara, chara.activity.item.as_opt());
        break;
    case Activity::Type::blend:
        auto_turn(g_config.animation_wait());
        activity_blending();
        break;
    case Activity::Type::perform:
        assert(chara.activity.item);
        auto_turn(g_config.animation_wait() * 2);
        activity_perform(chara, *chara.activity.item);
        break;
    case Activity::Type::travel:
        map_global_proc_travel_events(chara);
        return proc_movement_event(chara);
    default: break;
    }

    if (chara.activity.turn > 0)
    {
        return TurnResult::turn_end;
    }
    chara.activity.finish();
    if (chara.index == 0)
    {
        if (chatteleport == 1)
        {
            chatteleport = 0;
            return TurnResult::exit_map;
        }
    }

    return none;
}



void activity_perform(Character& performer, Item& instrument)
{
    if (!performer.activity)
    {
        activity_perform_start(performer, instrument);
    }
    else if (performer.activity.turn > 0)
    {
        activity_perform_doing(performer);
    }
    else
    {
        activity_perform_end(performer);
    }
}



void activity_sex(Character& chara_a, optional_ref<Character> chara_b)
{
    int sexhost = 0;
    if (!chara_a.activity)
    {
        assert(chara_b);
        chara_a.activity.type = Activity::Type::sex;
        chara_a.activity.turn = 25 + rnd(10);
        chara_a.activity_target = chara_b->index;
        chara_b->activity.type = Activity::Type::sex;
        chara_b->activity.turn = chara_a.activity.turn * 2;
        chara_b->activity_target = chara_a.index + 10000;
        if (is_in_fov(chara_a))
        {
            txt(i18n::s.get("core.activity.sex.take_clothes_off", chara_a));
        }
        return;
    }

    sexhost = 1;
    auto target_index = chara_a.activity_target;
    if (target_index >= 10000)
    {
        target_index -= 10000;
        sexhost = 0;
    }
    if (cdata[target_index].state() != Character::State::alive ||
        cdata[target_index].activity.type != Activity::Type::sex)
    {
        if (is_in_fov(chara_a))
        {
            txt(i18n::s.get(
                "core.activity.sex.spare_life",
                i18n::s.get_enum("core.ui.sex2", cdata[target_index].sex),
                cdata[target_index]));
        }
        chara_a.activity.finish();
        cdata[target_index].activity.finish();
        return;
    }
    if (chara_a.index == 0)
    {
        if (!action_sp(cdata.player(), 1 + rnd(2)))
        {
            txt(i18n::s.get("core.magic.common.too_exhausted"));
            chara_a.activity.finish();
            cdata[target_index].activity.finish();
            return;
        }
    }
    chara_a.emotion_icon = 317;
    if (chara_a.activity.turn > 0)
    {
        if (sexhost == 0)
        {
            if (chara_a.activity.turn % 5 == 0)
            {
                if (is_in_fov(chara_a))
                {
                    txt(i18n::s.get("core.activity.sex.dialog"),
                        Message::color{ColorIndex::cyan});
                }
            }
        }
        return;
    }
    if (sexhost == 0)
    {
        chara_a.activity.finish();
        return;
    }
    for (int cnt = 0; cnt < 2; ++cnt)
    {
        int c{};
        if (cnt == 0)
        {
            c = chara_a.index;
        }
        else
        {
            c = target_index;
        }
        chara_a.drunk = 0;
        if (cnt == 1)
        {
            if (rnd(3) == 0)
            {
                status_ailment_damage(cdata[c], StatusAilment::insane, 500);
            }
            if (rnd(5) == 0)
            {
                status_ailment_damage(cdata[c], StatusAilment::paralyzed, 500);
            }
            status_ailment_damage(cdata[c], StatusAilment::insane, 300);
            heal_insanity(cdata[c], 10);
            chara_gain_skill_exp(cdata[c], 11, 250 + (c >= 57) * 1000);
            chara_gain_skill_exp(cdata[c], 15, 250 + (c >= 57) * 1000);
        }
        if (rnd(15) == 0)
        {
            status_ailment_damage(cdata[c], StatusAilment::sick, 200);
        }
        chara_gain_skill_exp(cdata[c], 17, 250 + (c >= 57) * 1000);
    }
    int sexvalue = sdata(17, chara_a.index) * (50 + rnd(50)) + 100;

    std::string dialog_head;
    std::string dialog_tail;
    std::string dialog_after;

    if (is_in_fov(chara_a))
    {
        dialog_head =
            i18n::s.get("core.activity.sex.after_dialog", cdata[target_index]);
        Message::instance().txtef(ColorIndex::yellow_green);
    }
    if (target_index != 0)
    {
        if (cdata[target_index].gold >= sexvalue)
        {
            if (is_in_fov(chara_a))
            {
                dialog_tail =
                    i18n::s.get("core.activity.sex.take", cdata[target_index]);
            }
        }
        else
        {
            if (is_in_fov(chara_a))
            {
                dialog_tail = i18n::s.get(
                    "core.activity.sex.take_all_i_have", cdata[target_index]);
                if (rnd(3) == 0)
                {
                    if (chara_a.index != 0)
                    {
                        dialog_after = i18n::s.get(
                            "core.activity.sex.gets_furious", chara_a);
                        chara_a.enemy_id = target_index;
                        chara_a.hate = 20;
                    }
                }
            }
            if (cdata[target_index].gold <= 0)
            {
                cdata[target_index].gold = 1;
            }
            sexvalue = cdata[target_index].gold;
        }
        cdata[target_index].gold -= sexvalue;
        if (chara_a.index == 0)
        {
            chara_modify_impression(cdata[target_index], 5);
            flt();
            itemcreate_extra_inv(54, chara_a.position, sexvalue);
            dialog_after +=
                i18n::s.get("core.common.something_is_put_on_the_ground");
            modify_karma(cdata.player(), -1);
        }
        else
        {
            earn_gold(chara_a, sexvalue);
        }
    }
    if (!dialog_head.empty() || !dialog_tail.empty() || !dialog_after.empty())
    {
        txt(i18n::s.get("core.activity.sex.format", dialog_head, dialog_tail) +
            dialog_after);
    }
    chara_a.activity.finish();
    cdata[target_index].activity.finish();
}



void activity_eating(Character& eater, Item& food)
{
    if (!eater.activity)
    {
        activity_eating_start(eater, food);
    }
    else if (eater.activity.turn > 0)
    {
        // Do nothing.
    }
    else
    {
        if (is_in_fov(eater))
        {
            txt(i18n::s.get("core.activity.eat.finish", eater, food));
        }
        activity_eating_finish(eater, food);
        eater.activity.finish();
    }
}



void activity_eating_finish(Character& eater, Item& food)
{
    apply_general_eating_effect(eater, food);

    if (eater.index == 0)
    {
        item_identify(food, IdentifyState::partly);
    }
    if (chara_unequip(food))
    {
        chara_refresh(eater);
    }

    food.modify_number(-1);

    if (eater.index == 0)
    {
        show_eating_message(eater);
    }
    else
    {
        if (food == eater.ai_item)
        {
            eater.ai_item = ItemRef::null();
        }
        if (eater.was_passed_item_by_you_just_now())
        {
            if (food.material == 35 && food.param3 < 0)
            {
                txt(i18n::s.get("core.food.passed_rotten"),
                    Message::color{ColorIndex::cyan});
                damage_hp(eater, 999, -12);
                if (eater.state() != Character::State::alive)
                {
                    if (eater.relationship > 0)
                    {
                        modify_karma(cdata.player(), -5);
                    }
                    else
                    {
                        modify_karma(cdata.player(), -1);
                    }
                }
                chara_modify_impression(eater, -25);
                return;
            }
        }
    }

    chara_anorexia(eater);

    if ((food.id == ItemId::kagami_mochi && rnd(3)) ||
        (food.id == ItemId::mochi && rnd(10) == 0))
    {
        if (is_in_fov(eater))
        {
            txt(i18n::s.get("core.food.mochi.chokes", eater),
                Message::color{ColorIndex::purple});
            txt(i18n::s.get("core.food.mochi.dialog"));
        }
        ++eater.choked;
    }
}



void activity_others(Character& doer, optional_ref<Item> activity_item)
{
    if (doer.index != 0)
    {
        doer.activity.finish();
        return;
    }

    if (!doer.activity)
    {
        activity_others_start(doer, activity_item);
        return;
    }
    else
    {
        if (doer.activity.turn > 0)
        {
            activity_others_doing(doer, activity_item);
        }
        else
        {
            activity_others_end(doer, activity_item);
        }
    }
}



void spot_fishing(Character& fisher, optional_ref<Item> rod)
{
    static int fishstat;

    if (!fisher.activity)
    {
        txt(i18n::s.get("core.activity.fishing.start"));
        snd("core.fish_cast");
        if (rowactre == 0)
        {
            fisher.activity.item = ItemRef::from_ref(*rod);
        }
        fisher.activity.type = Activity::Type::fish;
        fisher.activity.turn = 100;
        racount = 0;
        fishstat = 0;
        gsel(9);
        picload(filesystem::dirs::graphic() / u8"fishing.bmp", 0, 0, true);
        gsel(0);
        return;
    }
    if (rowactre != 0)
    {
        search_material_spot();
        return;
    }
    if (fisher.activity.turn > 0)
    {
        if (rnd(5) == 0)
        {
            fishstat = 1;
            fish = fish_select_at_random(fisher.activity.item->param4);
        }
        if (fishstat == 1)
        {
            if (rnd(5) == 0)
            {
                if (g_config.animation_wait() != 0)
                {
                    for (int cnt = 0, cnt_end = (4 + rnd(4)); cnt < cnt_end;
                         ++cnt)
                    {
                        fishanime(0) = 1;
                        fishanime(1) = 3 + rnd(3);
                        addefmap(fishx, fishy, 4, 2);
                        ++scrturn;
                        update_screen();
                        redraw();
                        await(g_config.animation_wait() * 2);
                    }
                }
                if (rnd(3) == 0)
                {
                    fishstat = 2;
                }
                if (rnd(6) == 0)
                {
                    fishstat = 0;
                }
                fishanime = 0;
            }
        }
        if (fishstat == 2)
        {
            fishanime = 2;
            snd("core.water2");
            cdata.player().emotion_icon = 220;
            if (g_config.animation_wait() != 0)
            {
                for (int cnt = 0, cnt_end = (8 + rnd(10)); cnt < cnt_end; ++cnt)
                {
                    ++scrturn;
                    update_screen();
                    redraw();
                    await(g_config.animation_wait() * 2);
                }
            }
            if (rnd(10))
            {
                fishstat = 3;
            }
            else
            {
                fishstat = 0;
            }
            fishanime = 0;
        }
        if (fishstat == 3)
        {
            fishanime = 3;
            if (g_config.animation_wait() != 0)
            {
                for (int cnt = 0, cnt_end = (28 + rnd(15)); cnt < cnt_end;
                     ++cnt)
                {
                    if (cnt % 7 == 0)
                    {
                        snd("core.fish_fight");
                    }
                    fishanime(1) = cnt;
                    ++scrturn;
                    update_screen();
                    addefmap(fishx, fishy, 5, 2);
                    redraw();
                    await(g_config.animation_wait() * 2);
                }
            }
            if (the_fish_db[fish]->difficulty >= rnd_capped(sdata(185, 0) + 1))
            {
                fishstat = 0;
            }
            else
            {
                fishstat = 4;
            }
            fishanime = 0;
        }
        if (fishstat == 4)
        {
            fishanime = 4;
            snd("core.fish_get");
            if (g_config.animation_wait() != 0)
            {
                for (int cnt = 0; cnt < 21; ++cnt)
                {
                    fishanime(1) = cnt;
                    if (cnt == 1)
                    {
                        addefmap(fishx, fishy, 1, 3);
                    }
                    ++scrturn;
                    update_screen();
                    redraw();
                    await(g_config.animation_wait() * 2);
                }
            }
            sound_pick_up();
            fishanime = 0;
            fisher.activity.finish();
            fish_get(fish);
            chara_gain_exp_fishing(cdata.player());
            cdata.player().emotion_icon = 306;
        }
        if (rnd(10) == 0)
        {
            damage_sp(fisher, 1);
        }
        return;
    }
    txt(i18n::s.get("core.activity.fishing.fail"));
    fisher.activity.finish();
}



void spot_material(Character& chara)
{
    if (!chara.activity)
    {
        chara.activity.type = Activity::Type::search_material;
        chara.activity.turn = 40;
        txt(i18n::s.get("core.activity.material.start"));
        racount = 0;
        return;
    }
    if (rowactre != 0)
    {
        search_material_spot();
        return;
    }
    chara.activity.finish();
}



void spot_digging(Character& chara)
{
    if (!chara.activity)
    {
        chara.activity.type = Activity::Type::dig_ground;
        chara.activity.turn = 20;
        if (rowactre == 0)
        {
            txt(i18n::s.get("core.activity.dig_spot.start.global"));
        }
        else
        {
            txt(i18n::s.get("core.activity.dig_spot.start.other"));
        }
        racount = 0;
        return;
    }
    if (rowactre != 0)
    {
        search_material_spot();
        return;
    }
    if (chara.activity.turn > 0)
    {
        if (chara.turn % 5 == 0)
        {
            txt(i18n::s.get("core.activity.dig_spot.sound"),
                Message::color{ColorIndex::blue});
        }
        return;
    }
    txt(i18n::s.get("core.activity.dig_spot.finish"));
    if (map_data.type == mdata_t::MapType::world_map)
    {
        for (auto&& item : inv.pc())
        {
            if (item.number() == 0)
            {
                continue;
            }
            if (item.id == ItemId::treasure_map && item.param1 != 0 &&
                item.param1 == cdata.player().position.x &&
                item.param2 == cdata.player().position.y)
            {
                snd("core.chest1");
                txt(i18n::s.get("core.activity.dig_spot.something_is_there"),
                    Message::color{ColorIndex::orange});
                msg_halt();
                snd("core.ding2");
                flt();
                itemcreate_extra_inv(622, cdata.player().position, 2 + rnd(3));
                flt();
                itemcreate_extra_inv(55, cdata.player().position, 1 + rnd(3));
                flt();
                itemcreate_extra_inv(
                    54, cdata.player().position, rnd(10000) + 2000);
                for (int i = 0; i < 4; ++i)
                {
                    flt(calcobjlv(cdata.player().level + 10),
                        calcfixlv(Quality::good));
                    if (i == 0)
                    {
                        fixlv = Quality::godly;
                    }
                    flttypemajor = choice(fsetchest);
                    itemcreate_extra_inv(0, cdata.player().position, 0);
                }
                txt(i18n::s.get("core.common.something_is_put_on_the_ground"));
                save_set_autosave();
                item.modify_number(-1);
                break;
            }
        }
    }
    spillfrag(refx, refy, 1);
    chara.activity.finish();
}



void spot_mining_or_wall(Character& chara)
{
    static int countdig{};

    if (!chara.activity)
    {
        chara.activity.type = Activity::Type::dig_wall;
        chara.activity.turn = 40;
        if (rowactre == 0)
        {
            txt(i18n::s.get("core.activity.dig_mining.start.wall"));
        }
        else
        {
            txt(i18n::s.get("core.activity.dig_mining.start.spot"));
        }
        if (chip_data.for_cell(refx, refy).kind == 6)
        {
            txt(i18n::s.get("core.activity.dig_mining.start.hard"));
        }
        countdig = 0;
        racount = 0;
        return;
    }
    if (rowactre != 0)
    {
        search_material_spot();
        return;
    }
    if (chara.activity.turn > 0)
    {
        if (rnd(5) == 0)
        {
            damage_sp(chara, 1);
        }
        ++countdig;
        f = 0;
        if (chip_data.for_cell(refx, refy).kind == 6)
        {
            if (rnd(12000) <
                sdata(10, chara.index) + sdata(163, chara.index) * 10)
            {
                f = 1;
            }
            p = 30 - sdata(163, chara.index) / 2;
            if (p > 0)
            {
                if (countdig <= p)
                {
                    f = 0;
                }
            }
        }
        else
        {
            if (rnd(1500) <
                sdata(10, chara.index) + sdata(163, chara.index) * 10)
            {
                f = 1;
            }
            p = 20 - sdata(163, chara.index) / 2;
            if (p > 0)
            {
                if (countdig <= p)
                {
                    f = 0;
                }
            }
        }
        if (f == 1 ||
            (game_data.quest_flags.tutorial == 2 &&
             game_data.current_map == mdata_t::MapId::your_home))
        {
            rtval = 0;
            if (rnd(5) == 0)
            {
                rtval = 54;
            }
            if (rnd(8) == 0)
            {
                rtval = -1;
            }
            if (cell_data.at(refx, refy).feats != 0)
            {
                cell_featread(refx, refy);
                if (feat(1) == 22)
                {
                    discover_hidden_path();
                }
            }
            cell_data.at(refx, refy).chip_id_actual = tile_tunnel;
            spillfrag(refx, refy, 2);
            snd("core.crush1");
            BreakingAnimation({refx, refy}).play();
            txt(i18n::s.get("core.activity.dig_mining.finish.wall"));
            if (game_data.quest_flags.tutorial == 2 &&
                game_data.current_map == mdata_t::MapId::your_home)
            {
                flt();
                if (const auto item = itemcreate_extra_inv(208, digx, digy, 0))
                {
                    item->curse_state = CurseState::cursed;
                }
                txt(i18n::s.get("core.activity.dig_mining.finish.find"));
                game_data.quest_flags.tutorial = 3;
            }
            else if (
                rtval != 0 && game_data.current_map != mdata_t::MapId::shelter_)
            {
                if (rtval > 0)
                {
                    flt();
                    itemcreate_extra_inv(rtval, digx, digy, 0);
                }
                else if (rtval == -1)
                {
                    flt(calcobjlv(game_data.current_dungeon_level),
                        calcfixlv(Quality::good));
                    flttypemajor = 77000;
                    itemcreate_extra_inv(0, digx, digy, 0);
                }
                txt(i18n::s.get("core.activity.dig_mining.finish.find"));
            }
            chara_gain_exp_digging(cdata.player());
            chara.activity.finish();
        }
        else if (chara.turn % 5 == 0)
        {
            txt(i18n::s.get("core.activity.dig_spot.sound"),
                Message::color{ColorIndex::blue});
        }
        return;
    }
    txt(i18n::s.get("core.activity.dig_mining.fail"));
    chara.activity.finish();
}



TurnResult do_dig_after_sp_check(Character& chara)
{
    if (chara.sp < 0)
    {
        txt(i18n::s.get("core.action.dig.too_exhausted"));
        update_screen();
        return TurnResult::pc_turn_user_error;
    }
    rowactre = 0;
    digx = tlocx;
    digy = tlocy;
    spot_mining_or_wall(chara);
    return TurnResult::turn_end;
}



void matgetmain(int material_id, int amount, int spot_type)
{
    mat(material_id) += amount;
    snd("core.alert1");

    std::string verb;
    switch (spot_type)
    {
    case 0: verb = i18n::s.get("core.activity.material.get_verb.get"); break;
    case 1: verb = i18n::s.get("core.activity.material.get_verb.dig_up"); break;
    case 2:
        verb = i18n::s.get("core.activity.material.get_verb.fish_up");
        break;
    case 3:
        verb = i18n::s.get("core.activity.material.get_verb.harvest");
        break;
    case 5: verb = i18n::s.get("core.activity.material.get_verb.find"); break;
    default: verb = i18n::s.get("core.activity.material.get_verb.get"); break;
    }

    txt(i18n::s.get(
            "core.activity.material.get", verb, amount, matname(material_id)) +
            u8"("s + mat(material_id) + u8") "s,
        Message::color{ColorIndex::blue});
}



void matdelmain(int material_id, int amount)
{
    mat(material_id) -= amount;
    txt(i18n::s.get(
        "core.activity.material.lose", matname(material_id), amount));
    txt(i18n::s.get("core.activity.material.lose_total", mat(material_id)),
        Message::color{ColorIndex::blue});
}



void sleep_start(optional_ref<Item> bed)
{
    int timeslept = 0;
    if (game_data.current_map == mdata_t::MapId::quest)
    {
        txt(i18n::s.get("core.activity.sleep.but_you_cannot"));
        game_data.character_and_status_for_gene = 0;
        return;
    }
    if (game_data.catches_god_signal)
    {
        txtgod(cdata.player().god_id, 10);
    }
    load_sleep_background();
    play_music("core.mcCoda", false);
    msg_halt();
    for (int cnt = 0; cnt < 20; ++cnt)
    {
        gmode(2, cnt * 10);
        draw_sleep_background_frame();
        await(g_config.animation_wait() * 10);
    }
    gmode(2);
    for (int cnt = 0; cnt < ELONA_MAX_PARTY_CHARACTERS; ++cnt)
    {
        cdata[cnt].wet = 0;
        cdata[cnt].poisoned = 0;
        cdata[cnt].sleep = 0;
        cdata[cnt].confused = 0;
        cdata[cnt].blind = 0;
        cdata[cnt].paralyzed = 0;
        cdata[cnt].dimmed = 0;
        cdata[cnt].drunk = 0;
        cdata[cnt].bleeding = 0;
        game_data.continuous_active_hours = 0;
        cdata[cnt].hp = cdata[cnt].max_hp;
        cdata[cnt].mp = cdata[cnt].max_mp;
        cdata[cnt].sp = cdata[cnt].max_sp;
        status_ailment_heal(cdata[cnt], StatusAilment::sick, 7 + rnd(7));
        if (cdata[cnt].has_anorexia())
        {
            cdata[cnt].anorexia_count -= rnd(6);
        }
        else
        {
            cdata[cnt].anorexia_count -= rnd(3);
        }
        if (cdata[cnt].anorexia_count < 0)
        {
            cure_anorexia(cdata[cnt]);
            cdata[cnt].anorexia_count = 0;
        }
        heal_insanity(cdata[cnt], 10);
        if (cdata[cnt].has_lay_hand())
        {
            cdata[cnt].is_lay_hand_available() = true;
        }
    }
    mode = 9;
    timeslept = 7 + rnd(5);
    for (int cnt = 0, cnt_end = (timeslept); cnt < cnt_end; ++cnt)
    {
        ++game_data.date.hour;
        weather_changes();
        if (mode != 9)
        {
            load_sleep_background();
            mode = 9;
        }
        game_data.continuous_active_hours = 0;
        game_data.date.minute = 0;
        draw_sleep_background_frame();
        await(g_config.animation_wait() * 25);
    }
    if (game_data.character_and_status_for_gene != 0)
    {
        auto gene_chara_index = -1;
        for (const auto& ally : cdata.allies())
        {
            if (ally.has_made_gene())
            {
                if (ally.state() == Character::State::alive)
                {
                    gene_chara_index = ally.index;
                    break;
                }
            }
        }
        if (gene_chara_index != -1)
        {
            cdata[gene_chara_index].has_made_gene() = false;
            show_random_event_window(
                i18n::s.get("core.activity.sleep.new_gene.title"),
                i18n::s.get(
                    "core.activity.sleep.new_gene.text",
                    cdata[gene_chara_index]),
                {i18n::s.get_enum("core.activity.sleep.new_gene.choices", 0)},
                u8"bg_re14");
            save_gene();
        }
    }
    draw_sleep_background_frame();
    game_data.character_and_status_for_gene = 0;
    mode = 0;
    wake_up();
    cdata.player().nutrition -= 1500 / (trait(158) + 1);
    txt(i18n::s.get("core.activity.sleep.slept_for", timeslept),
        Message::color{ColorIndex::green});

    optional<int> bed_quality;
    if (bed && bed->param1 != 0 && bed->number() != 0 &&
        the_item_db[itemid2int(bed->id)]->subcategory == 60004)
    {
        bed_quality = bed->param1;
    }

    if (!bed_quality)
    {
        txt(i18n::s.get("core.activity.sleep.wake_up.so_so"));
    }
    else
    {
        i = 0;
        for (int cnt = 10; cnt < 18; ++cnt)
        {
            i += sdata.get(cnt, 0).original_level;
        }
        i = clamp(i / 6, 10, 1000);
        exp = i * i * i / 10;
        game_data.sleep_experience =
            game_data.sleep_experience * (*bed_quality) / 100;
        grown = 0;
        for (int cnt = 0;; ++cnt)
        {
            if (game_data.sleep_experience >= exp)
            {
                game_data.sleep_experience -= exp;
            }
            else if (cnt != 0)
            {
                break;
            }
            modify_potential(cdata.player(), 10 + rnd(8), 1);
            ++grown;
            if (cnt > 6)
            {
                if (rnd(5) == 0)
                {
                    game_data.sleep_experience = 0;
                    break;
                }
            }
        }
        txt(i18n::s.get("core.activity.sleep.wake_up.good", grown),
            Message::color{ColorIndex::green});
    }

    msg_halt();
    play_music();
    save_set_autosave();
    if (area_data[game_data.current_map].id == mdata_t::MapId::shop)
    {
        update_shop();
    }
}



void start_stealing(Character& thief, Item& steal_target)
{
    game_data.activity_about_to_start = 105;
    activity_others(thief, steal_target);
}

} // namespace elona
