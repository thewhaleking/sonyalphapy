#include "PropertyValueTable.h"
#include <cmath>
#include <cstring>
#include "CameraDevice.h"


namespace SDK = SCRSDK;

namespace impl
{
template <typename T>
std::vector<T> parse(unsigned char const* buf, std::uint32_t nval)
{
    constexpr std::size_t const type_size = sizeof(T);
    std::vector<T> result(nval);
    std::uint32_t i = 0; T* source = static_cast<T*>(buf);
    for (; i < nval; ++i, ++source) {
        MemCpyEx(&result[i], source, type_size);
    }
}

inline double Round(double value, int figure)
{
    bool isNegative = ( value < 0 );
    if (isNegative == true) value = -value;

    double rate = std::pow(10.0, figure);
    //double value1 = value * rate + 0.5;

    long tmp = (long)(value * rate + 0.5);
    value = tmp/rate;

    if (isNegative == true) value = -value;

    return value;
}

// Retrieve the hi-order word (16-bit value) of a dword (32-bit value)
constexpr std::uint16_t HIWORD(std::uint32_t dword)
{
    return static_cast<std::uint16_t>((dword >> 16) & 0xFFFF);
}

// Retrieve the low-order word (16-bit value) of a dword (32-bit value)
constexpr std::uint16_t LOWORD(std::uint32_t dword)
{
    return static_cast<std::uint16_t>(dword & 0xFFFF);
}
} // namespace impl

