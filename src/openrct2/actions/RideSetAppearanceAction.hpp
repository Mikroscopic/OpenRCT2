/*****************************************************************************
 * Copyright (c) 2014-2018 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include "../Cheats.h"
#include "../Context.h"
#include "../core/MemoryStream.h"
#include "../drawing/Drawing.h"
#include "../interface/Window.h"
#include "../localisation/Localisation.h"
#include "../localisation/StringIds.h"
#include "../ride/Ride.h"
#include "../ui/UiContext.h"
#include "../ui/WindowManager.h"
#include "../world/Park.h"
#include "GameAction.h"

enum class RideSetAppearanceType : uint8_t
{
    TrackColourMain,
    TrackColourAdditional,
    TrackColourSupports,
    MazeStyle = TrackColourSupports,
    VehicleColourBody,
    VehicleColourTrim,
    VehicleColourTernary,
    VehicleColourScheme,
    EntranceStyle
};

DEFINE_GAME_ACTION(RideSetAppearanceAction, GAME_COMMAND_SET_RIDE_APPEARANCE, GameActionResult)
{
private:
    NetworkRideId_t _rideIndex{ -1 };
    RideSetAppearanceType _type;
    uint8_t _value;
    uint32_t _index;

public:
    RideSetAppearanceAction()
    {
    }
    RideSetAppearanceAction(ride_id_t rideIndex, RideSetAppearanceType type, uint8_t value, uint32_t index)
        : _rideIndex(rideIndex)
        , _type(type)
        , _value(value)
        , _index(index)
    {
    }

    uint16_t GetActionFlags() const override
    {
        return GameAction::GetActionFlags() | GA_FLAGS::ALLOW_WHILE_PAUSED;
    }

    void Serialise(DataSerialiser & stream) override
    {
        GameAction::Serialise(stream);
        auto type = static_cast<uint8_t>(_type);
        stream << DS_TAG(_rideIndex) << DS_TAG(type) << DS_TAG(_value) << DS_TAG(_index);
    }

    GameActionResult::Ptr Query() const override
    {
        if (_rideIndex >= MAX_RIDES)
        {
            log_warning("Invalid game command for ride %u", _rideIndex);
            return std::make_unique<GameActionResult>(GA_ERROR::INVALID_PARAMETERS, STR_NONE);
        }

        Ride* ride = get_ride(_rideIndex);
        if (!ride || ride->type == RIDE_TYPE_NULL)
        {
            log_warning("Invalid game command, ride_id = %u", _rideIndex);
            return std::make_unique<GameActionResult>(GA_ERROR::INVALID_PARAMETERS, STR_NONE);
        }

        switch (_type)
        {
            case RideSetAppearanceType::TrackColourMain:
            case RideSetAppearanceType::TrackColourAdditional:
            case RideSetAppearanceType::TrackColourSupports:
                if (_index >= std::size(ride->track_colour))
                {
                    log_warning("Invalid game command, index %d out of bounds", _index);
                    return std::make_unique<GameActionResult>(GA_ERROR::INVALID_PARAMETERS, STR_NONE);
                }
                break;
            case RideSetAppearanceType::VehicleColourBody:
            case RideSetAppearanceType::VehicleColourTrim:
            case RideSetAppearanceType::VehicleColourTernary:
                if (_index >= std::size(ride->vehicle_colours))
                {
                    log_warning("Invalid game command, index %d out of bounds", _index);
                    return std::make_unique<GameActionResult>(GA_ERROR::INVALID_PARAMETERS, STR_NONE);
                }
                break;
            case RideSetAppearanceType::VehicleColourScheme:
            case RideSetAppearanceType::EntranceStyle:
                break;
            default:
                log_warning("Invalid game command, type %d not recognised", _type);
                return std::make_unique<GameActionResult>(GA_ERROR::INVALID_PARAMETERS, STR_NONE);
        }

        return std::make_unique<GameActionResult>();
    }

    GameActionResult::Ptr Execute() const override
    {
        Ride* ride = get_ride(_rideIndex);
        if (!ride)
        {
            log_warning("Invalid game command, ride_id = %u", _rideIndex);
            return std::make_unique<GameActionResult>(GA_ERROR::INVALID_PARAMETERS, STR_NONE);
        }

        switch (_type)
        {
            case RideSetAppearanceType::TrackColourMain:
                ride->track_colour[_index].main = _value;
                gfx_invalidate_screen();
                break;
            case RideSetAppearanceType::TrackColourAdditional:
                ride->track_colour[_index].additional = _value;
                gfx_invalidate_screen();
                break;
            case RideSetAppearanceType::TrackColourSupports:
                ride->track_colour[_index].supports = _value;
                gfx_invalidate_screen();
                break;
            case RideSetAppearanceType::VehicleColourBody:
                ride->vehicle_colours[_index].Body = _value;
                ride_update_vehicle_colours(ride);
                break;
            case RideSetAppearanceType::VehicleColourTrim:
                ride->vehicle_colours[_index].Trim = _value;
                ride_update_vehicle_colours(ride);
                break;
            case RideSetAppearanceType::VehicleColourTernary:
                ride->vehicle_colours[_index].Ternary = _value;
                ride_update_vehicle_colours(ride);
                break;
            case RideSetAppearanceType::VehicleColourScheme:
                ride->colour_scheme_type &= ~(RIDE_COLOUR_SCHEME_DIFFERENT_PER_TRAIN | RIDE_COLOUR_SCHEME_DIFFERENT_PER_CAR);
                ride->colour_scheme_type |= _value;
                for (uint32_t i = 1; i < std::size(ride->vehicle_colours); i++)
                {
                    ride->vehicle_colours[i] = ride->vehicle_colours[0];
                }
                ride_update_vehicle_colours(ride);
                break;
            case RideSetAppearanceType::EntranceStyle:
                ride->entrance_style = _value;
                gLastEntranceStyle = _value;
                gfx_invalidate_screen();
                break;
        }
        window_invalidate_by_number(WC_RIDE, _rideIndex);

        auto res = std::make_unique<GameActionResult>();
        if (ride->overall_view.xy != RCT_XY8_UNDEFINED)
        {
            LocationXYZ16 coord;
            coord.x = ride->overall_view.x * 32 + 16;
            coord.y = ride->overall_view.y * 32 + 16;
            coord.z = tile_element_height(coord.x, coord.y);
            res->Position.x = coord.x;
            res->Position.y = coord.y;
            res->Position.z = coord.z;
            network_set_player_last_action_coord(network_get_player_index(game_command_playerid), coord);
        }
        return res;
    }
};