namespace cli
{
std::vector<std::uint16_t> parse_f_number(unsigned char const* buf, std::uint32_t nval)
{
    using TargetType = std::uint16_t;
    constexpr std::size_t const type_size = sizeof(TargetType);
    TargetType const* source = reinterpret_cast<TargetType const*>(buf);
    std::vector<TargetType> result(nval);
    for (std::uint32_t i = 0; i < nval; ++i, ++source) {
        MemCpyEx(&result[i], source, type_size);
    }
    return result;
}

std::vector<std::uint32_t> parse_iso_sensitivity(unsigned char const* buf, std::uint32_t nval)
{
    using TargetType = std::uint32_t;
    constexpr std::size_t const type_size = sizeof(TargetType);
    TargetType const* source = reinterpret_cast<TargetType const*>(buf);
    std::vector<TargetType> result(nval);
    for (std::uint32_t i = 0; i < nval; ++i, ++source) {
        MemCpyEx(&result[i], source, type_size);
    }
    return result;
}

std::vector<std::uint32_t> parse_shutter_speed(unsigned char const* buf, std::uint32_t nval)
{
    using TargetType = std::uint32_t;
    constexpr std::size_t const type_size = sizeof(TargetType);
    TargetType const* source = reinterpret_cast<TargetType const*>(buf);
    std::vector<TargetType> result(nval);
    for (std::uint32_t i = 0; i < nval; ++i, ++source) {
        MemCpyEx(&result[i], source, type_size);
    }
    return result;
}

std::vector<std::uint16_t> parse_position_key_setting(unsigned char const* buf, std::uint32_t nval)
{
    using TargetType = std::uint16_t;
    constexpr std::size_t const type_size = sizeof(TargetType);
    TargetType const* source = reinterpret_cast<TargetType const*>(buf);
    std::vector<TargetType> result(nval);
    for (std::uint32_t i = 0; i < nval; ++i, ++source) {
        MemCpyEx(&result[i], source, type_size);
    }
    return result;
}

std::vector<std::uint32_t> parse_exposure_program_mode(unsigned char const* buf, std::uint32_t nval)
{
    using TargetType = std::uint32_t;
    constexpr std::size_t const type_size = sizeof(TargetType);
    TargetType const* source = reinterpret_cast<TargetType const*>(buf);
    std::vector<TargetType> result(nval);
    for (std::uint32_t i = 0; i < nval; ++i, ++source) {
        MemCpyEx(&result[i], source, type_size);
    }
    return result;
}

std::vector<std::uint32_t> parse_still_capture_mode(unsigned char const* buf, std::uint32_t nval)
{
    using TargetType = std::uint32_t;
    constexpr std::size_t const type_size = sizeof(TargetType);
    TargetType const* source = reinterpret_cast<TargetType const*>(buf);
    std::vector<TargetType> result(nval);
    for (std::uint32_t i = 0; i < nval; ++i, ++source) {
        MemCpyEx(&result[i], source, type_size);
    }
    return result;
}

std::vector<std::uint16_t> parse_focus_mode(unsigned char const* buf, std::uint32_t nval)
{
    using TargetType = std::uint16_t;
    constexpr std::size_t const type_size = sizeof(TargetType);
    TargetType const* source = reinterpret_cast<TargetType const*>(buf);
    std::vector<TargetType> result(nval);
    for (std::uint32_t i = 0; i < nval; ++i, ++source) {
        MemCpyEx(&result[i], source, type_size);
    }
    return result;
}

std::vector<std::uint16_t> parse_focus_area(unsigned char const* buf, std::uint32_t nval)
{
    using TargetType = std::uint16_t;
    constexpr std::size_t const type_size = sizeof(TargetType);
    TargetType const* source = reinterpret_cast<TargetType const*>(buf);
    std::vector<TargetType> result(nval);
    for (std::uint32_t i = 0; i < nval; ++i, ++source) {
        MemCpyEx(&result[i], source, type_size);
    }
    return result;
}

std::vector<std::uint16_t> parse_live_view_image_quality(unsigned char const* buf, std::uint32_t nval)
{
    using TargetType = std::uint16_t;
    constexpr std::size_t const type_size = sizeof(TargetType);
    TargetType const* source = reinterpret_cast<TargetType const*>(buf);
    std::vector<TargetType> result(nval);
    for (std::uint32_t i = 0; i < nval; ++i, ++source) {
        MemCpyEx(&result[i], source, type_size);
    }
    return result;
}

std::vector<std::uint8_t> parse_media_slotx_format_enable_status(unsigned char const* buf, std::uint32_t nval)
{
    using TargetType = std::uint8_t;
    constexpr std::size_t const type_size = sizeof(TargetType);
    TargetType const* source = reinterpret_cast<TargetType const*>(buf);
    std::vector<TargetType> result(nval);
    for (std::uint32_t i = 0; i < nval; ++i, ++source) {
        MemCpyEx(&result[i], source, type_size);
    }
    return result;
}

std::vector<std::uint16_t> parse_white_balance(unsigned char const* buf, std::uint32_t nval)
{
    using TargetType = std::uint16_t;
    constexpr std::size_t const type_size = sizeof(TargetType);
    TargetType const* source = reinterpret_cast<TargetType const*>(buf);
    std::vector<TargetType> result(nval);
    for (std::uint32_t i = 0; i < nval; ++i, ++source) {
        MemCpyEx(&result[i], source, type_size);
    }
    return result;
}

std::vector<std::uint16_t> parse_customwb_capture_standby(unsigned char const* buf, std::uint32_t nval)
{
    using TargetType = std::uint16_t;
    constexpr std::size_t const type_size = sizeof(TargetType);
    TargetType const* source = reinterpret_cast<TargetType const*>(buf);
    std::vector<TargetType> result(nval);
    for (std::uint32_t i = 0; i < nval; ++i, ++source) {
        MemCpyEx(&result[i], source, type_size);
    }
    return result;
}

std::vector<std::uint16_t> parse_customwb_capture_standby_cancel(unsigned char const* buf, std::uint32_t nval)
{
    using TargetType = std::uint16_t;
    constexpr std::size_t const type_size = sizeof(TargetType);
    TargetType const* source = reinterpret_cast<TargetType const*>(buf);
    std::vector<TargetType> result(nval);
    for (std::uint32_t i = 0; i < nval; ++i, ++source) {
        MemCpyEx(&result[i], source, type_size);
    }
    return result;
}

std::vector<std::uint16_t> parse_customwb_capture_operation(unsigned char const* buf, std::uint32_t nval)
{
    using TargetType = std::uint16_t;
    constexpr std::size_t const type_size = sizeof(TargetType);
    TargetType const* source = reinterpret_cast<TargetType const*>(buf);
    std::vector<TargetType> result(nval);
    for (std::uint32_t i = 0; i < nval; ++i, ++source) {
        MemCpyEx(&result[i], source, type_size);
    }
    return result;
}

std::vector<std::uint16_t> parse_customwb_capture_execution_state(unsigned char const* buf, std::uint32_t nval)
{
    using TargetType = std::uint16_t;
    constexpr std::size_t const type_size = sizeof(TargetType);
    TargetType const* source = reinterpret_cast<TargetType const*>(buf);
    std::vector<TargetType> result(nval);
    for (std::uint32_t i = 0; i < nval; ++i, ++source) {
        MemCpyEx(&result[i], source, type_size);
    }
    return result;
}

std::vector<std::uint8_t> parse_zoom_operation_status(unsigned char const* buf, std::uint32_t nval)
{
    using TargetType = std::uint8_t;
    constexpr std::size_t const type_size = sizeof(TargetType);
    TargetType const* source = reinterpret_cast<TargetType const*>(buf);
    std::vector<TargetType> result(nval);
    for (std::uint32_t i = 0; i < nval; ++i, ++source) {
        MemCpyEx(&result[i], source, type_size);
    }
    return result;
}

std::vector<std::uint8_t> parse_zoom_setting_type(unsigned char const* buf, std::uint32_t nval)
{
    using TargetType = std::uint8_t;
    constexpr std::size_t const type_size = sizeof(TargetType);
    TargetType const* source = reinterpret_cast<TargetType const*>(buf);
    std::vector<TargetType> result(nval);
    for (std::uint32_t i = 0; i < nval; ++i, ++source) {
        MemCpyEx(&result[i], source, type_size);
    }
    return result;
}

std::vector<std::uint8_t> parse_zoom_types_status(unsigned char const* buf, std::uint32_t nval)
{
    using TargetType = std::uint8_t;
    constexpr std::size_t const type_size = sizeof(TargetType);
    TargetType const* source = reinterpret_cast<TargetType const*>(buf);
    std::vector<TargetType> result(nval);
    for (std::uint32_t i = 0; i < nval; ++i, ++source) {
        MemCpyEx(&result[i], source, type_size);
    }
    return result;
}

std::vector<std::int8_t> parse_zoom_operation(unsigned char const* buf, std::uint32_t nval)
{
    using TargetType = std::int8_t;
    constexpr std::size_t const type_size = sizeof(TargetType);
    TargetType const* source = reinterpret_cast<TargetType const*>(buf);
    std::vector<TargetType> result(nval);
    for (std::uint32_t i = 0; i < nval; ++i, ++source) {
        MemCpyEx(&result[i], source, type_size);
    }
    return result;
}

std::vector<std::int8_t> parse_zoom_speed_range(unsigned char const* buf, std::uint32_t nval)
{
    using TargetType = std::int8_t;
    constexpr std::size_t const type_size = sizeof(TargetType);
    TargetType const* source = reinterpret_cast<TargetType const*>(buf);
    std::vector<TargetType> result(nval);
    for (std::uint32_t i = 0; i < nval; ++i, ++source) {
        MemCpyEx(&result[i], source, type_size);
    }
    return result;
}

std::vector<std::uint8_t> parse_save_zoom_and_focus_position(unsigned char const* buf, std::uint32_t nval)
{
    using TargetType = std::uint8_t;
    constexpr std::size_t const type_size = sizeof(TargetType);
    TargetType const* source = reinterpret_cast<TargetType const*>(buf);
    std::vector<TargetType> result(nval);
    for (std::uint32_t i = 0; i < nval; ++i, ++source) {
        MemCpyEx(&result[i], source, type_size);
    }
    return result;
}

std::vector<std::uint8_t> parse_load_zoom_and_focus_position(unsigned char const* buf, std::uint32_t nval)
{
    using TargetType = std::uint8_t;
    constexpr std::size_t const type_size = sizeof(TargetType);
    TargetType const* source = reinterpret_cast<TargetType const*>(buf);
    std::vector<TargetType> result(nval);
    for (std::uint32_t i = 0; i < nval; ++i, ++source) {
        MemCpyEx(&result[i], source, type_size);
    }
    return result;
}

std::vector<std::uint8_t> parse_remocon_zoom_speed_type(unsigned char const* buf, std::uint32_t nval)
{
    using TargetType = std::uint8_t;
    constexpr std::size_t const type_size = sizeof(TargetType);
    TargetType const* source = reinterpret_cast<TargetType const*>(buf);
    std::vector<TargetType> result(nval);
    for (std::uint32_t i = 0; i < nval; ++i, ++source) {
        MemCpyEx(&result[i], source, type_size);
    }
    return result;
}

std::vector<std::uint8_t> parse_playback_media(unsigned char const* buf, std::uint32_t nval)
{
    using TargetType = std::uint8_t;
    constexpr std::size_t const type_size = sizeof(TargetType);
    TargetType const* source = reinterpret_cast<TargetType const*>(buf);
    std::vector<TargetType> result(nval);
    for (std::uint32_t i = 0; i < nval; ++i, ++source) {
        MemCpyEx(&result[i], source, type_size);
    }
    return result;
}

std::vector<std::uint8_t> parse_gain_base_sensitivity(unsigned char const* buf, std::uint32_t nval)
{
    using TargetType = std::uint8_t;
    constexpr std::size_t const type_size = sizeof(TargetType);
    TargetType const* source = reinterpret_cast<TargetType const*>(buf);
    std::vector<TargetType> result(nval);
    for (std::uint32_t i = 0; i < nval; ++i, ++source) {
        MemCpyEx(&result[i], source, type_size);
    }
    return result;
}
std::vector<std::uint8_t> parse_gain_base_iso_sensitivity(unsigned char const* buf, std::uint32_t nval)
{
    using TargetType = std::uint8_t;
    constexpr std::size_t const type_size = sizeof(TargetType);
    TargetType const* source = reinterpret_cast<TargetType const*>(buf);
    std::vector<TargetType> result(nval);
    for (std::uint32_t i = 0; i < nval; ++i, ++source) {
        MemCpyEx(&result[i], source, type_size);
    }
    return result;
}

std::vector<std::uint8_t> parse_monitor_lut_setting(unsigned char const* buf, std::uint32_t nval)
{
    using TargetType = std::uint8_t;
    constexpr std::size_t const type_size = sizeof(TargetType);
    TargetType const* source = reinterpret_cast<TargetType const*>(buf);
    std::vector<TargetType> result(nval);
    for (std::uint32_t i = 0; i < nval; ++i, ++source) {
        MemCpyEx(&result[i], source, type_size);
    }
    return result;
}

std::vector<std::uint16_t> parse_exposure_index(unsigned char const* buf, std::uint32_t nval)
{
    using TargetType = std::uint16_t;
    constexpr std::size_t const type_size = sizeof(TargetType);
    TargetType const* source = reinterpret_cast<TargetType const*>(buf);
    std::vector<TargetType> result(nval);
    for (std::uint32_t i = 0; i < nval; ++i, ++source) {
        MemCpyEx(&result[i], source, type_size);
    }
    return result;
}

std::vector<std::uint16_t> parse_baselook_value(unsigned char const* buf, std::uint32_t nval)
{
    using TargetType = std::uint16_t;
    constexpr std::size_t const type_size = sizeof(TargetType);
    TargetType const* source = reinterpret_cast<TargetType const*>(buf);
    std::vector<TargetType> result(nval);
    for (std::uint32_t i = 0; i < nval; ++i, ++source) {
        MemCpyEx(&result[i], source, type_size);
    }
    return result;
}

std::vector<std::uint8_t> parse_shutter_mode_setting(unsigned char const* buf, std::uint32_t nval)
{
    using TargetType = std::uint8_t;
    constexpr std::size_t const type_size = sizeof(TargetType);
    TargetType const* source = reinterpret_cast<TargetType const*>(buf);
    std::vector<TargetType> result(nval);
    for (std::uint32_t i = 0; i < nval; ++i, ++source) {
        MemCpyEx(&result[i], source, type_size);
    }
    return result;
}

std::vector<std::uint8_t> parse_iris_mode_setting(unsigned char const* buf, std::uint32_t nval)
{
    using TargetType = std::uint8_t;
    constexpr std::size_t const type_size = sizeof(TargetType);
    TargetType const* source = reinterpret_cast<TargetType const*>(buf);
    std::vector<TargetType> result(nval);
    for (std::uint32_t i = 0; i < nval; ++i, ++source) {
        MemCpyEx(&result[i], source, type_size);
    }
    return result;
}

std::vector<std::uint8_t> parse_gain_control_setting(unsigned char const* buf, std::uint32_t nval)
{
    using TargetType = std::uint8_t;
    constexpr std::size_t const type_size = sizeof(TargetType);
    TargetType const* source = reinterpret_cast<TargetType const*>(buf);
    std::vector<TargetType> result(nval);
    for (std::uint32_t i = 0; i < nval; ++i, ++source) {
        MemCpyEx(&result[i], source, type_size);
    }
    return result;
}

std::vector<std::uint8_t> parse_exposure_control_type(unsigned char const* buf, std::uint32_t nval)
{
    using TargetType = std::uint8_t;
    constexpr std::size_t const type_size = sizeof(TargetType);
    TargetType const* source = reinterpret_cast<TargetType const*>(buf);
    std::vector<TargetType> result(nval);
    for (std::uint32_t i = 0; i < nval; ++i, ++source) {
        MemCpyEx(&result[i], source, type_size);
    }
    return result;
}

std::vector<std::uint16_t> parse_recording_setting(unsigned char const* buf, std::uint32_t nval)
{
    using TargetType = std::uint16_t;
    constexpr std::size_t const type_size = sizeof(TargetType);
    TargetType const* source = reinterpret_cast<TargetType const*>(buf);
    std::vector<TargetType> result(nval);
    for (std::uint32_t i = 0; i < nval; ++i, ++source) {
        MemCpyEx(&result[i], source, type_size);
    }
    return result;
}

std::vector<std::uint32_t> parse_dispmode_candidate(unsigned char const* buf, std::uint32_t nval)
{
    using TargetType = std::uint32_t;
    constexpr std::size_t const type_size = sizeof(TargetType);
    TargetType const* source = reinterpret_cast<TargetType const*>(buf);
    std::vector<TargetType> result(nval);
    for (std::uint32_t i = 0; i < nval; ++i, ++source) {
        MemCpyEx(&result[i], source, type_size);
    }
    return result;
}

std::vector<std::uint32_t> parse_dispmode_setting(unsigned char const* buf, std::uint32_t nval)
{
    using TargetType = std::uint32_t;
    constexpr std::size_t const type_size = sizeof(TargetType);
    TargetType const* source = reinterpret_cast<TargetType const*>(buf);
    std::vector<TargetType> result(nval);
    for (std::uint32_t i = 0; i < nval; ++i, ++source) {
        MemCpyEx(&result[i], source, type_size);
    }
    return result;
}

std::vector<std::uint8_t> parse_dispmode(unsigned char const* buf, std::uint32_t nval)
{
    using TargetType = std::uint8_t;
    constexpr std::size_t const type_size = sizeof(TargetType);
    TargetType const* source = reinterpret_cast<TargetType const*>(buf);
    std::vector<TargetType> result(nval);
    for (std::uint32_t i = 0; i < nval; ++i, ++source) {
        MemCpyEx(&result[i], source, type_size);
    }
    return result;
}

std::vector<std::int8_t> parse_gain_db_value(unsigned char const* buf, std::int32_t nval)
{
    using TargetType = std::int8_t;
    constexpr std::size_t const type_size = sizeof(TargetType);
    TargetType const* source = reinterpret_cast<TargetType const*>(buf);
    std::vector<TargetType> result(nval);
    for (std::uint32_t i = 0; i < nval; ++i, ++source) {
        MemCpyEx(&result[i], source, type_size);
    }
    return result;
}

std::vector<std::int8_t> parse_white_balance_tint(unsigned char const* buf, std::int32_t nval)
{
    using TargetType = std::int8_t;
    constexpr std::size_t const type_size = sizeof(TargetType);
    TargetType const* source = reinterpret_cast<TargetType const*>(buf);
    std::vector<TargetType> result(nval);
    for (std::uint32_t i = 0; i < nval; ++i, ++source) {
        MemCpyEx(&result[i], source, type_size);
    }
    return result;
}

std::vector<std::int16_t> parse_white_balance_tint_step(unsigned char const* buf, std::int32_t nval)
{
    using TargetType = std::int16_t;
    constexpr std::size_t const type_size = sizeof(TargetType);
    TargetType const* source = reinterpret_cast<TargetType const*>(buf);
    std::vector<TargetType> result(nval);
    for (std::uint32_t i = 0; i < nval; ++i, ++source) {
        MemCpyEx(&result[i], source, type_size);
    }
    return result;
}

std::vector<std::uint64_t> parse_shutter_speed_value(unsigned char const* buf, std::uint32_t nval)
{
    using TargetType = std::uint64_t;
    constexpr std::size_t const type_size = sizeof(TargetType);
    TargetType const* source = reinterpret_cast<TargetType const*>(buf);
    std::vector<TargetType> result(nval);
    for (std::uint32_t i = 0; i < nval; ++i, ++source) {
        MemCpyEx(&result[i], source, type_size);
    }
    return result;
}

std::vector<std::uint16_t> parse_focus_bracket_shot_num(unsigned char const* buf, std::uint32_t nval)
{
    using TargetType = std::uint16_t;
    constexpr std::size_t const type_size = sizeof(TargetType);
    TargetType const* source = reinterpret_cast<TargetType const*>(buf);
    std::vector<TargetType> result(nval);
    for (std::uint32_t i = 0; i < nval; ++i, ++source) {
        MemCpyEx(&result[i], source, type_size);
    }
    return result;
}

std::vector<std::uint8_t> parse_focus_bracket_focus_range(unsigned char const* buf, std::uint32_t nval)
{
    using TargetType = std::uint8_t;
    constexpr std::size_t const type_size = sizeof(TargetType);
    TargetType const* source = reinterpret_cast<TargetType const*>(buf);
    std::vector<TargetType> result(nval);
    for (std::uint32_t i = 0; i < nval; ++i, ++source) {
        MemCpyEx(&result[i], source, type_size);
    }
    return result;
}

std::vector<std::uint8_t> parse_image_stabilization_steady_shot(unsigned char const* buf, std::uint32_t nval)
{
    using TargetType = std::uint8_t;
    constexpr std::size_t const type_size = sizeof(TargetType);
    TargetType const* source = reinterpret_cast<TargetType const*>(buf);
    std::vector<TargetType> result(nval);
    for (std::uint32_t i = 0; i < nval; ++i, ++source) {
        MemCpyEx(&result[i], source, type_size);
    }
    return result;
}

std::vector<std::uint8_t> parse_movie_image_stabilization_steady_shot(unsigned char const* buf, std::uint32_t nval)
{
    using TargetType = std::uint8_t;
    constexpr std::size_t const type_size = sizeof(TargetType);
    TargetType const* source = reinterpret_cast<TargetType const*>(buf);
    std::vector<TargetType> result(nval);
    for (std::uint32_t i = 0; i < nval; ++i, ++source) {
        MemCpyEx(&result[i], source, type_size);
    }
    return result;
}

std::vector<std::uint8_t> parse_silent_mode(unsigned char const* buf, std::uint32_t nval)
{
    using TargetType = std::uint8_t;
    constexpr std::size_t const type_size = sizeof(TargetType);
    TargetType const* source = reinterpret_cast<TargetType const*>(buf);
    std::vector<TargetType> result(nval);
    for (std::uint32_t i = 0; i < nval; ++i, ++source) {
        MemCpyEx(&result[i], source, type_size);
    }
    return result;
}

std::vector<std::uint8_t> parse_silent_mode_aperture_drive_in_af(unsigned char const* buf, std::uint32_t nval)
{
    using TargetType = std::uint8_t;
    constexpr std::size_t const type_size = sizeof(TargetType);
    TargetType const* source = reinterpret_cast<TargetType const*>(buf);
    std::vector<TargetType> result(nval);
    for (std::uint32_t i = 0; i < nval; ++i, ++source) {
        MemCpyEx(&result[i], source, type_size);
    }
    return result;
}
std::vector<std::uint8_t> parse_silent_mode_shutter_when_power_off(unsigned char const* buf, std::uint32_t nval)
{
    using TargetType = std::uint8_t;
    constexpr std::size_t const type_size = sizeof(TargetType);
    TargetType const* source = reinterpret_cast<TargetType const*>(buf);
    std::vector<TargetType> result(nval);
    for (std::uint32_t i = 0; i < nval; ++i, ++source) {
        MemCpyEx(&result[i], source, type_size);
    }
    return result;
}

std::vector<std::uint8_t> parse_silent_mode_auto_pixel_mapping(unsigned char const* buf, std::uint32_t nval)
{
    using TargetType = std::uint8_t;
    constexpr std::size_t const type_size = sizeof(TargetType);
    TargetType const* source = reinterpret_cast<TargetType const*>(buf);
    std::vector<TargetType> result(nval);
    for (std::uint32_t i = 0; i < nval; ++i, ++source) {
        MemCpyEx(&result[i], source, type_size);
    }
    return result;
}

std::vector<std::uint8_t> parse_shutter_type(unsigned char const* buf, std::uint32_t nval)
{
    using TargetType = std::uint8_t;
    constexpr std::size_t const type_size = sizeof(TargetType);
    TargetType const* source = reinterpret_cast<TargetType const*>(buf);
    std::vector<TargetType> result(nval);
    for (std::uint32_t i = 0; i < nval; ++i, ++source) {
        MemCpyEx(&result[i], source, type_size);
    }
    return result;
}

std::vector<std::uint16_t> parse_movie_shooting_mode(unsigned char const* buf, std::uint32_t nval)
{
    using TargetType = std::uint16_t;
    constexpr std::size_t const type_size = sizeof(TargetType);
    TargetType const* source = reinterpret_cast<TargetType const*>(buf);
    std::vector<TargetType> result(nval);
    for (std::uint32_t i = 0; i < nval; ++i, ++source) {
        MemCpyEx(&result[i], source, type_size);
    }
    return result;
}

text format_f_number(std::uint16_t f_number)
{
    text_stringstream ts;
    if ((0x0000 == f_number) || (SDK::CrFnumber_Unknown == f_number)) {
        ts << TEXT("--");
    }
    else if(SDK::CrFnumber_Nothing == f_number) {
        // Do nothing
    }
    else {
        auto modValue = static_cast<std::uint16_t>(f_number % 100);
        if (modValue > 0) {
            ts << TEXT('F') << impl::Round((f_number / 100.0), 1);
        }
        else {
            ts << TEXT('F') << f_number / 100;
        }
    }

    return ts.str();
}

text format_iso_sensitivity(std::uint32_t iso)
{
    text_stringstream ts;

    std::uint32_t iso_ext = (iso >> 24) & 0x000000F0;  // bit 28-31
    std::uint32_t iso_mode = (iso >> 24) & 0x0000000F; // bit 24-27
    std::uint32_t iso_value = (iso & 0x00FFFFFF);      // bit  0-23

    if (iso_mode == SDK::CrISO_MultiFrameNR) {
        ts << TEXT("Multi Frame NR ");
    }
    else if (iso_mode == SDK::CrISO_MultiFrameNR_High) {
        ts << TEXT("Multi Frame NR High ");
    }

    if (iso_value == SDK::CrISO_AUTO) {
        ts << TEXT("ISO AUTO");
    }
    else {
        ts << TEXT("ISO ") << iso_value;
    }

    //if (iso_ext == SDK::CrISO_Ext) {
    //    ts << TEXT(" (EXT)");
    //}

    return ts.str();
}

text format_shutter_speed(std::uint32_t shutter_speed)
{
    text_stringstream ts;

    CrInt16u numerator = impl::HIWORD(shutter_speed);
    CrInt16u denominator = impl::LOWORD(shutter_speed);

    if (0 == shutter_speed) {
        ts << TEXT("Bulb");
    }
    else if (1 == numerator) {
        ts << numerator << '/' << denominator;
    }
    else if (0 == numerator % denominator) {
        ts << (numerator / denominator) << '"';
    }
    else {
        CrInt32 numdivision = numerator / denominator;
        CrInt32 numremainder = numerator % denominator;
        ts << numdivision << '.' << numremainder << '"';
    }
    return ts.str();
}

text format_position_key_setting(std::uint16_t position_key_setting)
{
    text_stringstream ts;

    switch (position_key_setting) {
    case SDK::CrPriorityKeySettings::CrPriorityKey_CameraPosition:
        ts << "Camera Position";
        break;
    case SDK::CrPriorityKeySettings::CrPriorityKey_PCRemote:
        ts << "PC Remote Setting";
        break;
    default:
        break;
    }

    return ts.str();
}

text format_exposure_program_mode(std::uint32_t exposure_program_mode)
{
    text_stringstream ts;

    switch (exposure_program_mode) {
    case SDK::CrExposureProgram::CrExposure_M_Manual:
        ts << "M_Manual";
        break;
    case SDK::CrExposureProgram::CrExposure_P_Auto:
        ts << "P_Auto";
        break;
    case SDK::CrExposureProgram::CrExposure_A_AperturePriority:
        ts << "A_AperturePriority";
        break;
    case SDK::CrExposureProgram::CrExposure_S_ShutterSpeedPriority:
        ts << "S_ShutterSpeedPriority";
        break;
     case SDK::CrExposureProgram::CrExposure_Program_Creative:
         ts << "ProgramCreative";
         break;
     case SDK::CrExposureProgram::CrExposure_Program_Action:
         ts << "ProgramAction";
         break;
    case SDK::CrExposureProgram::CrExposure_Portrait:
        ts << "Portrait";
        break;
    case SDK::CrExposureProgram::CrExposure_Auto:
        ts << "Auto";
        break;
    case SDK::CrExposureProgram::CrExposure_Auto_Plus:
        ts << "Auto_Plus";
        break;
    case SDK::CrExposureProgram::CrExposure_P_A:
        ts << "P_A";
        break;
    case SDK::CrExposureProgram::CrExposure_P_S:
        ts << "P_S";
        break;
    case SDK::CrExposureProgram::CrExposure_Sports_Action:
        ts << "Sports_Action";
        break;
    case SDK::CrExposureProgram::CrExposure_Sunset:
        ts << "Sunset";
        break;
    case SDK::CrExposureProgram::CrExposure_Night:
        ts << "Night";
        break;
    case SDK::CrExposureProgram::CrExposure_Landscape:
        ts << "Landscape";
        break;
    case SDK::CrExposureProgram::CrExposure_Macro:
        ts << "Macro";
        break;
    case SDK::CrExposureProgram::CrExposure_HandheldTwilight:
        ts << "HandheldTwilight";
        break;
    case SDK::CrExposureProgram::CrExposure_NightPortrait:
        ts << "NightPortrait";
        break;
    case SDK::CrExposureProgram::CrExposure_AntiMotionBlur:
        ts << "AntiMotionBlur";
        break;
    case SDK::CrExposureProgram::CrExposure_Pet:
        ts << "Pet";
        break;
    case SDK::CrExposureProgram::CrExposure_Gourmet:
        ts << "Gourmet";
        break;
    case SDK::CrExposureProgram::CrExposure_Fireworks:
        ts << "Fireworks";
        break;
    case SDK::CrExposureProgram::CrExposure_HighSensitivity:
        ts << "HighSensitivity";
        break;
    case SDK::CrExposureProgram::CrExposure_MemoryRecall:
        ts << "MemoryRecall";
        break;
    case SDK::CrExposureProgram::CrExposure_ContinuousPriority_AE_8pics:
        ts << "ContinuousPriority_AE_8pics";
        break;
    case SDK::CrExposureProgram::CrExposure_ContinuousPriority_AE_10pics:
        ts << "ContinuousPriority_AE_10pics";
        break;
    case SDK::CrExposureProgram::CrExposure_ContinuousPriority_AE_12pics:
        ts << "ContinuousPriority_AE_12pics";
        break;
    case SDK::CrExposureProgram::CrExposure_3D_SweepPanorama:
        ts << "3D_SweepPanorama";
        break;
    case SDK::CrExposureProgram::CrExposure_SweepPanorama:
        ts << "SweepPanorama";
        break;
    case SDK::CrExposureProgram::CrExposure_Movie_P:
        ts << "Movie_P";
        break;
    case SDK::CrExposureProgram::CrExposure_Movie_A:
        ts << "Movie_A";
        break;
    case SDK::CrExposureProgram::CrExposure_Movie_S:
        ts << "Movie_S";
        break;
    case SDK::CrExposureProgram::CrExposure_Movie_M:
        ts << "Movie_M";
        break;
    case SDK::CrExposureProgram::CrExposure_Movie_Auto:
        ts << "Movie_Auto";
        break;
    case SDK::CrExposureProgram::CrExposure_Movie_F:
        ts << "Movie_F";
        break;
    case SDK::CrExposureProgram::CrExposure_Movie_SQMotion_P:
        ts << "Movie_SQMotion_P";
        break;
    case SDK::CrExposureProgram::CrExposure_Movie_SQMotion_A:
        ts << "Movie_SQMotion_A";
        break;
    case SDK::CrExposureProgram::CrExposure_Movie_SQMotion_S:
        ts << "Movie_SQMotion_S";
        break;
    case SDK::CrExposureProgram::CrExposure_Movie_SQMotion_M:
        ts << "Movie_SQMotion_M";
        break;
    case SDK::CrExposureProgram::CrExposure_Movie_SQMotion_AUTO:
        ts << "Movie_SQMotion_AUTO";
        break;
    case SDK::CrExposureProgram::CrExposure_Movie_SQMotion_F:
        ts << "Movie_SQMotion_F";
        break;
    case SDK::CrExposureProgram::CrExposure_Flash_Off:
         ts << "FlashOff";
         break;
     case SDK::CrExposureProgram::CrExposure_PictureEffect:
         ts << "PictureEffect";
         break;
    case SDK::CrExposureProgram::CrExposure_HiFrameRate_P:
        ts << "HiFrameRate_P";
        break;
    case SDK::CrExposureProgram::CrExposure_HiFrameRate_A:
        ts << "HiFrameRate_A";
        break;
    case SDK::CrExposureProgram::CrExposure_HiFrameRate_S:
        ts << "HiFrameRate_S";
        break;
    case SDK::CrExposureProgram::CrExposure_HiFrameRate_M:
        ts << "HiFrameRate_M";
        break;
    case SDK::CrExposureProgram::CrExposure_SQMotion_P:
        ts << "SQMotion_P";
        break;
    case SDK::CrExposureProgram::CrExposure_SQMotion_A:
        ts << "SQMotion_A";
        break;
    case SDK::CrExposureProgram::CrExposure_SQMotion_S:
        ts << "SQMotion_S";
        break;
    case SDK::CrExposureProgram::CrExposure_SQMotion_M:
        ts << "SQMotion_M";
        break;
    case SDK::CrExposureProgram::CrExposure_MOVIE:
        ts << "MOVIE";
        break;
    case SDK::CrExposureProgram::CrExposure_STILL:
        ts << "STILL";
        break;
    case SDK::CrExposureProgram::CrExposure_F_MovieOrSQMotion:
        ts << "F(Movie or S&Q)";
        break;
    case SDK::CrExposureProgram::CrExposure_Movie_IntervalRec_F:
        ts << "Movie_IntervalRec_F";
        break;
    case SDK::CrExposureProgram::CrExposure_Movie_IntervalRec_P:
        ts << "Movie_IntervalRec_P";
        break;
    case SDK::CrExposureProgram::CrExposure_Movie_IntervalRec_A:
        ts << "Movie_IntervalRec_A";
        break;
    case SDK::CrExposureProgram::CrExposure_Movie_IntervalRec_S:
        ts << "Movie_IntervalRec_S";
        break;
    case SDK::CrExposureProgram::CrExposure_Movie_IntervalRec_M:
        ts << "Movie_IntervalRec_M";
        break;
    case SDK::CrExposureProgram::CrExposure_Movie_IntervalRec_AUTO:
        ts << "Movie_IntervalRec_AUTO";
        break;
    default:
        break;
    }

    return ts.str();
}

text format_still_capture_mode(std::uint32_t still_capture_mode)
{
    text_stringstream ts;
    switch (still_capture_mode) {
    case SDK::CrDriveMode::CrDrive_Single:
        ts << "CrDrive_Single";
        break;
    case SDK::CrDriveMode::CrDrive_Continuous_Hi:
        ts << "CrDrive_Continuous_Hi";
        break;
    case SDK::CrDriveMode::CrDrive_Continuous_Hi_Plus:
        ts << "CrDrive_Continuous_Hi_Plus";
        break;
    case SDK::CrDriveMode::CrDrive_Continuous_Hi_Live:
        ts << "CrDrive_Continuous_Hi_Live";
        break;
    case SDK::CrDriveMode::CrDrive_Continuous_Lo:
        ts << "CrDrive_Continuous_Lo";
        break;
    case SDK::CrDriveMode::CrDrive_Continuous:
        ts << "CrDrive_Continuous";
        break;
    case SDK::CrDriveMode::CrDrive_Continuous_SpeedPriority:
        ts << "CrDrive_Continuous_SpeedPriority";
        break;
    case SDK::CrDriveMode::CrDrive_Continuous_Mid:
        ts << "CrDrive_Continuous_Mid";
        break;
    case SDK::CrDriveMode::CrDrive_Continuous_Mid_Live:
        ts << "CrDrive_Continuous_Mid_Live";
        break;
    case SDK::CrDriveMode::CrDrive_Continuous_Lo_Live:
        ts << "CrDrive_Continuous_Lo_Live";
        break;
    case SDK::CrDriveMode::CrDrive_SingleBurstShooting_lo:
        ts << "CrDrive_SingleBurstShooting_lo";
        break;
    case SDK::CrDriveMode::CrDrive_SingleBurstShooting_mid:
        ts << "CrDrive_SingleBurstShooting_mid";
        break;
    case SDK::CrDriveMode::CrDrive_SingleBurstShooting_hi:
        ts << "CrDrive_SingleBurstShooting_hi";
        break;
    case SDK::CrDriveMode::CrDrive_Timelapse:
        ts << "CrDrive_Timelapse";
        break;
    case SDK::CrDriveMode::CrDrive_Timer_2s:
        ts << "CrDrive_Timer_2s";
        break;
    case SDK::CrDriveMode::CrDrive_Timer_5s:
        ts << "CrDrive_Timer_5s";
        break;
    case SDK::CrDriveMode::CrDrive_Timer_10s:
        ts << "CrDrive_Timer_10s";
        break;
    case SDK::CrDriveMode::CrDrive_Continuous_Bracket_03Ev_3pics:
        ts << "CrDrive_Continuous_Bracket_03Ev_3pics";
        break;
    case SDK::CrDriveMode::CrDrive_Continuous_Bracket_03Ev_5pics:
        ts << "CrDrive_Continuous_Bracket_03Ev_5pics";
        break;
    case SDK::CrDriveMode::CrDrive_Continuous_Bracket_03Ev_9pics:
        ts << "CrDrive_Continuous_Bracket_03Ev_9pics";
        break;
    case SDK::CrDriveMode::CrDrive_Continuous_Bracket_05Ev_3pics:
        ts << "CrDrive_Continuous_Bracket_05Ev_3pics";
        break;
    case SDK::CrDriveMode::CrDrive_Continuous_Bracket_05Ev_5pics:
        ts << "CrDrive_Continuous_Bracket_05Ev_5pics";
        break;
    case SDK::CrDriveMode::CrDrive_Continuous_Bracket_05Ev_9pics:
        ts << "CrDrive_Continuous_Bracket_05Ev_9pics";
        break;
    case SDK::CrDriveMode::CrDrive_Continuous_Bracket_07Ev_3pics:
        ts << "CrDrive_Continuous_Bracket_07Ev_3pics";
        break;
    case SDK::CrDriveMode::CrDrive_Continuous_Bracket_07Ev_5pics:
        ts << "CrDrive_Continuous_Bracket_07Ev_5pics";
        break;
    case SDK::CrDriveMode::CrDrive_Continuous_Bracket_07Ev_9pics:
        ts << "CrDrive_Continuous_Bracket_07Ev_9pics";
        break;
    case SDK::CrDriveMode::CrDrive_Continuous_Bracket_10Ev_3pics:
        ts << "CrDrive_Continuous_Bracket_10Ev_3pics";
        break;
    case SDK::CrDriveMode::CrDrive_Continuous_Bracket_10Ev_5pics:
        ts << "CrDrive_Continuous_Bracket_10Ev_5pics";
        break;
    case SDK::CrDriveMode::CrDrive_Continuous_Bracket_10Ev_9pics:
        ts << "CrDrive_Continuous_Bracket_10Ev_9pics";
        break;
    case SDK::CrDriveMode::CrDrive_Continuous_Bracket_20Ev_3pics:
        ts << "CrDrive_Continuous_Bracket_20Ev_3pics";
        break;
    case SDK::CrDriveMode::CrDrive_Continuous_Bracket_20Ev_5pics:
        ts << "CrDrive_Continuous_Bracket_20Ev_5pics";
        break;
    case SDK::CrDriveMode::CrDrive_Continuous_Bracket_30Ev_3pics:
        ts << "CrDrive_Continuous_Bracket_30Ev_3pics";
        break;
    case SDK::CrDriveMode::CrDrive_Continuous_Bracket_30Ev_5pics:
        ts << "CrDrive_Continuous_Bracket_30Ev_5pics";
        break;
    case SDK::CrDriveMode::CrDrive_Continuous_Bracket_03Ev_2pics_Plus:
        ts << "CrDrive_Continuous_Bracket_03Ev_2pics_Plus";
        break;
    case SDK::CrDriveMode::CrDrive_Continuous_Bracket_03Ev_2pics_Minus:
        ts << "CrDrive_Continuous_Bracket_03Ev_2pics_Minus";
        break;
    case SDK::CrDriveMode::CrDrive_Continuous_Bracket_03Ev_7pics:
        ts << "CrDrive_Continuous_Bracket_03Ev_7pics";
        break;
    case SDK::CrDriveMode::CrDrive_Continuous_Bracket_05Ev_2pics_Plus:
        ts << "CrDrive_Continuous_Bracket_05Ev_2pics_Plus";
        break;
    case SDK::CrDriveMode::CrDrive_Continuous_Bracket_05Ev_2pics_Minus:
        ts << "CrDrive_Continuous_Bracket_05Ev_2pics_Minus";
        break;
    case SDK::CrDriveMode::CrDrive_Continuous_Bracket_05Ev_7pics:
        ts << "CrDrive_Continuous_Bracket_05Ev_7pics";
        break;
    case SDK::CrDriveMode::CrDrive_Continuous_Bracket_07Ev_2pics_Plus:
        ts << "CrDrive_Continuous_Bracket_07Ev_2pics_Plus";
        break;
    case SDK::CrDriveMode::CrDrive_Continuous_Bracket_07Ev_2pics_Minus:
        ts << "CrDrive_Continuous_Bracket_07Ev_2pics_Minus";
        break;
    case SDK::CrDriveMode::CrDrive_Continuous_Bracket_07Ev_7pics:
        ts << "CrDrive_Continuous_Bracket_07Ev_7pics";
        break;
    case SDK::CrDriveMode::CrDrive_Continuous_Bracket_10Ev_2pics_Plus:
        ts << "CrDrive_Continuous_Bracket_10Ev_2pics_Plus";
        break;
    case SDK::CrDriveMode::CrDrive_Continuous_Bracket_10Ev_2pics_Minus:
        ts << "CrDrive_Continuous_Bracket_10Ev_2pics_Minus";
        break;
    case SDK::CrDriveMode::CrDrive_Continuous_Bracket_10Ev_7pics:
        ts << "CrDrive_Continuous_Bracket_10Ev_7pics";
        break;
    case SDK::CrDriveMode::CrDrive_Continuous_Bracket_13Ev_2pics_Plus:
        ts << "CrDrive_Continuous_Bracket_13Ev_2pics_Plus";
        break;
    case SDK::CrDriveMode::CrDrive_Continuous_Bracket_13Ev_2pics_Minus:
        ts << "CrDrive_Continuous_Bracket_13Ev_2pics_Minus";
        break;
    case SDK::CrDriveMode::CrDrive_Continuous_Bracket_13Ev_3pics:
        ts << "CrDrive_Continuous_Bracket_13Ev_3pics";
        break;
    case SDK::CrDriveMode::CrDrive_Continuous_Bracket_13Ev_5pics:
        ts << "CrDrive_Continuous_Bracket_13Ev_5pics";
        break;
    case SDK::CrDriveMode::CrDrive_Continuous_Bracket_13Ev_7pics:
        ts << "CrDrive_Continuous_Bracket_13Ev_7pics";
        break;
    case SDK::CrDriveMode::CrDrive_Continuous_Bracket_15Ev_2pics_Plus:
        ts << "CrDrive_Continuous_Bracket_15Ev_2pics_Plus";
        break;
    case SDK::CrDriveMode::CrDrive_Continuous_Bracket_15Ev_2pics_Minus:
        ts << "CrDrive_Continuous_Bracket_15Ev_2pics_Minus";
        break;
    case SDK::CrDriveMode::CrDrive_Continuous_Bracket_15Ev_3pics:
        ts << "CrDrive_Continuous_Bracket_15Ev_3pics";
        break;
    case SDK::CrDriveMode::CrDrive_Continuous_Bracket_15Ev_5pics:
        ts << "CrDrive_Continuous_Bracket_15Ev_5pics";
        break;
    case SDK::CrDriveMode::CrDrive_Continuous_Bracket_15Ev_7pics:
        ts << "CrDrive_Continuous_Bracket_15Ev_7pics";
        break;
    case SDK::CrDriveMode::CrDrive_Continuous_Bracket_17Ev_2pics_Plus:
        ts << "CrDrive_Continuous_Bracket_17Ev_2pics_Plus";
        break;
    case SDK::CrDriveMode::CrDrive_Continuous_Bracket_17Ev_2pics_Minus:
        ts << "CrDrive_Continuous_Bracket_17Ev_2pics_Minus";
        break;
    case SDK::CrDriveMode::CrDrive_Continuous_Bracket_17Ev_3pics:
        ts << "CrDrive_Continuous_Bracket_17Ev_3pics";
        break;
    case SDK::CrDriveMode::CrDrive_Continuous_Bracket_17Ev_5pics:
        ts << "CrDrive_Continuous_Bracket_17Ev_5pics";
        break;
    case SDK::CrDriveMode::CrDrive_Continuous_Bracket_17Ev_7pics:
        ts << "CrDrive_Continuous_Bracket_17Ev_7pics";
        break;
    case SDK::CrDriveMode::CrDrive_Continuous_Bracket_20Ev_2pics_Plus:
        ts << "CrDrive_Continuous_Bracket_20Ev_2pics_Plus";
        break;
    case SDK::CrDriveMode::CrDrive_Continuous_Bracket_20Ev_2pics_Minus:
        ts << "CrDrive_Continuous_Bracket_20Ev_2pics_Minus";
        break;
    case SDK::CrDriveMode::CrDrive_Continuous_Bracket_20Ev_7pics:
        ts << "CrDrive_Continuous_Bracket_20Ev_7pics";
        break;
    case SDK::CrDriveMode::CrDrive_Continuous_Bracket_23Ev_2pics_Plus:
        ts << "CrDrive_Continuous_Bracket_23Ev_2pics_Plus";
        break;
    case SDK::CrDriveMode::CrDrive_Continuous_Bracket_23Ev_2pics_Minus:
        ts << "CrDrive_Continuous_Bracket_23Ev_2pics_Minus";
        break;
    case SDK::CrDriveMode::CrDrive_Continuous_Bracket_23Ev_3pics:
        ts << "CrDrive_Continuous_Bracket_23Ev_3pics";
        break;
    case SDK::CrDriveMode::CrDrive_Continuous_Bracket_23Ev_5pics:
        ts << "CrDrive_Continuous_Bracket_23Ev_5pics";
        break;
    case SDK::CrDriveMode::CrDrive_Continuous_Bracket_25Ev_2pics_Plus:
        ts << "CrDrive_Continuous_Bracket_25Ev_2pics_Plus";
        break;
    case SDK::CrDriveMode::CrDrive_Continuous_Bracket_25Ev_2pics_Minus:
        ts << "CrDrive_Continuous_Bracket_25Ev_2pics_Minus";
        break;
    case SDK::CrDriveMode::CrDrive_Continuous_Bracket_25Ev_3pics:
        ts << "CrDrive_Continuous_Bracket_25Ev_3pics";
        break;
    case SDK::CrDriveMode::CrDrive_Continuous_Bracket_25Ev_5pics:
        ts << "CrDrive_Continuous_Bracket_25Ev_5pics";
        break;
    case SDK::CrDriveMode::CrDrive_Continuous_Bracket_27Ev_2pics_Plus:
        ts << "CrDrive_Continuous_Bracket_27Ev_2pics_Plus";
        break;
    case SDK::CrDriveMode::CrDrive_Continuous_Bracket_27Ev_2pics_Minus:
        ts << "CrDrive_Continuous_Bracket_27Ev_2pics_Minus";
        break;
    case SDK::CrDriveMode::CrDrive_Continuous_Bracket_27Ev_3pics:
        ts << "CrDrive_Continuous_Bracket_27Ev_3pics";
        break;
    case SDK::CrDriveMode::CrDrive_Continuous_Bracket_27Ev_5pics:
        ts << "CrDrive_Continuous_Bracket_27Ev_5pics";
        break;
    case SDK::CrDriveMode::CrDrive_Continuous_Bracket_30Ev_2pics_Plus:
        ts << "CrDrive_Continuous_Bracket_30Ev_2pics_Plus";
        break;
    case SDK::CrDriveMode::CrDrive_Continuous_Bracket_30Ev_2pics_Minus:
        ts << "CrDrive_Continuous_Bracket_30Ev_2pics_Minus";
        break;
    case SDK::CrDriveMode::CrDrive_Single_Bracket_03Ev_3pics:
        ts << "CrDrive_Single_Bracket_03Ev_3pics";
        break;
    case SDK::CrDriveMode::CrDrive_Single_Bracket_03Ev_5pics:
        ts << "CrDrive_Single_Bracket_03Ev_5pics";
        break;
    case SDK::CrDriveMode::CrDrive_Single_Bracket_03Ev_9pics:
        ts << "CrDrive_Single_Bracket_03Ev_9pics";
        break;
    case SDK::CrDriveMode::CrDrive_Single_Bracket_05Ev_3pics:
        ts << "CrDrive_Single_Bracket_05Ev_3pics";
        break;
    case SDK::CrDriveMode::CrDrive_Single_Bracket_05Ev_5pics:
        ts << "CrDrive_Single_Bracket_05Ev_5pics";
        break;
    case SDK::CrDriveMode::CrDrive_Single_Bracket_05Ev_9pics:
        ts << "CrDrive_Single_Bracket_05Ev_9pics";
        break;
    case SDK::CrDriveMode::CrDrive_Single_Bracket_07Ev_3pics:
        ts << "CrDrive_Single_Bracket_07Ev_3pics";
        break;
    case SDK::CrDriveMode::CrDrive_Single_Bracket_07Ev_5pics:
        ts << "CrDrive_Single_Bracket_07Ev_5pics";
        break;
    case SDK::CrDriveMode::CrDrive_Single_Bracket_07Ev_9pics:
        ts << "CrDrive_Single_Bracket_07Ev_9pics";
        break;
    case SDK::CrDriveMode::CrDrive_Single_Bracket_10Ev_3pics:
        ts << "CrDrive_Single_Bracket_10Ev_3pics";
        break;
    case SDK::CrDriveMode::CrDrive_Single_Bracket_10Ev_5pics:
        ts << "CrDrive_Single_Bracket_10Ev_5pics";
        break;
    case SDK::CrDriveMode::CrDrive_Single_Bracket_10Ev_9pics:
        ts << "CrDrive_Single_Bracket_10Ev_9pics";
        break;
    case SDK::CrDriveMode::CrDrive_Single_Bracket_20Ev_3pics:
        ts << "CrDrive_Single_Bracket_20Ev_3pics";
        break;
    case SDK::CrDriveMode::CrDrive_Single_Bracket_20Ev_5pics:
        ts << "CrDrive_Single_Bracket_20Ev_5pics";
        break;
    case SDK::CrDriveMode::CrDrive_Single_Bracket_30Ev_3pics:
        ts << "CrDrive_Single_Bracket_30Ev_3pics";
        break;
    case SDK::CrDriveMode::CrDrive_Single_Bracket_30Ev_5pics:
        ts << "CrDrive_Single_Bracket_30Ev_5pics";
        break;
    case SDK::CrDriveMode::CrDrive_Single_Bracket_03Ev_2pics_Plus:
        ts << "CrDrive_Single_Bracket_03Ev_2pics_Plus";
        break;
    case SDK::CrDriveMode::CrDrive_Single_Bracket_03Ev_2pics_Minus:
        ts << "CrDrive_Single_Bracket_03Ev_2pics_Minus";
        break;
    case SDK::CrDriveMode::CrDrive_Single_Bracket_03Ev_7pics:
        ts << "CrDrive_Single_Bracket_03Ev_7pics";
        break;
    case SDK::CrDriveMode::CrDrive_Single_Bracket_05Ev_2pics_Plus:
        ts << "CrDrive_Single_Bracket_05Ev_2pics_Plus";
        break;
    case SDK::CrDriveMode::CrDrive_Single_Bracket_05Ev_2pics_Minus:
        ts << "CrDrive_Single_Bracket_05Ev_2pics_Minus";
        break;
    case SDK::CrDriveMode::CrDrive_Single_Bracket_05Ev_7pics:
        ts << "CrDrive_Single_Bracket_05Ev_7pics";
        break;
    case SDK::CrDriveMode::CrDrive_Single_Bracket_07Ev_2pics_Plus:
        ts << "CrDrive_Single_Bracket_07Ev_2pics_Plus";
        break;
    case SDK::CrDriveMode::CrDrive_Single_Bracket_07Ev_2pics_Minus:
        ts << "CrDrive_Single_Bracket_07Ev_2pics_Minus";
        break;
    case SDK::CrDriveMode::CrDrive_Single_Bracket_07Ev_7pics:
        ts << "CrDrive_Single_Bracket_07Ev_7pics";
        break;
    case SDK::CrDriveMode::CrDrive_Single_Bracket_10Ev_2pics_Plus:
        ts << "CrDrive_Single_Bracket_10Ev_2pics_Plus";
        break;
    case SDK::CrDriveMode::CrDrive_Single_Bracket_10Ev_2pics_Minus:
        ts << "CrDrive_Single_Bracket_10Ev_2pics_Minus";
        break;
    case SDK::CrDriveMode::CrDrive_Single_Bracket_10Ev_7pics:
        ts << "CrDrive_Single_Bracket_10Ev_7pics";
        break;
    case SDK::CrDriveMode::CrDrive_Single_Bracket_13Ev_2pics_Plus:
        ts << "CrDrive_Single_Bracket_13Ev_2pics_Plus";
        break;
    case SDK::CrDriveMode::CrDrive_Single_Bracket_13Ev_2pics_Minus:
        ts << "CrDrive_Single_Bracket_13Ev_2pics_Minus";
        break;
    case SDK::CrDriveMode::CrDrive_Single_Bracket_13Ev_3pics:
        ts << "CrDrive_Single_Bracket_13Ev_3pics";
        break;
    case SDK::CrDriveMode::CrDrive_Single_Bracket_13Ev_5pics:
        ts << "CrDrive_Single_Bracket_13Ev_5pics";
        break;
    case SDK::CrDriveMode::CrDrive_Single_Bracket_13Ev_7pics:
        ts << "CrDrive_Single_Bracket_13Ev_7pics";
        break;
    case SDK::CrDriveMode::CrDrive_Single_Bracket_15Ev_2pics_Plus:
        ts << "CrDrive_Single_Bracket_15Ev_2pics_Plus";
        break;
    case SDK::CrDriveMode::CrDrive_Single_Bracket_15Ev_2pics_Minus:
        ts << "CrDrive_Single_Bracket_15Ev_2pics_Minus";
        break;
    case SDK::CrDriveMode::CrDrive_Single_Bracket_15Ev_3pics:
        ts << "CrDrive_Single_Bracket_15Ev_3pics";
        break;
    case SDK::CrDriveMode::CrDrive_Single_Bracket_15Ev_5pics:
        ts << "CrDrive_Single_Bracket_15Ev_5pics";
        break;
    case SDK::CrDriveMode::CrDrive_Single_Bracket_15Ev_7pics:
        ts << "CrDrive_Single_Bracket_15Ev_7pics";
        break;
    case SDK::CrDriveMode::CrDrive_Single_Bracket_17Ev_2pics_Plus:
        ts << "CrDrive_Single_Bracket_17Ev_2pics_Plus";
        break;
    case SDK::CrDriveMode::CrDrive_Single_Bracket_17Ev_2pics_Minus:
        ts << "CrDrive_Single_Bracket_17Ev_2pics_Minus";
        break;
    case SDK::CrDriveMode::CrDrive_Single_Bracket_17Ev_3pics:
        ts << "CrDrive_Single_Bracket_17Ev_3pics";
        break;
    case SDK::CrDriveMode::CrDrive_Single_Bracket_17Ev_5pics:
        ts << "CrDrive_Single_Bracket_17Ev_5pics";
        break;
    case SDK::CrDriveMode::CrDrive_Single_Bracket_17Ev_7pics:
        ts << "CrDrive_Single_Bracket_17Ev_7pics";
        break;
    case SDK::CrDriveMode::CrDrive_Single_Bracket_20Ev_2pics_Plus:
        ts << "CrDrive_Single_Bracket_20Ev_2pics_Plus";
        break;
    case SDK::CrDriveMode::CrDrive_Single_Bracket_20Ev_2pics_Minus:
        ts << "CrDrive_Single_Bracket_20Ev_2pics_Minus";
        break;
    case SDK::CrDriveMode::CrDrive_Single_Bracket_20Ev_7pics:
        ts << "CrDrive_Single_Bracket_20Ev_7pics";
        break;
    case SDK::CrDriveMode::CrDrive_Single_Bracket_23Ev_2pics_Plus:
        ts << "CrDrive_Single_Bracket_23Ev_2pics_Plus";
        break;
    case SDK::CrDriveMode::CrDrive_Single_Bracket_23Ev_2pics_Minus:
        ts << "CrDrive_Single_Bracket_23Ev_2pics_Minus";
        break;
    case SDK::CrDriveMode::CrDrive_Single_Bracket_23Ev_3pics:
        ts << "CrDrive_Single_Bracket_23Ev_3pics";
        break;
    case SDK::CrDriveMode::CrDrive_Single_Bracket_23Ev_5pics:
        ts << "CrDrive_Single_Bracket_23Ev_5pics";
        break;
    case SDK::CrDriveMode::CrDrive_Single_Bracket_25Ev_2pics_Plus:
        ts << "CrDrive_Single_Bracket_25Ev_2pics_Plus";
        break;
    case SDK::CrDriveMode::CrDrive_Single_Bracket_25Ev_2pics_Minus:
        ts << "CrDrive_Single_Bracket_25Ev_2pics_Minus";
        break;
    case SDK::CrDriveMode::CrDrive_Single_Bracket_25Ev_3pics:
        ts << "CrDrive_Single_Bracket_25Ev_3pics";
        break;
    case SDK::CrDriveMode::CrDrive_Single_Bracket_25Ev_5pics:
        ts << "CrDrive_Single_Bracket_25Ev_5pics";
        break;
    case SDK::CrDriveMode::CrDrive_Single_Bracket_27Ev_2pics_Plus:
        ts << "CrDrive_Single_Bracket_27Ev_2pics_Plus";
        break;
    case SDK::CrDriveMode::CrDrive_Single_Bracket_27Ev_2pics_Minus:
        ts << "CrDrive_Single_Bracket_27Ev_2pics_Minus";
        break;
    case SDK::CrDriveMode::CrDrive_Single_Bracket_27Ev_3pics:
        ts << "CrDrive_Single_Bracket_27Ev_3pics";
        break;
    case SDK::CrDriveMode::CrDrive_Single_Bracket_27Ev_5pics:
        ts << "CrDrive_Single_Bracket_27Ev_5pics";
        break;
    case SDK::CrDriveMode::CrDrive_Single_Bracket_30Ev_2pics_Plus:
        ts << "CrDrive_Single_Bracket_30Ev_2pics_Plus";
        break;
    case SDK::CrDriveMode::CrDrive_Single_Bracket_30Ev_2pics_Minus:
        ts << "CrDrive_Single_Bracket_30Ev_2pics_Minus";
        break;
    case SDK::CrDriveMode::CrDrive_WB_Bracket_Lo:
        ts << "CrDrive_WB_Bracket_Lo";
        break;
    case SDK::CrDriveMode::CrDrive_WB_Bracket_Hi:
        ts << "CrDrive_WB_Bracket_Hi";
        break;
    case SDK::CrDriveMode::CrDrive_DRO_Bracket_Lo:
        ts << "CrDrive_DRO_Bracket_Lo";
        break;
    case SDK::CrDriveMode::CrDrive_DRO_Bracket_Hi:
        ts << "CrDrive_DRO_Bracket_Hi";
        break;
    case SDK::CrDriveMode::CrDrive_Continuous_Timer_3pics:
        ts << "CrDrive_Continuous_Timer_3pics";
        break;
    case SDK::CrDriveMode::CrDrive_Continuous_Timer_5pics:
        ts << "CrDrive_Continuous_Timer_5pics";
        break;
    case SDK::CrDriveMode::CrDrive_Continuous_Timer_2s_3pics:
        ts << "CrDrive_Continuous_Timer_2s_3pics";
        break;
    case SDK::CrDriveMode::CrDrive_Continuous_Timer_2s_5pics:
        ts << "CrDrive_Continuous_Timer_2s_5pics";
        break;
    case SDK::CrDriveMode::CrDrive_Continuous_Timer_5s_3pics:
        ts << "CrDrive_Continuous_Timer_5s_3pics";
        break;
    case SDK::CrDriveMode::CrDrive_Continuous_Timer_5s_5pics:
        ts << "CrDrive_Continuous_Timer_5s_5pics";
        break;
    case SDK::CrDriveMode::CrDrive_LPF_Bracket:
        ts << "CrDrive_LPF_Bracket";
        break;
    case SDK::CrDriveMode::CrDrive_RemoteCommander:
        ts << "CrDrive_RemoteCommander";
        break;
    case SDK::CrDriveMode::CrDrive_MirrorUp:
        ts << "CrDrive_MirrorUp";
        break;
    case SDK::CrDriveMode::CrDrive_SelfPortrait_1:
        ts << "CrDrive_SelfPortrait_1";
        break;
    case SDK::CrDriveMode::CrDrive_SelfPortrait_2:
        ts << "CrDrive_SelfPortrait_2";
        break;
    case SDK::CrDriveMode::CrDrive_FocusBracket:
        ts << "CrDrive_FocusBracket";
        break;
    default:
        break;
    }
    return ts.str();
}

text format_focus_mode(std::uint16_t focus_mode)
{
    text_stringstream ts;

    switch (focus_mode) {
    case SDK::CrFocusMode::CrFocus_MF:
        ts << "MF";
        break;
    case SDK::CrFocusMode::CrFocus_AF_S:
        ts << "AF_S";
        break;
    case SDK::CrFocusMode::CrFocus_AF_C:
        ts << "AF_C";
        break;
    case SDK::CrFocusMode::CrFocus_AF_A:
        ts << "AF_A";
        break;
    case SDK::CrFocusMode::CrFocus_DMF:
        ts << "DMF";
        break;
    case SDK::CrFocusMode::CrFocus_AF_D:
        ts << "AF_D";
        break;
    case SDK::CrFocusMode::CrFocus_PF:
        ts << "PF";
        break;
    default:
        break;
    }

    return ts.str();
}

text format_focus_area(std::uint16_t focus_area)
{
    text_stringstream ts;

    switch (focus_area) {
    case SDK::CrFocusArea::CrFocusArea_Unknown:
        ts << "Unknown";
        break;
    case SDK::CrFocusArea::CrFocusArea_Wide:
        ts << "Wide";
        break;
    case SDK::CrFocusArea::CrFocusArea_Zone:
        ts << "Zone";
        break;
    case SDK::CrFocusArea::CrFocusArea_Center:
        ts << "Center";
        break;
    case SDK::CrFocusArea::CrFocusArea_Flexible_Spot_S:
        ts << "Flexible Spot S";
        break;
    case SDK::CrFocusArea::CrFocusArea_Flexible_Spot_M:
        ts << "Flexible Spot M";
        break;
    case SDK::CrFocusArea::CrFocusArea_Flexible_Spot_L:
        ts << "Flexible Spot L";
        break;
    case SDK::CrFocusArea::CrFocusArea_Expand_Flexible_Spot:
        ts << "Expand Flexible Spot";
        break;
    case SDK::CrFocusArea::CrFocusArea_Flexible_Spot:
        ts << "Flexible Spot";
        break;
    case SDK::CrFocusArea::CrFocusArea_Tracking_Wide:
        ts << "Tracking Wide";
        break;
    case SDK::CrFocusArea::CrFocusArea_Tracking_Zone:
        ts << "Tracking Zone";
        break;
    case SDK::CrFocusArea::CrFocusArea_Tracking_Center:
        ts << "Tracking Center";
        break;
    case SDK::CrFocusArea::CrFocusArea_Tracking_Flexible_Spot_S:
        ts << "Tracking Flexible Spot S";
        break;
    case SDK::CrFocusArea::CrFocusArea_Tracking_Flexible_Spot_M:
        ts << "Tracking Flexible Spot M";
        break;
    case SDK::CrFocusArea::CrFocusArea_Tracking_Flexible_Spot_L:
        ts << "Tracking Flexible Spot L";
        break;
    case SDK::CrFocusArea::CrFocusArea_Tracking_Expand_Flexible_Spot:
        ts << "Tracking Expand Flexible Spot";
        break;
    case SDK::CrFocusArea::CrFocusArea_Tracking_Flexible_Spot:
        ts << "Tracking Flexible Spot";
        break;
    default:
        break;
    }

    return ts.str();
}

text format_live_view_image_quality(std::uint16_t live_view_image_quality)
{
    text_stringstream ts;

    switch (live_view_image_quality) {
    case SDK::CrPropertyLiveViewImageQuality::CrPropertyLiveViewImageQuality_High:
        ts << "High";
        break;
    case SDK::CrPropertyLiveViewImageQuality::CrPropertyLiveViewImageQuality_Low:
        ts << "Low";
        break;
    default:
        break;
    }

    return ts.str();
}

text format_media_slotx_format_enable_status(std::uint8_t media_slotx_format_enable_status)
{
    text_stringstream ts;

    switch (media_slotx_format_enable_status) {
    case SDK::CrMediaFormat::CrMediaFormat_Disable:
        ts << "Disable";
        break;
    case SDK::CrMediaFormat::CrMediaFormat_Enable:
        ts << "Enabled";
        break;
    default:
        break;
    }

    return ts.str();
}
text format_white_balance(std::uint16_t value)
{
    text_stringstream ts;

    switch (value) {
    case SDK::CrWhiteBalanceSetting::CrWhiteBalance_AWB:
        ts << "AWB";
        break;
    case SDK::CrWhiteBalanceSetting::CrWhiteBalance_Underwater_Auto:
        ts << "Underwater_Auto";
        break;
    case SDK::CrWhiteBalanceSetting::CrWhiteBalance_Daylight:
        ts << "Daylight";
        break;
    case SDK::CrWhiteBalanceSetting::CrWhiteBalance_Shadow:
        ts << "Shadow";
        break;
    case SDK::CrWhiteBalanceSetting::CrWhiteBalance_Cloudy:
        ts << "Cloudy";
        break;
    case SDK::CrWhiteBalanceSetting::CrWhiteBalance_Tungsten:
        ts << "Tungsten";
        break;
    case SDK::CrWhiteBalanceSetting::CrWhiteBalance_Fluorescent:
        ts << "Fluorescent";
        break;
    case SDK::CrWhiteBalanceSetting::CrWhiteBalance_Fluorescent_WarmWhite:
        ts << "Fluorescent_WarmWhite";
        break;
    case SDK::CrWhiteBalanceSetting::CrWhiteBalance_Fluorescent_CoolWhite:
        ts << "Fluorescent_CoolWhite";
        break;
    case SDK::CrWhiteBalanceSetting::CrWhiteBalance_Fluorescent_DayWhite:
        ts << "Fluorescent_DayWhite";
        break;
    case SDK::CrWhiteBalanceSetting::CrWhiteBalance_Fluorescent_Daylight:
        ts << "Fluorescent_Daylight";
        break;
    case SDK::CrWhiteBalanceSetting::CrWhiteBalance_Flush:
        ts << "Flush";
        break;
    case SDK::CrWhiteBalanceSetting::CrWhiteBalance_ColorTemp:
        ts << "ColorTemp";
        break;
    case SDK::CrWhiteBalanceSetting::CrWhiteBalance_Custom_1:
        ts << "Custom_1";
        break;
    case SDK::CrWhiteBalanceSetting::CrWhiteBalance_Custom_2:
        ts << "Custom_2";
        break;
    case SDK::CrWhiteBalanceSetting::CrWhiteBalance_Custom_3:
        ts << "Custom_3";
        break;
    case SDK::CrWhiteBalanceSetting::CrWhiteBalance_Custom:
        ts << "Custom";
        break;
    default:
        break;
    }

    return ts.str();
}

text format_customwb_capture_standby(std::uint16_t customwb_capture_standby)
{
    text_stringstream ts;

    switch (customwb_capture_standby) {
    case SDK::CrPropertyCustomWBOperation::CrPropertyCustomWBOperation_Disable:
        ts << "Disable";
        break;
    case SDK::CrPropertyCustomWBOperation::CrPropertyCustomWBOperation_Enable:
        ts << "Enable";
        break;
    default:
        break;
    }

    return ts.str();
}

text format_customwb_capture_standby_cancel(std::uint16_t customwb_capture_standby_cancel)
{
    text_stringstream ts;

    switch (customwb_capture_standby_cancel) {
    case SDK::CrPropertyCustomWBOperation::CrPropertyCustomWBOperation_Disable:
        ts << "Disable";
        break;
    case SDK::CrPropertyCustomWBOperation::CrPropertyCustomWBOperation_Enable:
        ts << "Enable";
        break;

    default:
        break;
    }

    return ts.str();
}

text format_customwb_capture_operation(std::uint16_t customwb_capture_operation)
{
    text_stringstream ts;

    switch (customwb_capture_operation) {
    case SDK::CrPropertyCustomWBOperation::CrPropertyCustomWBOperation_Disable:
        ts << "Disable";
        break;
    case SDK::CrPropertyCustomWBOperation::CrPropertyCustomWBOperation_Enable:
        ts << "Enabled";
        break;
    default:
        break;
    }

    return ts.str();
}

text format_customwb_capture_execution_state(std::uint16_t customwb_capture_execution_state)
{
    text_stringstream ts;

    switch (customwb_capture_execution_state) {
    case SDK::CrPropertyCustomWBExecutionState::CrPropertyCustomWBExecutionState_Invalid:
        ts << "Invalid";
        break;
    case SDK::CrPropertyCustomWBExecutionState::CrPropertyCustomWBExecutionState_Standby:
        ts << "Standby";
        break;
    case SDK::CrPropertyCustomWBExecutionState::CrPropertyCustomWBExecutionState_Capturing:
        ts << "Capturing";
        break;
    case SDK::CrPropertyCustomWBExecutionState::CrPropertyCustomWBExecutionState_OperatingCamera:
        ts << "OperatingCamera";
        break;
    default:
        break;
    }

    return ts.str();
}

text format_zoom_operation_status(std::uint8_t zoom_operation_status)
{
    text_stringstream ts;

    switch (zoom_operation_status) {
    case SDK::CrZoomOperationEnableStatus::CrZoomOperationEnableStatus_Disable:
        ts << "Disable";
        break;
    case SDK::CrZoomOperationEnableStatus::CrZoomOperationEnableStatus_Enable:
        ts << "Enable";
        break;
    default:
        break;
    }

    return ts.str();
}

text format_zoom_setting_type(std::uint8_t zoom_setting_type)
{
    text_stringstream ts;

    switch (zoom_setting_type) {
    case SDK::CrZoomSettingType::CrZoomSetting_OpticalZoomOnly:
        ts << "OpticalZoom";
        break;
    case SDK::CrZoomSettingType::CrZoomSetting_SmartZoomOnly:
        ts << "SmartZoom";
        break;
    case SDK::CrZoomSettingType::CrZoomSetting_On_ClearImageZoom:
        ts << "ClearImageZoom";
        break;
    case SDK::CrZoomSettingType::CrZoomSetting_On_DigitalZoom:
        ts << "DigitalZoom";
        break;
    default:
        break;
    }

    return ts.str();
}

text format_zoom_types_status(std::uint8_t zoom_types_status)
{
    text_stringstream ts;

    switch (zoom_types_status) {
    case SDK::CrZoomTypeStatus::CrZoomTypeStatus_OpticalZoom:
        ts << "OpticalZoom";
        break;
    case SDK::CrZoomTypeStatus::CrZoomTypeStatus_SmartZoom:
        ts << "SmartZoom";
        break;
    case SDK::CrZoomTypeStatus::CrZoomTypeStatus_ClearImageZoom:
        ts << "ClearImageZoom";
        break;
    case SDK::CrZoomTypeStatus::CrZoomTypeStatus_DigitalZoom:
        ts << "DigitalZoom";
        break;
    default:
        break;
    }

    return ts.str();
}

text format_remocon_zoom_speed_type(std::uint8_t remocon_zoom_speed_type)
{
    text_stringstream ts;

    switch (remocon_zoom_speed_type) {
    case SDK::CrRemoconZoomSpeedType::CrRemoconZoomSpeedType_Invalid:
        ts << "Invalid";
        break;
    case SDK::CrRemoconZoomSpeedType::CrRemoconZoomSpeedType_Variable:
        ts << "Variable";
        break;
    case SDK::CrRemoconZoomSpeedType::CrRemoconZoomSpeedType_Fixed:
        ts << "Fixed";
        break;
    default:
        break;
    }

    return ts.str();
}

text format_camera_setting_save_operation(std::uint16_t camera_setting_save_operation)
{
    text_stringstream ts;

    switch (camera_setting_save_operation) {
    case SDK::CrCameraSettingSaveOperation::CrCameraSettingSaveOperation_Disable:
        ts << "Disable";
        break;
    case SDK::CrCameraSettingSaveOperation::CrCameraSettingSaveOperation_Enable:
        ts << "Enabled";
        break;
    default:
        break;
    }
    return ts.str();
}

text format_camera_setting_read_operation(std::uint16_t camera_setting_read_operation)
{
    text_stringstream ts;

    switch (camera_setting_read_operation) {
    case SDK::CrCameraSettingReadOperation::CrCameraSettingReadOperation_Disable:
        ts << "Disable";
        break;
    case SDK::CrCameraSettingReadOperation::CrCameraSettingReadOperation_Enable:
        ts << "Enabled";
        break;
    default:
        break;
    }
    return ts.str();
}

text format_camera_setting_save_read_state(std::uint8_t camera_setting_save_read_state)
{
    text_stringstream ts;

    switch (camera_setting_save_read_state) {
    case SDK::CrCameraSettingSaveReadState::CrCameraSettingSaveReadState_Idle:
        ts << "Idle";
        break;
    case SDK::CrCameraSettingSaveReadState::CrCameraSettingSaveReadState_Reading:
        ts << "Reading";
        break;
    default:
        break;
    }

    return ts.str();
}

text format_aps_c_or_full_switching_setting(std::uint8_t aps_c_or_full_switching_setting)
{
    text_stringstream ts;

    switch (aps_c_or_full_switching_setting) {
    case SDK::CrAPS_C_or_Full_SwitchingSetting::CrAPS_C_or_Full_SwitchingSetting_Full:
        ts << "Full";
        break;
    case SDK::CrAPS_C_or_Full_SwitchingSetting::CrAPS_C_or_Full_SwitchingSetting_APS_C:
        ts << "APS-C";
        break;
    default:
        break;
    }
    return ts.str();
}

text format_aps_c_or_full_switching_enable_status(std::uint8_t aps_c_or_full_switching_enable_status)
{
    text_stringstream ts;

    switch (aps_c_or_full_switching_enable_status) {
    case SDK::CrAPS_C_or_Full_SwitchingEnableStatus::CrAPS_C_or_Full_Switching_Disable:
        ts << "Disable";
        break;
    case SDK::CrAPS_C_or_Full_SwitchingEnableStatus::CrAPS_C_or_Full_Switching_Enable:
        ts << "Enable";
        break;
    default:
        break;
    }
    return ts.str();
}

text format_playback_media(std::uint8_t playback_media)
{
    text_stringstream ts;

    switch (playback_media) {
    case SDK::CrPlaybackMedia::CrPlaybackMedia_Slot1:
        ts << "Slot1";
        break;
    case SDK::CrPlaybackMedia::CrPlaybackMedia_Slot2:
        ts << "Slot2";
        break;
    default:
        break;
    }

    return ts.str();
}

text format_camera_setting_reset_enable_status(std::uint8_t camera_setting_reset_enable_status)
{
    text_stringstream ts;

    switch (camera_setting_reset_enable_status) {
    case SDK::CrCameraSettingsResetEnableStatus::CrCameraSettingsReset_Disable:
        ts << "Disable";
        break;
    case SDK::CrCameraSettingsResetEnableStatus::CrCameraSettingsReset_Enable:
        ts << "Enable";
        break;
    default:
        break;
    }
    return ts.str();
}

text format_gain_base_sensitivity(std::uint8_t gain_base)
{
    text_stringstream ts;

    switch (gain_base) {
    case SDK::CrGainBaseSensitivity::CrGainBaseSensitivity_High:
        ts << "High Level";
        break;
    case SDK::CrGainBaseSensitivity::CrGainBaseSensitivity_Low:
        ts << "Low Level";
        break;
    default:
        break;
    }

    return ts.str();
}
text format_gain_base_iso_sensitivity(std::uint8_t gain_base_iso)
{
    text_stringstream ts;

    switch (gain_base_iso) {
    case SDK::CrGainBaseIsoSensitivity::CrGainBaseIsoSensitivity_High:
        ts << "High Level";
        break;
    case SDK::CrGainBaseIsoSensitivity::CrGainBaseIsoSensitivity_Low:
        ts << "Low Level";
        break;
    default:
        break;
    }

    return ts.str();
}

text format_monitor_lut_setting(std::uint8_t monitor_setup)
{
    text_stringstream ts;

    switch (monitor_setup) {
    case SDK::CrMonitorLUTSetting::CrMonitorLUT_OFF:
        ts << "OFF";
        break;
    case SDK::CrMonitorLUTSetting::CrMonitorLUT_ON:
        ts << "ON";
        break;
    default:
        break;
    }
    return ts.str();
}

text format_baselook_value(std::uint8_t baselook_value)
{
    text_stringstream ts;

    switch (baselook_value) {
    case SDK::CrBaseLookValueSetter::CrBaseLookValue_Preset:
        ts << "(Preset)";
        break;
    case SDK::CrBaseLookValueSetter::CrBaseLookValue_User:
        ts << "(User)";
        break;
    default:
        break;
    }
    return ts.str();
}

text format_shutter_mode_setting(std::uint8_t shutter_mode_setting)
{
    text_stringstream ts;

    switch (shutter_mode_setting) {
    case SDK::CrShutterModeSetting::CrShutterMode_Automatic:
        ts << "Automatic";
        break;
    case SDK::CrShutterModeSetting::CrShutterMode_Manual:
        ts << "Manual";
        break;
    default:
        break;
    }

    return ts.str();
}

text format_iris_mode_setting(std::uint8_t iris_mode_setting)
{
    text_stringstream ts;

    switch (iris_mode_setting) {
    case SDK::CrIrisModeSetting::CrIrisMode_Automatic:
        ts << "Automatic";
        break;
    case SDK::CrIrisModeSetting::CrIrisMode_Manual:
        ts << "Manual";
        break;
    default:
        break;
    }

    return ts.str();
}

text format_exposure_control_type(std::uint8_t exposure_control_type)
{
    text_stringstream ts;

    switch (exposure_control_type) {
    case SDK::CrExposureCtrlType::CrExposureCtrlType_PASMMode:
        ts << "P/A/S/M Mode";
        break;
    case SDK::CrExposureCtrlType::CrExposureCtrlType_FlexibleExposureMode:
        ts << "Flexible Exposure Mode";
        break;
    default:
        break;
    }

    return ts.str();
}

text format_gain_control_setting(std::uint8_t gain_control_setting)
{
    text_stringstream ts;

    switch (gain_control_setting) {
    case SDK::CrGainControlSetting::CrGainControl_Automatic:
        ts << "Automatic";
        break;
    case SDK::CrGainControlSetting::CrGainControl_Manual:
        ts << "Manual";
        break;
    default:
        break;
    }

    return ts.str();
}

text format_recording_setting(std::uint16_t recording_setting)
{
    //SDK::CrRecordingSettingMovie
    text_stringstream ts;
    switch (recording_setting)
    {
    case SDK::CrRecordingSettingMovie::CrRecordingSettingMovie_60p_50M:
        ts << "60p 50M / XAVC S";
        break;
    case SDK::CrRecordingSettingMovie::CrRecordingSettingMovie_30p_50M:
        ts << "30p 50M / XAVC S";
        break;
    case SDK::CrRecordingSettingMovie::CrRecordingSettingMovie_24p_50M:
        ts << "24p 50M / XAVC S";
        break;
    case SDK::CrRecordingSettingMovie::CrRecordingSettingMovie_50p_50M:
        ts << "50p 50M / XAVC S";
        break;
    case SDK::CrRecordingSettingMovie::CrRecordingSettingMovie_25p_50M:
        ts << "25p 50M / XAVC S";
        break;
    case SDK::CrRecordingSettingMovie::CrRecordingSettingMovie_60i_24M:
        ts << "60i 24M(FX) / AVCHD";
        break;
    case SDK::CrRecordingSettingMovie::CrRecordingSettingMovie_50i_24M_FX:
        ts << "50i 24M(FX) / AVCHD";
        break;
    case SDK::CrRecordingSettingMovie::CrRecordingSettingMovie_60i_17M_FH:
        ts << "60i 17M(FH) / AVCHD";
        break;
    case SDK::CrRecordingSettingMovie::CrRecordingSettingMovie_50i_17M_FH:
        ts << "50i 17M(FH) / AVCHD";
        break;
    case SDK::CrRecordingSettingMovie::CrRecordingSettingMovie_60p_28M_PS:
        ts << "60p 28M(PS) / AVCHD";
        break;
    case SDK::CrRecordingSettingMovie::CrRecordingSettingMovie_50p_28M_PS:
        ts << "50p 28M(PS) / AVCHD";
        break;
    case SDK::CrRecordingSettingMovie::CrRecordingSettingMovie_24p_24M_FX:
        ts << "24p 24M(FX) / AVCHD";
        break;
    case SDK::CrRecordingSettingMovie::CrRecordingSettingMovie_25p_24M_FX:
        ts << "25p 24M(FX) / AVCHD";
        break;
    case SDK::CrRecordingSettingMovie::CrRecordingSettingMovie_24p_17M_FH:
        ts << "24p 17M(FH) / AVCHD";
        break;
    case SDK::CrRecordingSettingMovie::CrRecordingSettingMovie_25p_17M_FH:
        ts << "25p 17M(FH) / AVCHD";
        break;
    case SDK::CrRecordingSettingMovie::CrRecordingSettingMovie_120p_50M_1280x720:
        ts << "120p 50M (1280x720) / XAVC S";
        break;
    case SDK::CrRecordingSettingMovie::CrRecordingSettingMovie_100p_50M_1280x720:
        ts << "100p 50M (1280x720) / XAVC S";
        break;
    case SDK::CrRecordingSettingMovie::CrRecordingSettingMovie_1920x1080_30p_16M:
        ts << "1920x1080 30p 16M / MP4";
        break;
    case SDK::CrRecordingSettingMovie::CrRecordingSettingMovie_1920x1080_25p_16M:
        ts << "1920x1080 25p 16M / MP4";
        break;
    case SDK::CrRecordingSettingMovie::CrRecordingSettingMovie_1280x720_30p_6M:
        ts << "1280x720 30p 6M / MP4";
        break;
    case SDK::CrRecordingSettingMovie::CrRecordingSettingMovie_1280x720_25p_6M:
        ts << "1280x720 25p 6M / MP4";
        break;
    case SDK::CrRecordingSettingMovie::CrRecordingSettingMovie_1920x1080_60p_28M:
        ts << "1920x1080 60p 28M / MP4";
        break;
    case SDK::CrRecordingSettingMovie::CrRecordingSettingMovie_1920x1080_50p_28M:
        ts << "1920x1080 50p 28M / MP4";
        break;
    case SDK::CrRecordingSettingMovie::CrRecordingSettingMovie_60p_25M_XAVC_S_HD:
        ts << "60p 25M / XAVC S HD";
        break;
    case SDK::CrRecordingSettingMovie::CrRecordingSettingMovie_50p_25M_XAVC_S_HD:
        ts << "50p 25M / XAVC S HD";
        break;
    case SDK::CrRecordingSettingMovie::CrRecordingSettingMovie_30p_16M_XAVC_S_HD:
        ts << "30p 16M / XAVC S HD";
        break;
    case SDK::CrRecordingSettingMovie::CrRecordingSettingMovie_25p_16M_XAVC_S_HD:
        ts << "25p 16M / XAVC S HD";
        break;
    case SDK::CrRecordingSettingMovie::CrRecordingSettingMovie_120p_100M_1920x1080_XAVC_S_HD:
        ts << "120p 100M (1920x1080)  / XAVC S HD";
        break;
    case SDK::CrRecordingSettingMovie::CrRecordingSettingMovie_100p_100M_1920x1080_XAVC_S_HD:
        ts << "100p 100M (1920x1080)  / XAVC S HD";
        break;
    case SDK::CrRecordingSettingMovie::CrRecordingSettingMovie_120p_60M_1920x1080_XAVC_S_HD:
        ts << "120p 60M (1920x1080) / XAVC S HD";
        break;
    case SDK::CrRecordingSettingMovie::CrRecordingSettingMovie_100p_60M_1920x1080_XAVC_S_HD:
        ts << "100p 60M (1920x1080) / XAVC S HD";
        break;
    case SDK::CrRecordingSettingMovie::CrRecordingSettingMovie_30p_100M_XAVC_S_4K:
        ts << "30p 100M / XAVC S 4K";
        break;
    case SDK::CrRecordingSettingMovie::CrRecordingSettingMovie_25p_100M_XAVC_S_4K:
        ts << "25p 100M / XAVC S 4K";
        break;
    case SDK::CrRecordingSettingMovie::CrRecordingSettingMovie_24p_100M_XAVC_S_4K:
        ts << "24p 100M / XAVC S 4K";
        break;
    case SDK::CrRecordingSettingMovie::CrRecordingSettingMovie_30p_60M_XAVC_S_4K:
        ts << "30p 60M / XAVC S 4K";
        break;
    case SDK::CrRecordingSettingMovie::CrRecordingSettingMovie_25p_60M_XAVC_S_4K:
        ts << "25p 60M / XAVC S 4K";
        break;
    case SDK::CrRecordingSettingMovie::CrRecordingSettingMovie_24p_60M_XAVC_S_4K:
        ts << "24p 60M / XAVC S 4K";
        break;
    case SDK::CrRecordingSettingMovie::CrRecordingSettingMovie_600M_422_10bit:
        ts << "600M 422 10bit";
        break;
    case SDK::CrRecordingSettingMovie::CrRecordingSettingMovie_500M_422_10bit:
        ts << "500M 422 10bit";
        break;
    case SDK::CrRecordingSettingMovie::CrRecordingSettingMovie_400M_420_10bit:
        ts << "400M 420 10bit";
        break;
    case SDK::CrRecordingSettingMovie::CrRecordingSettingMovie_300M_422_10bit:
        ts << "300M 422 10bit";
        break;
    case SDK::CrRecordingSettingMovie::CrRecordingSettingMovie_280M_422_10bit:
        ts << "280M 422 10bit";
        break;
    case SDK::CrRecordingSettingMovie::CrRecordingSettingMovie_250M_422_10bit:
        ts << "250M 422 10bit";
        break;
    case SDK::CrRecordingSettingMovie::CrRecordingSettingMovie_240M_422_10bit:
        ts << "240M 422 10bit";
        break;
    case SDK::CrRecordingSettingMovie::CrRecordingSettingMovie_222M_422_10bit:
        ts << "222M 422 10bit";
        break;
    case SDK::CrRecordingSettingMovie::CrRecordingSettingMovie_200M_422_10bit:
        ts << "200M 422 10bit";
        break;
    case SDK::CrRecordingSettingMovie::CrRecordingSettingMovie_200M_420_10bit:
        ts << "200M 420 10bit";
        break;
    case SDK::CrRecordingSettingMovie::CrRecordingSettingMovie_200M_420_8bit:
        ts << "200M 420 8bit";
        break;
    case SDK::CrRecordingSettingMovie::CrRecordingSettingMovie_185M_422_10bit:
        ts << "185M 422 10bit";
        break;
    case SDK::CrRecordingSettingMovie::CrRecordingSettingMovie_150M_420_10bit:
        ts << "150M 420 10bit";
        break;
    case SDK::CrRecordingSettingMovie::CrRecordingSettingMovie_150M_420_8bit:
        ts << "150M 420 8bit";
        break;
    case SDK::CrRecordingSettingMovie::CrRecordingSettingMovie_140M_422_10bit:
        ts << "140M 422 10bit";
        break;
    case SDK::CrRecordingSettingMovie::CrRecordingSettingMovie_111M_422_10bit:
        ts << "111M 422 10bit";
        break;
    case SDK::CrRecordingSettingMovie::CrRecordingSettingMovie_100M_422_10bit:
        ts << "100M 422 10bit";
        break;
    case SDK::CrRecordingSettingMovie::CrRecordingSettingMovie_100M_420_10bit:
        ts << "100M 420 10bit";
        break;
    case SDK::CrRecordingSettingMovie::CrRecordingSettingMovie_100M_420_8bit:
        ts << "100M 420 8bit";
        break;
    case SDK::CrRecordingSettingMovie::CrRecordingSettingMovie_93M_422_10bit:
        ts << "93M 422 10bit";
        break;
    case SDK::CrRecordingSettingMovie::CrRecordingSettingMovie_89M_422_10bit:
        ts << "89M 422 10bit";
        break;
    case SDK::CrRecordingSettingMovie::CrRecordingSettingMovie_75M_420_10bit:
        ts << "75M 420 10bit";
        break;
    case SDK::CrRecordingSettingMovie::CrRecordingSettingMovie_60M_420_8bit:
        ts << "60M 420 8bit";
        break;
    case SDK::CrRecordingSettingMovie::CrRecordingSettingMovie_50M_422_10bit:
        ts << "50M 422 10bit";
        break;
    case SDK::CrRecordingSettingMovie::CrRecordingSettingMovie_50M_420_10bit:
        ts << "50M 420 10bit";
        break;
    case SDK::CrRecordingSettingMovie::CrRecordingSettingMovie_50M_420_8bit:
        ts << "50M 420 8bit";
        break;
    case SDK::CrRecordingSettingMovie::CrRecordingSettingMovie_45M_420_10bit:
        ts << "45M 420 10bit";
        break;
    case SDK::CrRecordingSettingMovie::CrRecordingSettingMovie_30M_420_10bit:
        ts << "30M 420 10bit";
        break;
    case SDK::CrRecordingSettingMovie::CrRecordingSettingMovie_25M_420_8bit:
        ts << "25M 420 8bit";
        break;
    case SDK::CrRecordingSettingMovie::CrRecordingSettingMovie_16M_420_8bit:
        ts << "16M 420 8bit";
        break;
    case SDK::CrRecordingSettingMovie::CrRecordingSettingMovie_520M_422_10bit:
        ts << "520M 422 10bit";
        break;
    case SDK::CrRecordingSettingMovie::CrRecordingSettingMovie_260M_422_10bit:
        ts << "260M 422 10bit";
        break;
    default:
        break;
    }

    return ts.str();
}

text format_dispmode(std::uint8_t dispmode)
{
    text_stringstream ts;
    switch (dispmode)
    {
    case SDK::CrDispMode::CrDispMode_GraphicDisplay:
        ts << "Graphic Display";
        break;
    case SDK::CrDispMode::CrDispMode_DisplayAllInfo:
        ts << "Display All Information";
        break;
    case SDK::CrDispMode::CrDispMode_NoDispInfo:
        ts << "No Display Information";
        break;
    case SDK::CrDispMode::CrDispMode_Histogram:
        ts << "Histogram";
        break;
    case SDK::CrDispMode::CrDispMode_Level:
        ts << "Level";
        break;
    case SDK::CrDispMode::CrDispMode_ForViewFinder:
        ts << "For viewfinder";
        break;
    case SDK::CrDispMode::CrDispMode_MonitorOff:
        ts << "Monitor Off";
        break;
    default:
        break;
    }
    return ts.str();
}

text format_movie_rec_button_toggle_enable_status(std::uint8_t movie_rec_button_toggle_enable_status)
{
    text_stringstream ts;

    switch (movie_rec_button_toggle_enable_status) {
    case SDK::CrMovieRecButtonToggleEnableStatus::CrMovieRecButtonToggle_Disable:
        ts << "Disable";
        break;
    case SDK::CrMovieRecButtonToggleEnableStatus::CrMovieRecButtonToggle_Enable:
        ts << "Enable";
        break;
    default:
        break;
    }
    return ts.str();
}

text format_shutter_speed_value(std::uint64_t shutter_speed_value)
{
    text_stringstream ts;

    CrInt32u numerator = (CrInt32u)(shutter_speed_value >> 32);
    CrInt32u denominator = (CrInt32u)(shutter_speed_value & 0x0000FFFF);
    ts << numerator << '/' << denominator;
    return ts.str();
}

text format_media_slotx_status(std::uint8_t media_slotx_status)
{
    text_stringstream ts;

    switch (media_slotx_status) {
    case SDK::CrSlotStatus::CrSlotStatus_OK:
        ts << "OK";
        break;
    case SDK::CrSlotStatus::CrSlotStatus_NoCard:
        ts << "No Card";
        break;
    case SDK::CrSlotStatus::CrSlotStatus_CardError:
        ts << "Card Error";
        break;
    case SDK::CrSlotStatus::CrSlotStatus_RecognizingOrLockedError:
        ts << "Recognizing Or LockedError";
        break;
    default:
        break;
    }
    return ts.str();
}

text format_image_stabilization_steady_shot(std::uint8_t image_stabilization_steady_shot)
{
    text_stringstream ts;

    switch (image_stabilization_steady_shot) {
    case SDK::CrImageStabilizationSteadyShot::CrImageStabilizationSteadyShot_Off:
        ts << "OFF";
        break;
    case SDK::CrImageStabilizationSteadyShot::CrImageStabilizationSteadyShot_On:
        ts << "ON";
        break;
    default:
        break;
    }
    return ts.str();
}

text format_movie_image_stabilization_steady_shot(std::uint8_t movie_image_stabilization_steady_shot)
{
    text_stringstream ts;

    switch (movie_image_stabilization_steady_shot) {
    case SDK::CrImageStabilizationSteadyShotMovie::CrImageStabilizationSteadyShotMovie_Off:
        ts << "OFF";
        break;
    case SDK::CrImageStabilizationSteadyShotMovie::CrImageStabilizationSteadyShotMovie_Standard:
        ts << "Standard";
        break;
    case SDK::CrImageStabilizationSteadyShotMovie::CrImageStabilizationSteadyShotMovie_Active:
        ts << "Active";
        break;
    case SDK::CrImageStabilizationSteadyShotMovie::CrImageStabilizationSteadyShotMovie_DynamicActive:
        ts << "Dynamic active";
        break;
    default:
        break;
    }
    return ts.str();
}

text format_silent_mode(std::uint8_t silent_mode)
{
    text_stringstream ts;

    switch (silent_mode) {
    case SDK::CrSilentMode::CrSilentMode_Off:
        ts << "OFF";
        break;
    case SDK::CrSilentMode::CrSilentMode_On:
        ts << "ON";
        break;
    default:
        break;
    }
    return ts.str();
}

text format_silent_mode_aperture_drive_in_af(std::uint8_t aperture_drive_in_af)
{
    text_stringstream ts;

    switch (aperture_drive_in_af) {
    case SDK::CrSilentModeApertureDriveInAF::CrSilentModeApertureDriveInAF_NotTarget:
        ts << "Not Target";
        break;
    case SDK::CrSilentModeApertureDriveInAF::CrSilentModeApertureDriveInAF_Standard:
        ts << "Standard";
        break;
    case SDK::CrSilentModeApertureDriveInAF::CrSilentModeApertureDriveInAF_SilentPriority:
        ts << "Silent Priority";
        break;
    default:
        break;
    }
    return ts.str();
}

text format_silent_mode_shutter_when_power_off(std::uint8_t when_power_off)
{
    text_stringstream ts;

    switch (when_power_off) {
    case SDK::CrSilentModeShutterWhenPowerOff::CrSilentModeShutterWhenPowerOff_NotTarget:
        ts << "Not Target";
        break;
    case SDK::CrSilentModeShutterWhenPowerOff::CrSilentModeShutterWhenPowerOff_Off:
        ts << "OFF";
        break;
    default:
        break;
    }
    return ts.str();
}
text format_silent_mode_auto_pixel_mapping(std::uint8_t auto_pixel_mapping)
{
    text_stringstream ts;

    switch (auto_pixel_mapping) {
    case SDK::CrSilentModeAutoPixelMapping::CrSilentModeAutoPixelMapping_NotTarget:
        ts << "Not Target";
        break;
    case SDK::CrSilentModeAutoPixelMapping::CrSilentModeAutoPixelMapping_Off:
        ts << "OFF";
        break;
    default:
        break;
    }
    return ts.str();
}

text format_shutter_type(std::uint8_t shutter_type)
{
    text_stringstream ts;

    switch (shutter_type) {
    case SDK::CrShutterType::CrShutterType_Auto:
        ts << "Auto";
        break;
    case SDK::CrShutterType::CrShutterType_MechanicalShutter:
        ts << "Mechanical Shutter";
        break;
    case SDK::CrShutterType::CrShutterType_ElectronicShutter:
        ts << "Electronic Shutter";
        break;
    default:
        break;
    }
    return ts.str();
}

text format_movie_shooting_mode(std::uint16_t movie_shooting_mode)
{
    text_stringstream ts;

    switch (movie_shooting_mode) {
    case SDK::CrMovieShootingMode::CrMovieShootingMode_Off:
        ts << "OFF";
        break;
    case SDK::CrMovieShootingMode::CrMovieShootingMode_CineEI:
        ts << "Cine EI";
        break;
    case SDK::CrMovieShootingMode::CrMovieShootingMode_CineEIQuick:
        ts << "Cine EI Quick";
        break;
    case SDK::CrMovieShootingMode::CrMovieShootingMode_Custom:
        ts << "Custom";
        break;
    case SDK::CrMovieShootingMode::CrMovieShootingMode_FlexibleISO:
        ts << "Flexible ISO";
        break;
    default:
        break;
    }
    return ts.str();
}

} // namespace cli
