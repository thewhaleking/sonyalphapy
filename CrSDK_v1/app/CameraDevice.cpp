#if defined (_WIN32) || defined(_WIN64)

#include <SDKDDKVer.h>

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>
#include <memory>

// TODO: reference additional headers your program requires here
#pragma warning (push)
#pragma warning (disable:4819)	// warning C4819: The file contains a character that cannot be represented in the current code page (932). Save the file in Unicode format to prevent data loss
#pragma warning (pop)
#endif

#include "CameraDevice.h"
#include <chrono>
#include <filesystem>
namespace fs = std::filesystem;
#include <fstream>
#include <thread>
#include "CRSDK/CrDeviceProperty.h"
#include "Text.h"


#if defined(__APPLE__) || defined(__linux__)
#include <sys/stat.h>
#include <vector>
#include <dirent.h>
#endif


#if defined(__APPLE__) || defined(__linux__)
#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#else
#include <conio.h>
#endif


// Enumerator
enum Password_Key {

#if defined(__APPLE__) || defined(__linux__)
    Password_Key_Back = 127,
    Password_Key_Enter = 10
#else
    Password_Key_Back = 8,
    Password_Key_Enter = 13
#endif

};


#if defined(__APPLE__) || defined(__linux__)
/* reads from keypress, doesn't echo */
int getch_for_Nix(void)
{
    struct termios oldattr, newattr;
    int iptCh;
    tcgetattr(STDIN_FILENO, &oldattr);
    newattr = oldattr;
    newattr.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newattr);
    iptCh = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldattr);
    return iptCh;
}
#endif


namespace SDK = SCRSDK;
using namespace std::chrono_literals;

constexpr int const ImageSaveAutoStartNo = -1;

namespace cli
{
CameraDevice::CameraDevice(std::int32_t no, SCRSDK::ICrCameraObjectInfo const* camera_info)
    : m_number(no)
    , m_device_handle(0)
    , m_connected(false)
    , m_conn_type(ConnectionType::UNKNOWN)
    , m_net_info()
    , m_usb_info()
    , m_prop()
    , m_lvEnbSet(true)
    , m_modeSDK(SCRSDK::CrSdkControlMode_Remote)
    , m_spontaneous_disconnection(false)
    , m_fingerprint("")
    , m_userPassword("")
{
    m_info = SDK::CreateCameraObjectInfo(
        camera_info->GetName(),
        camera_info->GetModel(),
        camera_info->GetUsbPid(),
        camera_info->GetIdType(),
        camera_info->GetIdSize(),
        camera_info->GetId(),
        camera_info->GetConnectionTypeName(),
        camera_info->GetAdaptorName(),
        camera_info->GetPairingNecessity(),
        camera_info->GetSSHsupport()
    );

    m_conn_type = parse_connection_type(m_info->GetConnectionTypeName());
    switch (m_conn_type)
    {
    case ConnectionType::NETWORK:
        m_net_info = parse_ip_info(m_info->GetId(), m_info->GetIdSize());
        break;
    case ConnectionType::USB:
        m_usb_info.pid = m_info->GetUsbPid();
        break;
    case ConnectionType::UNKNOWN:
        [[fallthrough]];
    default:
        // Do nothing
        break;
    }
}

CameraDevice::~CameraDevice()
{
    if (m_info) m_info->Release();
}

bool CameraDevice::getfingerprint()
{
    CrInt32u fpLen = 0;
    char fpBuff[128] = { 0 };
    SDK::CrError err = SDK::GetFingerprint(m_info, fpBuff, &fpLen);

    if (CR_SUCCEEDED(err))
    {
        m_fingerprint = std::string(fpBuff, fpLen);
        return true;
    }
    return false;
}

bool CameraDevice::connect(SCRSDK::CrSdkControlMode openMode, SCRSDK::CrReconnectingSet reconnect)
{
    const char* inputId = "admin";
    char inputPassword[32] = { 0 };
    if (SDK::CrSSHsupportValue::CrSSHsupport_ON == get_sshsupport())
    {
        if (!is_getfingerprint())
        {
            bool resFp = getfingerprint();
            if (resFp)
            {
                tout << "fingerprint: \n" << m_fingerprint.c_str() << std::endl;
                tout << std::endl << "Are you sure you want to continue connecting ? (y/n) > ";
                text yesno;
                std::getline(cli::tin, yesno);
                if (yesno != TEXT("y"))
                {
                    m_fingerprint.clear();
                    return false;
                }
            }
        }
        if (!is_setpassword())
        {
            cli::tout << "Please SSH password > ";
            cli::text userPw;
 
            // Stores the password
            char maskPw = '*';
            char ch_ipt = {};

            // Until condition is true
            while (true) {

#if defined (_WIN32) || defined(_WIN64)
                ch_ipt = getch();
#else
                ch_ipt = getch_for_Nix();
#endif

                // if the ch_ipt
                if (ch_ipt == Password_Key_Enter) {
                    tout << std::endl;
                    break;
                }
                else if (ch_ipt == Password_Key_Back
                    && userPw.length() != 0) {
                    userPw.pop_back();

                    // Cout statement is very
                    // important as it will erase
                    // previously printed character
                    tout << "\b \b";

                    continue;
                }

                // Without using this, program
                // will crash as \b can't be
                // print in beginning of line
                else if (ch_ipt == Password_Key_Back
                    && userPw.length() == 0) {
                    continue;
                }

                userPw.push_back(ch_ipt);
                tout << maskPw;
            }

#if defined(_WIN32) || (_WIN64)
            mbstate_t mbstate;
            size_t retPw;
            memset(&mbstate, 0, sizeof(mbstate_t));
            const wchar_t* wcsInStr = userPw.c_str();
            errno_t erno = wcsrtombs_s(&retPw, inputPassword, &wcsInStr, 32, &mbstate);
#else
            strncpy(inputPassword, (const char*)userPw.c_str(), userPw.size());
#endif
            m_userPassword = std::string(inputPassword, userPw.size());

        }
    }

    m_spontaneous_disconnection = false;
    auto connect_status = SDK::Connect(m_info, this, &m_device_handle, openMode, reconnect, inputId, m_userPassword.c_str(), m_fingerprint.c_str(), m_fingerprint.size());
    if (CR_FAILED(connect_status)) {
        text id(this->get_id());
        tout << std::endl << "Failed to connect: 0x" << std::hex << connect_status << std::dec << ". " << m_info->GetModel() << " (" << id.data() << ")\n";
        m_userPassword.clear();
        return false;
    }
    set_save_info();
    return true;
}

bool CameraDevice::disconnect()
{
    // m_fingerprint.clear();  // Use as needed
    // m_userPassword.clear(); // Use as needed
    m_spontaneous_disconnection = true;
    tout << "Disconnect from camera...\n";
    auto disconnect_status = SDK::Disconnect(m_device_handle);
    if (CR_FAILED(disconnect_status)) {
        tout << "Disconnect failed to initialize.\n";
        return false;
    }
    return true;
}

bool CameraDevice::release()
{
    tout << "Release camera...\n";
    auto finalize_status = SDK::ReleaseDevice(m_device_handle);
    m_device_handle = 0; // clear
    if (CR_FAILED(finalize_status)) {
        tout << "Finalize device failed to initialize.\n";
        return false;
    }
    return true;
}

CrInt32u CameraDevice::get_sshsupport()
{
    return m_info->GetSSHsupport();
}

SCRSDK::CrSdkControlMode CameraDevice::get_sdkmode() 
{
    load_properties();
    if (SDK::CrSdkControlMode_ContentsTransfer == m_modeSDK) {
        tout << TEXT("Contents Transfer Mode\n");
    }
    else {
        tout << TEXT("Remote Control Mode\n");
    }
    return m_modeSDK;
}

void CameraDevice::capture_image() const
{
    tout << "Capture image...\n";
    tout << "Shutter down\n";
    SDK::SendCommand(m_device_handle, SDK::CrCommandId::CrCommandId_Release, SDK::CrCommandParam_Down);

    // Wait, then send shutter up
    std::this_thread::sleep_for(35ms);
    tout << "Shutter up\n";
    SDK::SendCommand(m_device_handle, SDK::CrCommandId::CrCommandId_Release, SDK::CrCommandParam_Up);
}

void CameraDevice::s1_shooting() const
{
    text input;
    tout << "Is the focus mode set to AF? (y/n): ";
    std::getline(tin, input);
    if (input != TEXT("y")) {
        tout << "Set the focus mode to AF\n";
        return;
    }

    tout << "S1 shooting...\n";
    tout << "Shutter Half Press down\n";
    SDK::CrDeviceProperty prop;
    prop.SetCode(SDK::CrDevicePropertyCode::CrDeviceProperty_S1);
    prop.SetCurrentValue(SDK::CrLockIndicator::CrLockIndicator_Locked);
    prop.SetValueType(SDK::CrDataType::CrDataType_UInt16);
    SDK::SetDeviceProperty(m_device_handle, &prop);

    // Wait, then send shutter up
    std::this_thread::sleep_for(1s);
    tout << "Shutter Half Press up\n";
    prop.SetCurrentValue(SDK::CrLockIndicator::CrLockIndicator_Unlocked);
    SDK::SetDeviceProperty(m_device_handle, &prop);
}

void CameraDevice::af_shutter() const
{
    text input;
    tout << "Is the focus mode set to AF? (y/n): ";
    std::getline(tin, input);
    if (input != TEXT("y")) {
        tout << "Set the focus mode to AF\n";
        return;
    }

    tout << "S1 shooting...\n";
    tout << "Shutter Half Press down\n";
    SDK::CrDeviceProperty prop;
    prop.SetCode(SDK::CrDevicePropertyCode::CrDeviceProperty_S1);
    prop.SetCurrentValue(SDK::CrLockIndicator::CrLockIndicator_Locked);
    prop.SetValueType(SDK::CrDataType::CrDataType_UInt16);
    SDK::SetDeviceProperty(m_device_handle, &prop);

    // Wait, then send shutter down
    std::this_thread::sleep_for(500ms);
    tout << "Shutter down\n";
    SDK::SendCommand(m_device_handle, SDK::CrCommandId::CrCommandId_Release, SDK::CrCommandParam::CrCommandParam_Down);

    // Wait, then send shutter up
    std::this_thread::sleep_for(35ms);
    tout << "Shutter up\n";
    SDK::SendCommand(m_device_handle, SDK::CrCommandId::CrCommandId_Release, SDK::CrCommandParam::CrCommandParam_Up);

    // Wait, then send shutter up
    std::this_thread::sleep_for(1s);
    tout << "Shutter Half Press up\n";
    prop.SetCurrentValue(SDK::CrLockIndicator::CrLockIndicator_Unlocked);
    SDK::SetDeviceProperty(m_device_handle, &prop);
}

void CameraDevice::continuous_shooting()
{
    load_properties();
    tout << "Capture image...\n";
    tout << "Continuous Shooting\n";
    if (1 == m_prop.position_key_setting.writable) {
        // Set, PriorityKeySettings property
        SDK::CrDeviceProperty priority;
        priority.SetCode(SDK::CrDevicePropertyCode::CrDeviceProperty_PriorityKeySettings);
        priority.SetCurrentValue(SDK::CrPriorityKeySettings::CrPriorityKey_PCRemote);
        priority.SetValueType(SDK::CrDataType::CrDataType_UInt32Array);
        auto err_priority = SDK::SetDeviceProperty(m_device_handle, &priority);
        if (CR_FAILED(err_priority)) {
            tout << "Priority Key setting FAILED\n";
            return;
        }
        std::this_thread::sleep_for(500ms);
        get_position_key_setting();
    }

    // Set, still_capture_mode property
    SDK::CrDeviceProperty mode;
    auto& values = m_prop.still_capture_mode.possible;
    mode.SetCode(SDK::CrDevicePropertyCode::CrDeviceProperty_DriveMode);
    if (find(values.begin(), values.end(), SDK::CrDriveMode::CrDrive_Continuous_Hi) != values.end()) {
        mode.SetCurrentValue(SDK::CrDriveMode::CrDrive_Continuous_Hi);
    }
    else if (find(values.begin(), values.end(), SDK::CrDriveMode::CrDrive_Continuous) != values.end()) {
        mode.SetCurrentValue(SDK::CrDriveMode::CrDrive_Continuous);
    }
    else if (1 == m_prop.still_capture_mode.writable) {
        tout << "Continuous Shot hi or Continuous Shot is not supported.\n";
        return;
    }
    else {
        tout << "Still Capture Mode setting is not writable.\n";
        return;
    }
    mode.SetValueType(SDK::CrDataType::CrDataType_UInt32Array);
    auto err_still_capture_mode = SDK::SetDeviceProperty(m_device_handle, &mode);
    if (CR_FAILED(err_still_capture_mode)) {
        tout << "Still Capture Mode setting FAILED\n";
        return;
    }

    // get_still_capture_mode();
    std::this_thread::sleep_for(1s);
    get_still_capture_mode();

	if ((m_prop.still_capture_mode.current == SDK::CrDriveMode::CrDrive_Continuous_Hi)||
		(m_prop.still_capture_mode.current == SDK::CrDriveMode::CrDrive_Continuous)){
        tout << "Still Capture Mode setting SUCCESS\n";
        tout << "Shutter down\n";
        SDK::SendCommand(m_device_handle, SDK::CrCommandId::CrCommandId_Release, SDK::CrCommandParam::CrCommandParam_Down);

        // Wait, then send shutter up
        std::this_thread::sleep_for(500ms);
        tout << "Shutter up\n";
        SDK::SendCommand(m_device_handle, SDK::CrCommandId::CrCommandId_Release, SDK::CrCommandParam::CrCommandParam_Up);
    }
    else {
        tout << "Still Capture Mode setting FAILED\n";
    }
}

void CameraDevice::get_aperture()
{
    load_properties();
    tout << format_f_number(m_prop.f_number.current) << '\n';
}

void CameraDevice::get_iso()
{
    load_properties();

    tout << "ISO: " << format_iso_sensitivity(m_prop.iso_sensitivity.current) << '\n';
}

void CameraDevice::get_shutter_speed()
{
    load_properties();
    tout << "Shutter speed: " << format_shutter_speed(m_prop.shutter_speed.current) << '\n';
}

void CameraDevice::get_position_key_setting()
{
    load_properties();
    tout << "Position Key Setting: " << format_position_key_setting(m_prop.position_key_setting.current) << '\n';
}

void CameraDevice::get_exposure_program_mode()
{
    load_properties();
    tout << "Exposure Program Mode: " << format_exposure_program_mode(m_prop.exposure_program_mode.current) << '\n';
}

void CameraDevice::get_still_capture_mode()
{
    load_properties();
    tout << "Still Capture Mode: " << format_still_capture_mode(m_prop.still_capture_mode.current) << '\n';
}

void CameraDevice::get_focus_mode()
{
    load_properties();
    tout << "Focus Mode: " << format_focus_mode(m_prop.focus_mode.current) << '\n';
}

void CameraDevice::get_focus_area()
{
    load_properties();
    tout << "Focus Area: " << format_focus_area(m_prop.focus_area.current) << '\n';
}

void CameraDevice::get_live_view()
{
    tout << "GetLiveView...\n";

    CrInt32 num = 0;
    SDK::CrLiveViewProperty* property = nullptr;
    auto err = SDK::GetLiveViewProperties(m_device_handle, &property, &num);
    if (CR_FAILED(err)) {
        tout << "GetLiveView FAILED\n";
        return;
    }
    SDK::ReleaseLiveViewProperties(m_device_handle, property);

    SDK::CrImageInfo inf;
    err = SDK::GetLiveViewImageInfo(m_device_handle, &inf);
    if (CR_FAILED(err)) {
        tout << "GetLiveView FAILED\n";
        return;
    }

    CrInt32u bufSize = inf.GetBufferSize();
    if (bufSize < 1)
    {
        tout << "GetLiveView FAILED \n";
    }
    else
    {
        auto* image_data = new SDK::CrImageDataBlock();
        if (!image_data)
        {
            tout << "GetLiveView FAILED (new CrImageDataBlock class)\n";
            return;
        }
        CrInt8u* image_buff = new CrInt8u[bufSize];
        if (!image_buff)
        {
            delete image_data;
            tout << "GetLiveView FAILED (new Image buffer)\n";
            return;
        }
        image_data->SetSize(bufSize);
        image_data->SetData(image_buff);

        err = SDK::GetLiveViewImage(m_device_handle, image_data);
        if (CR_FAILED(err))
        {
            // FAILED
            if (err == SDK::CrWarning_Frame_NotUpdated) {
                tout << "Warning. GetLiveView Frame NotUpdate\n";
            }
            else if (err == SDK::CrError_Memory_Insufficient) {
                tout << "Warning. GetLiveView Memory insufficient\n";
            }
            delete[] image_buff; // Release
            delete image_data; // Release
        }
        else
        {
            if (0 < image_data->GetSize())
            {
                // Display
                // etc.
#if defined(__APPLE__)
                char path[MAC_MAX_PATH]; /*MAX_PATH*/
                memset(path, 0, sizeof(path));
                if(NULL == getcwd(path, sizeof(path) - 1)){
                    // FAILED
                    delete[] image_buff; // Release
                    delete image_data; // Release
                    tout << "Folder path is too long.\n";
                    return;
                }
                char filename[] ="/LiveView000000.JPG";
                if(strlen(path) + strlen(filename) > MAC_MAX_PATH){
                    // FAILED
                    delete[] image_buff; // Release
                    delete image_data; // Release
                    tout << "Failed to create save path.\n";
                    return;
                }
                strncat(path, filename, strlen(filename));
#else
                auto path = fs::current_path();
                path.append(TEXT("LiveView000000.JPG"));
#endif
                tout << path << '\n';

                std::ofstream file(path, std::ios::out | std::ios::binary);
                if (!file.bad())
                {
                    file.write((char*)image_data->GetImageData(), image_data->GetImageSize());
                    file.close();
                }
                tout << "GetLiveView SUCCESS\n";
                delete[] image_buff; // Release
                delete image_data; // Release
            }
            else
            {
                // FAILED
                delete[] image_buff; // Release
                delete image_data; // Release
            }
        }
    }
}

void CameraDevice::get_live_view_image_quality()
{
    load_properties();
    tout << "Live View Image Quality: " << format_live_view_image_quality(m_prop.live_view_image_quality.current) << '\n';
}

void CameraDevice::get_select_media_format()
{
    load_properties();
    tout << "Media SLOT1 Full Format Enable Status: " << format_media_slotx_format_enable_status(m_prop.media_slot1_full_format_enable_status.current) << std::endl;
    tout << "Media SLOT2 Full Format Enable Status: " << format_media_slotx_format_enable_status(m_prop.media_slot2_full_format_enable_status.current) << std::endl;
    // Valid Quick format
    if (-1 != m_prop.media_slot1_quick_format_enable_status.writable || -1 != m_prop.media_slot2_quick_format_enable_status.writable){
        tout << "Media SLOT1 Quick Format Enable Status: " << format_media_slotx_format_enable_status(m_prop.media_slot1_quick_format_enable_status.current) << std::endl;
        tout << "Media SLOT2 Quick Format Enable Status: " << format_media_slotx_format_enable_status(m_prop.media_slot2_quick_format_enable_status.current) << std::endl;
    }
}

void CameraDevice::get_white_balance()
{
    load_properties();
    tout << "White Balance: " << format_white_balance(m_prop.white_balance.current) << '\n';
}

bool CameraDevice::get_custom_wb()
{
    bool state = false;
    load_properties();
    tout << "CustomWB Capture Standby Operation: " << format_customwb_capture_standby(m_prop.customwb_capture_standby.current) << '\n';
    tout << "CustomWB Capture Standby CancelOperation: " << format_customwb_capture_standby_cancel(m_prop.customwb_capture_standby_cancel.current) << '\n';
    tout << "CustomWB Capture Operation: " << format_customwb_capture_operation(m_prop.customwb_capture_operation.current) << '\n';
    tout << "CustomWB Capture Execution State: " << format_customwb_capture_execution_state(m_prop.customwb_capture_execution_state.current) << '\n';
    if (m_prop.customwb_capture_operation.current == 1) {
        state = true;
    }
    return state;
}

void CameraDevice::get_zoom_operation()
{
    load_properties();
    tout << "Zoom Operation Status: " << format_zoom_operation_status(m_prop.zoom_operation_status.current) << '\n';
    if (m_prop.zoom_setting_type.current > 0) {
        tout << "Zoom Setting Type: " << format_zoom_setting_type(m_prop.zoom_setting_type.current) << '\n';
    }
    if (m_prop.zoom_types_status.current > 0) {
        tout << "Zoom Type Status: " << format_zoom_types_status(m_prop.zoom_types_status.current) << '\n';
    }

    // Zoom Speed Range is not supported
    if (m_prop.zoom_speed_range.possible.size() < 2) {
        tout << "Zoom Speed Range: -1 to 1" << std::endl 
             << "Zoom Speed Type: " << format_remocon_zoom_speed_type(m_prop.remocon_zoom_speed_type.current) << std::endl;
    }
    else {
        tout << "Zoom Speed Range: " << (int)m_prop.zoom_speed_range.possible.at(0) << " to " << (int)m_prop.zoom_speed_range.possible.at(1) << std::endl
             << "Zoom Speed Type: " << format_remocon_zoom_speed_type(m_prop.remocon_zoom_speed_type.current) << std::endl;
    }

    std::int32_t nprop = 0;
    SDK::CrDeviceProperty* prop_list = nullptr;
    CrInt32u getCode = SDK::CrDevicePropertyCode::CrDeviceProperty_Zoom_Bar_Information;
    auto status = SDK::GetSelectDeviceProperties(m_device_handle, 1, &getCode, &prop_list, &nprop);

    if (CR_FAILED(status)) {
        tout << "Failed to get Zoom Bar Information.\n";
        return;
    }

    if (prop_list && 0 < nprop) {
        auto prop = prop_list[0];
        if (SDK::CrDevicePropertyCode::CrDeviceProperty_Zoom_Bar_Information == prop.GetCode())
        {
            tout << "Zoom Bar Information: 0x" << std::hex << prop.GetCurrentValue() << std::dec << '\n';
        }
        SDK::ReleaseDeviceProperties(m_device_handle, prop_list);
    }
}

void CameraDevice::get_remocon_zoom_speed_type()
{
    load_properties();
    tout << "Zoom Speed Type: " << format_remocon_zoom_speed_type(m_prop.remocon_zoom_speed_type.current) << '\n';
}

bool CameraDevice::get_aps_c_or_full_switching_setting()
{
    load_properties();
    if (m_prop.aps_c_of_full_switching_setting.current < SDK::CrAPS_C_or_Full_SwitchingSetting::CrAPS_C_or_Full_SwitchingSetting_Full)
    {
        tout << "APS-C/FULL Switching Setting is not supported\n";
        return false;
    }
    tout << "APS-C/FULL Switching Enable Status: " << format_aps_c_or_full_switching_enable_status(m_prop.aps_c_of_full_switching_enable_status.current) << '\n';
    tout << "APS-C/FULL Switching Setting: " << format_aps_c_or_full_switching_setting(m_prop.aps_c_of_full_switching_setting.current) << '\n';
    return true;
}

bool CameraDevice::get_camera_setting_saveread_state()
{
    load_properties();
    if (m_prop.camera_setting_save_read_state.current == SDK::CrCameraSettingSaveReadState::CrCameraSettingSaveReadState_Reading) {
        tout << "Unable to download/upload Camera-Setting file. \n";
        return false;
    }
    tout << "Camera-Setting Save/Read State: " << format_camera_setting_save_read_state(m_prop.camera_setting_save_read_state.current) << '\n';
    return true;
}

bool CameraDevice::get_playback_media()
{
    load_properties();
    if (m_prop.playback_media.possible.size() < 1) {
        tout << "Playback Media is not supported\n";
        return false;
    }
    tout << "Playback Media: " << format_playback_media(m_prop.playback_media.current) << '\n';
    return true;
}


bool CameraDevice::get_gain_base_sensitivity()
{
    load_properties();
    if (m_prop.gain_base_sensitivity.possible.size() < 1) {
        tout << "Gain Base Sensitivity is not supported\n";
        return false;
    }
    tout << "Gain Base: " << format_gain_base_sensitivity(m_prop.gain_base_sensitivity.current) << '\n';
    return true;
}

bool CameraDevice::get_gain_base_iso_sensitivity()
{
    load_properties();
    if (m_prop.gain_base_iso_sensitivity.possible.size() < 1){
        tout << "Gain Base ISO Sensitivity is not supported \n";
        return false;
    }
    tout << "Gain Base ISO: " << format_gain_base_iso_sensitivity(m_prop.gain_base_iso_sensitivity.current) << '\n';
    return true;
}

bool CameraDevice::get_monitor_lut_setting()
{
    load_properties();
    if (m_prop.monitor_lut_setting.possible.size() < 1) {
        tout << "Monitor LUT Setting is not supported \n";
        return false;
    }
    tout << "Monitor LUT Setting: " << format_monitor_lut_setting(m_prop.monitor_lut_setting.current) << '\n';
    return true;
}

bool CameraDevice::get_exposure_index()
{
    load_properties();
    if (m_prop.exposure_index.possible.size() < 1) {
        tout << "Exposure Index is not supported \n";
        return false;
    }
    tout << "Exposure Index: " << m_prop.exposure_index.current << '\n';
    return true;
}

bool CameraDevice::get_baselook_value()
{
    load_properties();
    if (m_prop.baselook_value.possible.size() < 1) {
        tout << "BaseLook Value is not supported \n";
        return false;
    }
    //If you want to display a string, you need to execute RequestDisplayStringList() in advance.
    SDK::CrDisplayStringListInfo* pProperty = nullptr;
    SDK::CrDisplayStringType type;
    CrInt32u numOfList = 0;
    CrInt8u baseValueSetter;
    CrInt8u get_baseLookValue;

    SDK::CrError err = SDK::GetDisplayStringList(m_device_handle, SDK::CrDisplayStringType_BaseLook_Name_Display, &pProperty, &numOfList);

    if (CR_SUCCEEDED(err) && numOfList >0) {
        for (int i = 0; i < numOfList; ++i) {
            if (pProperty[i].value == m_prop.baselook_value.current) {
                tout << "BaseLook Value: " << format_dispstrlist(pProperty[i]) << '\n';
                break;
            }
        }
        ReleaseDisplayStringList(m_device_handle, pProperty);
    }
    else {
        CrInt8u valHI = (m_prop.baselook_value.current >>8 ) & 0x01;
        CrInt8u valLO = m_prop.baselook_value.current & 0xFF;
        tout << "BaseLook Value: Index" << (int)valLO << " " << format_baselook_value(valHI) << '\n';
    }

    return true;
}

bool CameraDevice::get_iris_mode_setting()
{
    load_properties();
    if (m_prop.iris_mode_setting.possible.size() < 1) {
        tout << "Iris Mode Setting is not supported \n";
        return false;
    }
    tout << "Iris Mode Setting: " << format_iris_mode_setting(m_prop.iris_mode_setting.current) << '\n';
    return true;
}

bool CameraDevice::get_shutter_mode_setting()
{
    load_properties();
    if (m_prop.shutter_mode_setting.possible.size() < 1) {
        tout << "Shutter Mode Setting is not supported \n";
        return false;
    }
    tout << "Shutter Mode Setting: " << format_shutter_mode_setting(m_prop.shutter_mode_setting.current) << '\n';
    return true;
}

void CameraDevice::get_iso_current_sensitivity()
{
    load_properties();
    if (m_prop.iso_current_sensitivity.current == 0) {
        tout << "ISO Current Sensitivity is not supported. \n";
        return;
    }
    tout << "ISO Current Sensitivity: " << format_iso_sensitivity(m_prop.iso_current_sensitivity.current) << '\n';
}

bool CameraDevice::get_exposure_control_type()
{
    load_properties();
    if (m_prop.exposure_control_type.possible.size() < 1) {
        tout << "Exposure Control Type is not supported.\n";
        return false;
    }
    tout << "Exposure Control Type: " << format_exposure_control_type(m_prop.exposure_control_type.current) << '\n';
    return true;
}

bool CameraDevice::get_gain_control_setting()
{
    load_properties();
    if (m_prop.gain_control_setting.possible.size() < 1) {
        tout << "Gain Control Setting is not supported.\n";
        return false;
    }
    tout << "Gain Control Setting: " << format_gain_control_setting(m_prop.gain_control_setting.current) << '\n';
    return true;
}

bool CameraDevice::get_recording_setting()
{
    load_properties();
    if (m_prop.recording_setting.possible.size() < 1) {
        tout << "Recording Setting is not supported.\n";
        return false;
    }
    tout << "Recording Setting: " << format_recording_setting(m_prop.recording_setting.current) << '\n';
    return true;
}

bool CameraDevice::get_gain_db_value()
{
    load_properties();
    if (m_prop.gain_db_value.possible.size() < 1) {
        tout << "Gain dB Value is not supported.\n";
        return false;
    }
    tout << "Gain dB current value: " << (int)m_prop.gain_db_value.current << "dB\n";
    return true;
}

bool CameraDevice::get_shutter_speed_value()
{
    load_properties();
    if (m_prop.shutter_speed_value.possible.size() < 1) {
        tout << "Shutter Speed Value is not supported.\n";
        return false;
    }
    tout << "Shutter speed value: " << format_shutter_speed_value(m_prop.shutter_speed_value.current) << '\n';
    return true;
}

bool CameraDevice::get_white_balance_tint()
{
    load_properties();
    if (m_prop.white_balance_tint.possible.size() < 1)
    {
        tout << "White Balance Tint is not supported.\n";
        return false;
    }
    if (m_prop.white_balance_tint_step.possible.size() < 1)
    {
        tout << "White Balance Tint Step is not supported.\n";
        return false;
    }
    tout << "White Balance Tint CurrentValue: " << (int)m_prop.white_balance_tint.current << '\n';
    tout << "White Balance Tint        Range: " << (int)m_prop.white_balance_tint.possible.at(0) << " to " << (int)m_prop.white_balance_tint.possible.at(1) << std::endl;
    tout << "White Balance Tint Step   Range: " << (int)m_prop.white_balance_tint_step.possible.at(0) << " to " << (int)m_prop.white_balance_tint_step.possible.at(1) << std::endl;
    return true;
}

void CameraDevice::get_media_slot_status()
{
    load_properties();
    tout << "Media SLOT1 Status: " << format_media_slotx_status(m_prop.media_slot1_status.current) << std::endl;
    if(m_prop.media_slot2_status.writable == -1)
        tout << "Media SLOT2 Status is not supported.\n";
    else
        tout << "Media SLOT2 Status: " << format_media_slotx_status(m_prop.media_slot2_status.current) << std::endl;
}

bool CameraDevice::get_movie_rec_button_toggle_enable_status()
{
    load_properties();
    if (m_prop.movie_rec_button_toggle_enable_status.writable == -1)
    {
        tout << "Movie Rec Button(Toggle) is not supported\n";
        return false;
    }
    tout << "Movie Rec Button(Toggle) Enable Status: " << format_movie_rec_button_toggle_enable_status(m_prop.movie_rec_button_toggle_enable_status.current) << '\n';
    return true;
}

bool CameraDevice::get_focus_bracket_shot_num()
{
    load_properties();
    if (m_prop.focus_bracket_shot_num.possible.size() < 3)
    {
        tout << "Focus Bracket Shot Number is not supported \n";
        return false;
    }
    tout << "Focus Bracket Shot Number: " <<m_prop.focus_bracket_shot_num.current << '\n';
    tout << "                   Range : " << (int)m_prop.focus_bracket_shot_num.possible.at(0) << " to " << (int)m_prop.focus_bracket_shot_num.possible.at(1) << std::endl;

    return true;
}

bool CameraDevice::get_focus_bracket_focus_range()
{
    load_properties();
    if (m_prop.focus_bracket_focus_range.possible.size() < 3)
    {
        tout << "Focus Bracket Focus Range is not supported \n";
        return false;
    }
    tout << "Focus Bracket Focus Range CurrentValue: " <<(int)m_prop.focus_bracket_focus_range.current << '\n';
    tout << "                                 Range: " << (int)m_prop.focus_bracket_focus_range.possible.at(0) << " to " << (int)m_prop.focus_bracket_focus_range.possible.at(1) << std::endl;

    return true;
}

bool CameraDevice::get_movie_image_stabilization_steady_shot()
{
    load_properties();
    if (m_prop.movie_image_stabilization_steady_shot.possible.size() < 1) {
        tout << "Image Stabilization Steady Shot(Movie) is not supported.\n";
        return false;
    }
    tout << "Image Stabilization Steady Shot(Movie): " << format_movie_image_stabilization_steady_shot(m_prop.movie_image_stabilization_steady_shot.current) << '\n';
    return true;
}

bool CameraDevice::get_image_stabilization_steady_shot()
{
    load_properties();
    if (m_prop.image_stabilization_steady_shot.possible.size() < 1) {
        tout << "Image Stabilization Steady Shot(Still) is not supported.\n";
        return false;
    }
    tout << "Image Stabilization Steady Shot(Still): " << format_image_stabilization_steady_shot(m_prop.image_stabilization_steady_shot.current) << '\n';
    return true;
}

bool CameraDevice::get_silent_mode()
{
    load_properties();
    if (m_prop.silent_mode.possible.size() < 1) {
        tout << "Silent Mode is not supported.\n";
        return false;
    }
    tout << "Silent Mode: " << format_silent_mode(m_prop.silent_mode.current) << '\n';
    return true;
}

bool CameraDevice::get_silent_mode_aperture_drive_in_af()
{
    load_properties();
    if (m_prop.silent_mode_aperture_drive_in_af.possible.size() < 1) {
        tout << "Silent Mode Aperture Drive in AF is not supported.\n";
        return false;
    }
    tout << "Aperture Drive in AF: " << format_silent_mode_aperture_drive_in_af(m_prop.silent_mode_aperture_drive_in_af.current) << '\n';
    return true;
}

bool CameraDevice::get_silent_mode_shutter_when_power_off()
{
    load_properties();
    if (m_prop.silent_mode_shutter_when_power_off.possible.size() < 1) {
        tout << "Silent Mode Shutter When Power OFF is not supported.\n";
        return false;
    }
    tout << "Shutter When Power OFF: " << format_silent_mode_shutter_when_power_off(m_prop.silent_mode_shutter_when_power_off.current) << '\n';
    return true;
}

bool CameraDevice::get_silent_mode_auto_pixel_mapping()
{
    load_properties();
    if (m_prop.silent_mode_auto_pixel_mapping.possible.size() < 1) {
        tout << "Silent Mode Auto Pixel Mapping is not supported.\n";
        return false;
    }
    tout << "Auto Pixel Mapping: " << format_silent_mode_auto_pixel_mapping(m_prop.silent_mode_auto_pixel_mapping.current) << '\n';
    return true;
}

bool CameraDevice::get_shutter_type()
{
    load_properties();
    if (m_prop.shutter_type.possible.size() < 1)
    {
        tout << "Shutter Type is not supported.\n";
        return false;
    }
    tout << "Shutter Type: " << format_shutter_type(m_prop.shutter_type.current) << '\n';
    return true;
}


bool CameraDevice::get_movie_shooting_mode()
{
    load_properties();
    if (m_prop.movie_shooting_mode.possible.size() < 1) {
        tout << "Movie Shooting Mode is not supported.\n";
        return false;
    }
    tout << "Movie Shooting Mode: " << format_movie_shooting_mode(m_prop.movie_shooting_mode.current) << '\n';
    return true;
}

void CameraDevice::set_aperture()
{
    if (1 != m_prop.f_number.writable) {
        // Not a settable property
        tout << "Aperture is not writable\n";
        return;
    }

    text input;
    tout << "Would you like to set a new Aperture value? (y/n): ";
    std::getline(tin, input);
    if (input != TEXT("y")) {
        tout << "Skip setting a new value.\n";
        return;
    }

    tout << "Choose a number set a new Aperture value:\n";
    tout << "[-1] Cancel input\n";

    auto& values = m_prop.f_number.possible;
    for (std::size_t i = 0; i < values.size(); ++i) {
        tout << '[' << i << "] " << format_f_number(values[i]) << '\n';
    }

    tout << "[-1] Cancel input\n";
    tout << "Choose a number set a new Aperture value:\n";

    tout << "input> ";
    std::getline(tin, input);
    text_stringstream ss(input);
    int selected_index = 0;
    ss >> selected_index;

    if (selected_index < 0 || values.size() <= selected_index) {
        tout << "Input cancelled.\n";
        return;
    }

    SDK::CrDeviceProperty prop;
    prop.SetCode(SDK::CrDevicePropertyCode::CrDeviceProperty_FNumber);
    prop.SetCurrentValue(values[selected_index]);
    prop.SetValueType(SDK::CrDataType::CrDataType_UInt16Array);

    SDK::SetDeviceProperty(m_device_handle, &prop);
}

void CameraDevice::set_iso()
{
    if (1 != m_prop.iso_sensitivity.writable) {
        // Not a settable property
        tout << "ISO is not writable\n";
        return;
    }

    text input;
    tout << "Would you like to set a new ISO value? (y/n): ";
    std::getline(tin, input);
    if (input != TEXT("y")) {
        tout << "Skip setting a new value.\n";
        return;
    }

    tout << "Choose a number set a new ISO value:\n";
    tout << "[-1] Cancel input\n";

    auto& values = m_prop.iso_sensitivity.possible;
    for (std::size_t i = 0; i < values.size(); ++i) {
        tout << '[' << i << "] " << format_iso_sensitivity(values[i]) << '\n';
    }

    tout << "[-1] Cancel input\n";
    tout << "Choose a number set a new ISO value:\n";

    tout << "input> ";
    std::getline(tin, input);
    text_stringstream ss(input);
    int selected_index = 0;
    ss >> selected_index;

    if (selected_index < 0 || values.size() <= selected_index) {
        tout << "Input cancelled.\n";
        return;
    }

    SDK::CrDeviceProperty prop;
    prop.SetCode(SDK::CrDevicePropertyCode::CrDeviceProperty_IsoSensitivity);
    prop.SetCurrentValue(values[selected_index]);
    prop.SetValueType(SDK::CrDataType::CrDataType_UInt32Array);

    SDK::SetDeviceProperty(m_device_handle, &prop);
}

bool CameraDevice::set_save_info() const
{
#if defined(__APPLE__)
    text_char path[MAC_MAX_PATH]; /*MAX_PATH*/
    memset(path, 0, sizeof(path));
    if(NULL == getcwd(path, sizeof(path) - 1)){
        tout << "Folder path is too long.\n";
        return false;
    }
    auto save_status = SDK::SetSaveInfo(m_device_handle
        , path, (char*)"", ImageSaveAutoStartNo);
#else
    text path = fs::current_path().native();
    tout << path.data() << '\n';

    auto save_status = SDK::SetSaveInfo(m_device_handle
        , const_cast<text_char*>(path.data()), TEXT(""), ImageSaveAutoStartNo);
#endif
    if (CR_FAILED(save_status)) {
        tout << "Failed to set save path.\n";
        return false;
    }
    return true;
}

void CameraDevice::set_shutter_speed()
{
    if (1 != m_prop.shutter_speed.writable) {
        // Not a settable property
        tout << "Shutter Speed is not writable\n";
        return;
    }

    text input;
    tout << "Would you like to set a new Shutter Speed value? (y/n): ";
    std::getline(tin, input);
    if (input != TEXT("y")) {
        tout << "Skip setting a new value.\n";
        return;
    }

    tout << "Choose a number set a new Shutter Speed value:\n";
    tout << "[-1] Cancel input\n";

    auto& values = m_prop.shutter_speed.possible;
    for (std::size_t i = 0; i < values.size(); ++i) {
        tout << '[' << i << "] " << format_shutter_speed(values[i]) << '\n';
    }

    tout << "[-1] Cancel input\n";
    tout << "Choose a number set a new Shutter Speed value:\n";

    tout << "input> ";
    std::getline(tin, input);
    text_stringstream ss(input);
    int selected_index = 0;
    ss >> selected_index;

    if (selected_index < 0 || values.size() <= selected_index) {
        tout << "Input cancelled.\n";
        return;
    }

    SDK::CrDeviceProperty prop;
    prop.SetCode(SDK::CrDevicePropertyCode::CrDeviceProperty_ShutterSpeed);
    prop.SetCurrentValue(values[selected_index]);
    prop.SetValueType(SDK::CrDataType::CrDataType_UInt32Array);

    SDK::SetDeviceProperty(m_device_handle, &prop);
}

void CameraDevice::set_position_key_setting()
{
    if (1 != m_prop.position_key_setting.writable) {
        // Not a settable property
        tout << "Position Key Setting is not writable\n";
        return;
    }

    text input;
    tout << "Would you like to set a new Position Key Setting value? (y/n): ";
    std::getline(tin, input);
    if (input != TEXT("y")) {
        tout << "Skip setting a new value.\n";
        return;
    }

    tout << "Choose a number set a new Position Key Setting value:\n";
    tout << "[-1] Cancel input\n";

    auto& values = m_prop.position_key_setting.possible;
    for (std::size_t i = 0; i < values.size(); ++i) {
        tout << '[' << i << "] " << format_position_key_setting(values[i]) << '\n';
    }

    tout << "[-1] Cancel input\n";
    tout << "Choose a number set a new Position Key Setting value:\n";

    tout << "input> ";
    std::getline(tin, input);
    text_stringstream ss(input);
    int selected_index = 0;
    ss >> selected_index;

    if (selected_index < 0 || values.size() <= selected_index) {
        tout << "Input cancelled.\n";
        return;
    }

    SDK::CrDeviceProperty prop;
    prop.SetCode(SDK::CrDevicePropertyCode::CrDeviceProperty_PriorityKeySettings);
    prop.SetCurrentValue(values[selected_index]);
    prop.SetValueType(SDK::CrDataType::CrDataType_UInt8Array);

    SDK::SetDeviceProperty(m_device_handle, &prop);
}

void CameraDevice::set_exposure_program_mode()
{
    if (1 != m_prop.exposure_program_mode.writable) {
        // Not a settable property
        tout << "Exposure Program Mode is not writable\n";
        return;
    }

    text input;
    tout << "Would you like to set a new Exposure Program Mode value? (y/n): ";
    std::getline(tin, input);
    if (input != TEXT("y")) {
        tout << "Skip setting a new value.\n";
        return;
    }

    tout << "Choose a number set a new Exposure Program Mode value:\n";
    tout << "[-1] Cancel input\n";

    auto& values = m_prop.exposure_program_mode.possible;
    for (std::size_t i = 0; i < values.size(); ++i) {
        tout << '[' << i << "] " << format_exposure_program_mode(values[i]) << '\n';
    }

    tout << "[-1] Cancel input\n";
    tout << "Choose a number set a new Exposure Program Mode value:\n";

    tout << "input> ";
    std::getline(tin, input);
    text_stringstream ss(input);
    int selected_index = 0;
    ss >> selected_index;

    if (selected_index < 0 || values.size() <= selected_index) {
        tout << "Input cancelled.\n";
        return;
    }

    SDK::CrDeviceProperty prop;
    prop.SetCode(SDK::CrDevicePropertyCode::CrDeviceProperty_ExposureProgramMode);
    prop.SetCurrentValue(values[selected_index]);
    prop.SetValueType(SDK::CrDataType::CrDataType_UInt16Array);

    SDK::SetDeviceProperty(m_device_handle, &prop);
}

void CameraDevice::set_still_capture_mode()
{
    if (1 != m_prop.still_capture_mode.writable) {
        // Not a settable property
        tout << "Still Capture Mode is not writable\n";
        return;
    }

    text input;
    tout << "Would you like to set a new Still Capture Mode value? (y/n): ";
    std::getline(tin, input);
    if (input != TEXT("y")) {
        tout << "Skip setting a new value.\n";
        return;
    }

    tout << "Choose a number set a new Still Capture Mode value:\n";
    tout << "[-1] Cancel input\n";

    auto& values = m_prop.still_capture_mode.possible;
    for (std::size_t i = 0; i < values.size(); ++i) {
        tout << '[' << i << "] " << format_still_capture_mode(values[i]) << '\n';
    }

    tout << "[-1] Cancel input\n";
    tout << "Choose a number set a new Still Capture Mode value:\n";

    tout << "input> ";
    std::getline(tin, input);
    text_stringstream ss(input);
    int selected_index = 0;
    ss >> selected_index;

    if (selected_index < 0 || values.size() <= selected_index) {
        tout << "Input cancelled.\n";
        return;
    }

    SDK::CrDeviceProperty prop;
    prop.SetCode(SDK::CrDevicePropertyCode::CrDeviceProperty_DriveMode);
    prop.SetCurrentValue(values[selected_index]);
    prop.SetValueType(SDK::CrDataType::CrDataType_UInt32Array);

    SDK::SetDeviceProperty(m_device_handle, &prop);
}

void CameraDevice::set_focus_mode()
{
    if (1 != m_prop.focus_mode.writable) {
        // Not a settable property
        tout << "Focus Mode is not writable\n";
        return;
    }

    text input;
    tout << "Would you like to set a new Focus Mode value? (y/n): ";
    std::getline(tin, input);
    if (input != TEXT("y")) {
        tout << "Skip setting a new value.\n";
        return;
    }

    tout << "Choose a number set a new Focus Mode value:\n";
    tout << "[-1] Cancel input\n";

    auto& values = m_prop.focus_mode.possible;
    for (std::size_t i = 0; i < values.size(); ++i) {
        tout << '[' << i << "] " << format_focus_mode(values[i]) << '\n';
    }

    tout << "[-1] Cancel input\n";
    tout << "Choose a number set a new Focus Mode value:\n";

    tout << "input> ";
    std::getline(tin, input);
    text_stringstream ss(input);
    int selected_index = 0;
    ss >> selected_index;

    if (selected_index < 0 || values.size() <= selected_index) {
        tout << "Input cancelled.\n";
        return;
    }

    SDK::CrDeviceProperty prop;
    prop.SetCode(SDK::CrDevicePropertyCode::CrDeviceProperty_FocusMode);
    prop.SetCurrentValue(values[selected_index]);
    prop.SetValueType(SDK::CrDataType::CrDataType_UInt16Array);

    SDK::SetDeviceProperty(m_device_handle, &prop);
}

void CameraDevice::set_focus_area()
{
    if (1 != m_prop.focus_area.writable) {
        // Not a settable property
        tout << "Focus Area is not writable\n";
        return;
    }

    text input;
    tout << "Would you like to set a new Focus Area value? (y/n): ";
    std::getline(tin, input);
    if (input != TEXT("y")) {
        tout << "Skip setting a new value.\n";
        return;
    }

    tout << "Choose a number set a new Focus Area value:\n";
    tout << "[-1] Cancel input\n";

    auto& values = m_prop.focus_area.possible;
    for (std::size_t i = 0; i < values.size(); ++i) {
        tout << '[' << i << "] " << format_focus_area(values[i]) << '\n';
    }

    tout << "[-1] Cancel input\n";
    tout << "Choose a number set a new Focus Area value:\n";

    tout << "input> ";
    std::getline(tin, input);
    text_stringstream ss(input);
    int selected_index = 0;
    ss >> selected_index;

    if (selected_index < 0 || values.size() <= selected_index) {
        tout << "Input cancelled.\n";
        return;
    }

    SDK::CrDeviceProperty prop;
    prop.SetCode(SDK::CrDevicePropertyCode::CrDeviceProperty_FocusArea);
    prop.SetCurrentValue(values[selected_index]);
    prop.SetValueType(SDK::CrDataType::CrDataType_UInt16Array);

    SDK::SetDeviceProperty(m_device_handle, &prop);
}

void CameraDevice::set_live_view_image_quality()
{
    if (1 != m_prop.live_view_image_quality.writable) {
        // Not a settable property
        tout << "Live View Image Quality is not writable\n";
        return;
    }

    text input;
    tout << "Would you like to set a new Live View Image Quality value? (y/n): ";
    std::getline(tin, input);
    if (input != TEXT("y")) {
        tout << "Skip setting a new value.\n";
        return;
    }

    tout << "Choose a number set a new Live View Image Quality value:\n";
    tout << "[-1] Cancel input\n";

    auto& values = m_prop.live_view_image_quality.possible;
    for (std::size_t i = 0; i < values.size(); ++i) {
        tout << '[' << i << "] " << format_live_view_image_quality(values[i]) << '\n';
    }

    tout << "[-1] Cancel input\n";
    tout << "Choose a number set a new Live View Image Quality value:\n";

    tout << "input> ";
    std::getline(tin, input);
    text_stringstream ss(input);
    int selected_index = 0;
    ss >> selected_index;

    if (selected_index < 0 || values.size() <= selected_index) {
        tout << "Input cancelled.\n";
        return;
    }

    SDK::CrDeviceProperty prop;
    prop.SetCode(SDK::CrDevicePropertyCode::CrDeviceProperty_LiveView_Image_Quality);
    prop.SetCurrentValue(values[selected_index]);
    prop.SetValueType(SDK::CrDataType::CrDataType_UInt16Array);

    SDK::SetDeviceProperty(m_device_handle, &prop);
}

void CameraDevice::set_white_balance()
{
    if (1 != m_prop.white_balance.writable) {
        // Not a settable property
        tout << "White Balance is not writable\n";
        return;
    }

    text input;
    tout << std::endl << "Would you like to set a new White Balance value? (y/n): ";
    std::getline(tin, input);
    if (input != TEXT("y")) {
        tout << "Skip setting a new value.\n";
        return;
    }

    tout << std::endl << "Choose a number set a new White Balance value:\n";
    tout << "[-1] Cancel input\n";

    auto& values = m_prop.white_balance.possible;
    for (std::size_t i = 0; i < values.size(); ++i) {
        tout << '[' << i << "] " << format_white_balance(values[i]) << '\n';
    }

    tout << "[-1] Cancel input\n";
    tout << std::endl << "Choose a number set a new White Balance value:\n";

    tout << std::endl << "input> ";
    std::getline(tin, input);
    text_stringstream ss(input);
    int selected_index = 0;
    ss >> selected_index;

    if (selected_index < 0 || values.size() <= selected_index) {
        tout << "Input cancelled.\n";
        return;
    }

    SDK::CrDeviceProperty prop;
    prop.SetCode(SDK::CrDevicePropertyCode::CrDeviceProperty_WhiteBalance);
    prop.SetCurrentValue(values[selected_index]);
    prop.SetValueType(SDK::CrDataType::CrDataType_UInt16Array);

    SDK::SetDeviceProperty(m_device_handle, &prop);
}

void CameraDevice::execute_lock_property(CrInt16u code)
{
    load_properties();

    text input;
    tout << std::endl << "Would you like to execute Unlock or Lock? (y/n): ";
    std::getline(tin, input);
    if (input != TEXT("y")) {
        tout << "Skip execute a new value.\n";
        return;
    }

    tout << std::endl << "Choose a number:\n";
    tout << "[-1] Cancel input\n";

    tout << "[1] Unlock" << '\n';
    tout << "[2] Lock" << '\n';

    tout << "[-1] Cancel input\n";
    tout << "Choose a number:\n";

    tout << std::endl << "input> ";
    std::getline(tin, input);
    text_stringstream ss(input);
    int selected_index = 0;
    ss >> selected_index;

    CrInt64u ptpValue = 0;
    switch (selected_index) {
    case 1:
        ptpValue = SDK::CrLockIndicator::CrLockIndicator_Unlocked;
        break;
    case 2:
        ptpValue = SDK::CrLockIndicator::CrLockIndicator_Locked;
        break;
    default:
        selected_index = -1;
        break;
    }

    if (-1 == selected_index) {
        tout << "Input cancelled.\n";
        return;
    }

    SDK::CrDeviceProperty prop;
    prop.SetCode(code);
    prop.SetCurrentValue((CrInt64u)(ptpValue));
    prop.SetValueType(SDK::CrDataType::CrDataType_UInt16Array);

    SDK::SetDeviceProperty(m_device_handle, &prop);
}

void CameraDevice::get_af_area_position()
{
    CrInt32 num = 0;
    SDK::CrLiveViewProperty* lvProperty = nullptr;
    CrInt32u getCode = SDK::CrLiveViewPropertyCode::CrLiveViewProperty_AF_Area_Position;
    auto err = SDK::GetSelectLiveViewProperties(m_device_handle, 1, &getCode, &lvProperty, &num);
    if (CR_FAILED(err)) {
        tout << "Failed to get AF Area Position [LiveViewProperties]\n";
        return;
    }

    if (lvProperty && 1 == num) {
        // Got AF Area Position
        auto prop = lvProperty[0];
        if (SDK::CrFrameInfoType::CrFrameInfoType_FocusFrameInfo == prop.GetFrameInfoType()) {
            int sizVal = prop.GetValueSize();
            int count = sizVal / sizeof(SDK::CrFocusFrameInfo);
            SDK::CrFocusFrameInfo* pFrameInfo = (SDK::CrFocusFrameInfo*)prop.GetValue();
            if (0 == sizVal || nullptr == pFrameInfo) {
                tout << "  FocusFrameInfo nothing\n";
            }
            else {
                for (std::int32_t frame = 0; frame < count; ++frame) {
                    auto lvprop = pFrameInfo[frame];
                    char buff[512];
                    memset(buff, 0, sizeof(buff));
#if defined(_WIN32) || (_WIN64)
                    sprintf_s(buff, "  FocusFrameInfo no[%d] pri[%d] w[%d] h[%d] Deno[%d-%d] Nume[%d-%d]",
                        frame + 1,
                        lvprop.priority,
                        lvprop.width, lvprop.height,
                        lvprop.xDenominator, lvprop.yDenominator,
                        lvprop.xNumerator, lvprop.yNumerator);
#else
                    snprintf(buff, sizeof(buff), "  FocusFrameInfo no[%d] pri[%d] w[%d] h[%d] Deno[%d-%d] Nume[%d-%d]",
                        frame + 1,
                        lvprop.priority,
                        lvprop.width, lvprop.height,
                        lvprop.xDenominator, lvprop.yDenominator,
                        lvprop.xNumerator, lvprop.yNumerator);
#endif
                      
                    tout << buff << std::endl;
                }
            }
        }
        SDK::ReleaseLiveViewProperties(m_device_handle, lvProperty);
    }
}

void CameraDevice::set_af_area_position()
{
    load_properties();

    if (1 == m_prop.position_key_setting.writable) {
        // Set, PriorityKeySettings property
        tout << std::endl << "Set camera to PC remote" << std::endl;
        SDK::CrDeviceProperty priority;
        priority.SetCode(SDK::CrDevicePropertyCode::CrDeviceProperty_PriorityKeySettings);
        priority.SetCurrentValue(SDK::CrPriorityKeySettings::CrPriorityKey_PCRemote);
        priority.SetValueType(SDK::CrDataType::CrDataType_UInt32Array);
        auto err_priority = SDK::SetDeviceProperty(m_device_handle, &priority);
        if (CR_FAILED(err_priority)) {
            tout << "Priority Key setting FAILED\n";
            return;
        }
        std::this_thread::sleep_for(500ms);
        get_position_key_setting();
    }

    // Set, ExposureProgramMode property
    tout << std::endl << "Set the Exposure Program mode to P Auto" << std::endl;
    SDK::CrDeviceProperty expromode;
    expromode.SetCode(SDK::CrDevicePropertyCode::CrDeviceProperty_ExposureProgramMode);
    expromode.SetCurrentValue(SDK::CrExposureProgram::CrExposure_P_Auto);
    expromode.SetValueType(SDK::CrDataType::CrDataType_UInt16Array);
    SDK::CrError err_expromode;
    bool execStat = false;
    int i = 0;
    while (i < 5)
    {
        err_expromode = SDK::SetDeviceProperty(m_device_handle, &expromode);
        if (CR_FAILED(err_expromode)) {
            tout << "Exposure Program mode FAILED\n";
            return;
        }
        std::this_thread::sleep_for(1000ms);
        get_exposure_program_mode();
        if (m_prop.exposure_program_mode.current == SDK::CrExposureProgram::CrExposure_P_Auto) {
            execStat = true;
            break;
        }
        i++;
    }
    if (false == execStat)
    {
        tout << std::endl << "Exposure Program mode FAILED\n";
        return;
    }

    if (1 != m_prop.focus_area.writable) {
        tout << "Focus Area is not writable\n";
        return;
    }

    tout << "Exposure Program mode SUCCESS.\n";

    // Set, FocusArea property
    tout << std::endl << "Set Focus Area to Flexible_Spot_S\n";

    SDK::CrDeviceProperty prop;
    prop.SetCode(SDK::CrDevicePropertyCode::CrDeviceProperty_FocusArea);
    prop.SetCurrentValue(SDK::CrFocusArea::CrFocusArea_Flexible_Spot_S);
    prop.SetValueType(SDK::CrDataType::CrDataType_UInt16Array);

    auto& values = m_prop.focus_area.possible;
    if(find(values.begin(), values.end(), SDK::CrFocusArea::CrFocusArea_Flexible_Spot_S) != values.end()) {
        prop.SetCurrentValue(SDK::CrFocusArea::CrFocusArea_Flexible_Spot_S);
    }
    else {
        tout << "Focus Area: Flexible_Spot_S is invalid.\n";
        tout << "Please confirm Focus Area Limit Setting in Camera Menu.\n";
        return;
    }

    auto err_prop = SDK::SetDeviceProperty(m_device_handle, &prop);
    execStat = false;
    i = 0;
    while (i < 5)
    {
        err_expromode = SDK::SetDeviceProperty(m_device_handle, &prop);
        if (CR_FAILED(err_prop)) {
            tout << "Focus Area FAILED\n";
            return;
        }
        std::this_thread::sleep_for(1000ms);
        get_focus_area();
        if (m_prop.focus_area.current == SDK::CrFocusArea::CrFocusArea_Flexible_Spot_S) {
            execStat = true;
            break;
        }
        i++;
    }
    if (false == execStat)
    {
        tout << "Focus Area FAILED\n";
        return;
    }
    tout << "Focus Area SUCCESS\n";
    get_af_area_position();


    execute_pos_xy(SDK::CrDevicePropertyCode::CrDeviceProperty_AF_Area_Position);
}

void CameraDevice::set_select_media_format()
{
    bool validQuickFormat = false;
    SDK::CrCommandId ptpFormatType = SDK::CrCommandId::CrCommandId_MediaFormat;

    if ((SDK::CrMediaFormat::CrMediaFormat_Disable == m_prop.media_slot1_full_format_enable_status.current) &&
        (SDK::CrMediaFormat::CrMediaFormat_Disable == m_prop.media_slot2_full_format_enable_status.current)) {
            // Not a settable property
        tout << std::endl << "Slot1 and Slot2 can not format\n";
        return;
    }

    if ((-1 != m_prop.media_slot1_quick_format_enable_status.writable || -1 != m_prop.media_slot2_quick_format_enable_status.writable)
        &&
         ((SDK::CrMediaFormat::CrMediaFormat_Enable == m_prop.media_slot1_quick_format_enable_status.current) ||
          (SDK::CrMediaFormat::CrMediaFormat_Enable == m_prop.media_slot2_quick_format_enable_status.current))) {
            validQuickFormat = true;
    }

    text input;
    tout << std::endl << "Would you like to format the media? (y/n): ";
    std::getline(tin, input);
    if (input != TEXT("y")) {
        tout << "Skip format.\n";
        return;
    }

    // Full or Quick
    if (validQuickFormat) {
        tout << "Choose a format type number:" << std::endl;
        tout << "[-1] Cancel input" << std::endl;
        tout << "[1] Full Format" << std::endl;
        tout << "[2] Quick Format" << std::endl;

        tout << std::endl << "input> ";
        std::getline(tin, input);
        text_stringstream sstype(input);
        int selected_type = 0;
        sstype >> selected_type;

        if ((selected_type < 1) || (2 < selected_type)) {
            tout << "Input cancelled.\n";
            return;
        }

        if (2 == selected_type) {
            ptpFormatType = SDK::CrCommandId::CrCommandId_MediaQuickFormat;
        }
    }

    tout << std::endl << "Choose a number Which media do you want to format ? \n";
    tout << "[-1] Cancel input\n";

    tout << "[1] SLOT1" << '\n';
    tout << "[2] SLOT2" << '\n';

    tout << std::endl << "input> ";
    std::getline(tin, input);
    text_stringstream ss(input);
    int selected_index = 0;
    ss >> selected_index;

    if ((selected_index < 1) || (2 < selected_index)) {
        tout << "Input cancelled.\n";
        return;
    }

    tout << std::endl << "All data will be deleted. Is it OK ? (y/n): ";
    std::getline(tin, input);
    if (input != TEXT("y")) {
        tout << "Skip format.\n";
        return;
    }

    // Get the latest status
    get_select_media_format();

    CrInt64u ptpValue = 0xFFFF;
    if (SDK::CrCommandId::CrCommandId_MediaQuickFormat == ptpFormatType) {
        if ((1 == selected_index) && (SDK::CrMediaFormat::CrMediaFormat_Enable == m_prop.media_slot1_quick_format_enable_status.current)) {
        ptpValue = SDK::CrCommandParam::CrCommandParam_Up;
        }
        else if ((2 == selected_index) && (SDK::CrMediaFormat::CrMediaFormat_Enable == m_prop.media_slot2_quick_format_enable_status.current)) {
            ptpValue = SDK::CrCommandParam::CrCommandParam_Down;
        }
    }
    else
    {
        if ((1 == selected_index) && (SDK::CrMediaFormat::CrMediaFormat_Enable == m_prop.media_slot1_full_format_enable_status.current)) {
            ptpValue = SDK::CrCommandParam::CrCommandParam_Up;
        }
        else if ((2 == selected_index) && (SDK::CrMediaFormat::CrMediaFormat_Enable == m_prop.media_slot2_full_format_enable_status.current)) {
            ptpValue = SDK::CrCommandParam::CrCommandParam_Down;
        }
    }

    if (0xFFFF == ptpValue)
    {
        tout << std::endl << "The Selected slot cannot be formatted.\n";
        return;
    }

    SDK::SendCommand(m_device_handle, ptpFormatType, (SDK::CrCommandParam)ptpValue);

    tout << std::endl << "Formatting .....\n";

    int startflag = 0;
    CrInt32u getCodes = SDK::CrDevicePropertyCode::CrDeviceProperty_Media_FormatProgressRate;

    std::int32_t nprop = 0;
    SDK::CrDeviceProperty* prop_list = nullptr;

    // check of progress
    while (true)
    {
        auto status = SDK::GetSelectDeviceProperties(m_device_handle, 1, &getCodes, &prop_list, &nprop);
        if (CR_FAILED(status)) {
            tout << "Failed to get Media FormatProgressRate.\n";
            return;
        }
        if (prop_list && 1 == nprop) {
            auto prop = prop_list[0];
        
            if (getCodes == prop.GetCode())
            {
                if ((0 == startflag) && (0 < prop.GetCurrentValue()))
                {
                    startflag = 1;
                }
                if ((1 == startflag) && (0 == prop.GetCurrentValue()))
                {
                    tout << std::endl << "Format completed " << '\n';
                    SDK::ReleaseDeviceProperties(m_device_handle, prop_list);
                    prop_list = nullptr;
                    break;
                }
                tout << "\r" << "FormatProgressRate: " << prop.GetCurrentValue();
            }
        }
        std::this_thread::sleep_for(250ms);
        SDK::ReleaseDeviceProperties(m_device_handle, prop_list);
        prop_list = nullptr;
    }
}

void CameraDevice::execute_movie_rec()
{
    load_properties();

    text input;
    tout << std::endl << "Operate the movie recording button ? (y/n): ";
    std::getline(tin, input);
    if (input != TEXT("y")) {
        tout << "Skip .\n";
        return;
    }

    tout << "Choose a number:\n";
    tout << "[-1] Cancel input\n";

    tout << "[1] Up" << '\n';
    tout << "[2] Down" << '\n';

    tout << "[-1] Cancel input\n";
    tout << "Choose a number:\n";

    tout << std::endl << "input> ";
    std::getline(tin, input);
    text_stringstream ss(input);
    int selected_index = 0;
    ss >> selected_index;

    if (selected_index < 0) {
        tout << "Input cancelled.\n";
        return;
    }

    CrInt64u ptpValue = 0;
    switch (selected_index) {
    case 1:
        ptpValue = SDK::CrCommandParam::CrCommandParam_Up;
        break;
    case 2:
        ptpValue = SDK::CrCommandParam::CrCommandParam_Down;
        break;
    default:
        selected_index = -1;
        break;
    }

    if (-1 == selected_index) {
        tout << "Input cancelled.\n";
        return;
    }

    SDK::SendCommand(m_device_handle, SDK::CrCommandId::CrCommandId_MovieRecord, (SDK::CrCommandParam)ptpValue);

}

void CameraDevice::set_custom_wb()
{
    load_properties();
    if (-1 == m_prop.customwb_capture_execution_state.writable) {
        tout << "Custom WB Capture is Not Supported.\n";
        return ;
    }
    if (1 == m_prop.position_key_setting.writable) {
        // Set, PriorityKeySettings property
        tout << std::endl << "Set camera to PC remote" << std::endl;
        SDK::CrDeviceProperty priority;
        priority.SetCode(SDK::CrDevicePropertyCode::CrDeviceProperty_PriorityKeySettings);
        priority.SetCurrentValue(SDK::CrPriorityKeySettings::CrPriorityKey_PCRemote);
        priority.SetValueType(SDK::CrDataType::CrDataType_UInt32Array);
        auto err_priority = SDK::SetDeviceProperty(m_device_handle, &priority);
        if (CR_FAILED(err_priority)) {
            tout << "Priority Key setting FAILED\n";
            return;
        }
        std::this_thread::sleep_for(500ms);
        get_position_key_setting();
    }

    // Set, ExposureProgramMode property
    tout << std::endl << "Set the Exposure Program mode to P mode" << std::endl;
    SDK::CrDeviceProperty expromode;
    expromode.SetCode(SDK::CrDevicePropertyCode::CrDeviceProperty_ExposureProgramMode);
    expromode.SetCurrentValue(SDK::CrExposureProgram::CrExposure_P_Auto);
    expromode.SetValueType(SDK::CrDataType::CrDataType_UInt16Array);
    SDK::CrError err_expromode;
    bool execStat = false;
    int i = 0;
    while (i < 5)
    {
        err_expromode = SDK::SetDeviceProperty(m_device_handle, &expromode);
        if (CR_FAILED(err_expromode)) {
            tout << "Exposure Program mode FAILED\n";
            return;
        }
        std::this_thread::sleep_for(1000ms);
        get_exposure_program_mode();
        if (m_prop.exposure_program_mode.current == SDK::CrExposureProgram::CrExposure_P_Auto) {
            execStat = true;
            break;
        }
        i++;
    }
    if (false == execStat)
    {
        tout << std::endl << "Exposure Program mode FAILED\n";
        return;
    }
    tout << "Exposure Program mode SUCCESS.\n";

    // Set, White Balance property
    tout << std::endl << "Set the White Balance to Custom1\n";
    SDK::CrDeviceProperty wb;
    wb.SetCode(SDK::CrDevicePropertyCode::CrDeviceProperty_WhiteBalance);
    wb.SetCurrentValue(SDK::CrWhiteBalanceSetting::CrWhiteBalance_Custom_1);
    wb.SetValueType(SDK::CrDataType::CrDataType_UInt16Array);
    auto err_wb = SDK::SetDeviceProperty(m_device_handle, &wb);
    if (CR_FAILED(err_wb)) {
        tout << "White Balance FAILED\n";
        return;
    }
    std::this_thread::sleep_for(2000ms);
    get_white_balance();

    execStat = false;
    // Set, custom WB capture standby 
    tout << std::endl << "Set custom WB capture standby " << std::endl;

    i = 0;
    while ((false == execStat) && (i < 5))
    {
        execute_downup_property(SDK::CrDevicePropertyCode::CrDeviceProperty_CustomWB_Capture_Standby);
        std::this_thread::sleep_for(1000ms);
        tout << std::endl;
        execStat = get_custom_wb();
        i++;

    }

    if (false == execStat)
    {
        tout << std::endl << "CustomWB Capture Standby FAILED\n";
        return;
    }

    // Set, custom WB capture 
    tout << std::endl << "Set custom WB capture ";
    execute_pos_xy(SDK::CrDevicePropertyCode::CrDeviceProperty_CustomWB_Capture);

    std::this_thread::sleep_for(5000ms);

    // Set, custom WB capture standby cancel 
    text input;
    tout << std::endl << "Set custom WB capture standby cancel. Please enter something. " << std::endl;
    std::getline(tin, input);
    if (0 == input.size() || 0 < input.size()) {
        execute_downup_property(SDK::CrDevicePropertyCode::CrDeviceProperty_CustomWB_Capture_Standby_Cancel);
        get_custom_wb();
        tout << std::endl << "Finish custom WB capture\n";
    }
    else
    {
        tout << std::endl << "Did not finish normally\n";
    }
}

void CameraDevice::set_zoom_operation()
{
    text input;
    tout << std::endl << "Operate the zoom ? (y/n): ";
    std::getline(tin, input);
    if (input != TEXT("y")) {
        tout << "Skip .\n";
        return;
    }
    // Get the latest value
    load_properties();
    while (true)
    {
        CrInt64 ptpValue = 0;
        bool cancel = false;

        // Zoom Speed Range is not supported
        if (m_prop.zoom_speed_range.possible.size() < 2) {
            tout << std::endl << "Choose a number:\n";
            tout << "[-1] Cancel input\n";

            tout << "[0] Stop" << '\n';
            tout << "[1] Wide" << '\n';
            tout << "[2] Tele" << '\n';

            tout << "[-1] Cancel input\n";
            tout << "Choose a number:\n";

            tout << std::endl << "input> ";
            std::getline(tin, input);
            text_stringstream ss(input);
            int selected_index = 0;
            ss >> selected_index;

            switch (selected_index) {
            case 0:
                ptpValue = SDK::CrZoomOperation::CrZoomOperation_Stop;
                break;
            case 1:
                ptpValue = SDK::CrZoomOperation::CrZoomOperation_Wide;
                break;
            case 2:
                ptpValue = SDK::CrZoomOperation::CrZoomOperation_Tele;
                break;
            default:
                tout << "Input cancelled.\n";
                return;
                break;
            }
        }
        else{
            tout << std::endl << "Set the value of zoom speed (Out-of-range value to Cancel):\n";
            tout << std::endl << "input> ";
            std::getline(tin, input);
            text_stringstream ss(input);
            int input_value = 0;
            ss >> input_value;
            // Get the latest value
            load_properties();
            //Stop zoom and return to the top menu when out-of-range values or non-numeric values are entered
            if (((input_value == 0) && (input != TEXT("0"))) || (input_value < (int)m_prop.zoom_speed_range.possible.at(0)) || ((int)m_prop.zoom_speed_range.possible.at(1) < input_value))
            {
                cancel = true;
                ptpValue = SDK::CrZoomOperation::CrZoomOperation_Stop;
                tout << "Input cancelled.\n";
            }
            else {
                ptpValue = (CrInt64)input_value;
            }
        }
        if (SDK::CrZoomOperationEnableStatus::CrZoomOperationEnableStatus_Enable != m_prop.zoom_operation_status.current) {
            tout << "Zoom Operation is not executable.\n";
            return;
        }
        SDK::CrDeviceProperty prop;
        prop.SetCode(SDK::CrDevicePropertyCode::CrDeviceProperty_Zoom_Operation);
        prop.SetCurrentValue((CrInt64u)ptpValue);
        prop.SetValueType(SDK::CrDataType::CrDataType_UInt16Array);
        SDK::SetDeviceProperty(m_device_handle, &prop);
        if (cancel == true) {
            return;
        }
        get_zoom_operation();
    }
}

void CameraDevice::set_remocon_zoom_speed_type()
{
    if (1 != m_prop.remocon_zoom_speed_type.writable) {
        // Not a settable property
        tout << "Zoom speed type is not writable\n";
        return;
    }

    text input;
    tout << "Would you like to set a new zoom speed type value? (y/n): ";
    std::getline(tin, input);
    if (input != TEXT("y")) {
        tout << "Skip setting a new value.\n";
        return;
    }

    tout << "Choose a number set a new zoom speed type value:\n";
    tout << "[-1] Cancel input\n";

    auto& values = m_prop.remocon_zoom_speed_type.possible;

    for (std::size_t i = 0; i < values.size(); ++i) {
        tout << '[' << i << "] " << format_remocon_zoom_speed_type(values[i]) << '\n';
    }

    tout << "[-1] Cancel input\n";
    tout << "Choose a number set a new zoom speed type value:\n" << std::endl;

    tout << "input> ";
    std::getline(tin, input);
    text_stringstream ss(input);
    int selected_index = 0;
    ss >> selected_index;

    if (selected_index < 0 || values.size() <= selected_index) {
        tout << "Input cancelled.\n";
        return;
    }

    SDK::CrDeviceProperty prop;
    prop.SetCode(SDK::CrDevicePropertyCode::CrDeviceProperty_Remocon_Zoom_Speed_Type);
    prop.SetCurrentValue(values[selected_index]);
    prop.SetValueType(SDK::CrDataType::CrDataType_UInt32Array);

    SDK::SetDeviceProperty(m_device_handle, &prop);
}

bool CameraDevice::set_drive_mode(CrInt64u Value)
{
    SDK::CrDeviceProperty mode;
    mode.SetCode(SDK::CrDevicePropertyCode::CrDeviceProperty_DriveMode);
    mode.SetCurrentValue(Value);
    mode.SetValueType(SDK::CrDataType::CrDataType_UInt32Array);
    auto err_still_capture_mode = SDK::SetDeviceProperty(m_device_handle, &mode);
    if (CR_FAILED(err_still_capture_mode)) {
        return false;
    }
    else {
        return true;
    }
}

void CameraDevice::execute_camera_setting_reset()
{
    load_properties();
    if (SDK::CrCameraSettingsResetEnableStatus::CrCameraSettingsReset_Enable == m_prop.camera_setting_reset_enable_status.current) {
        tout << "Camera Setting Reset Enable Status: Enable \n";
    } else {
        tout << "Camera Setting Reset not executable.\n";
        return;
    }

    text input;
    tout << std::endl << "The camera settings will be initialized! Are you sure you want to do this ? (y/n): ";
    std::getline(tin, input);
    if (input != TEXT("y")) {
        tout << "Skip.\n";
        return;
    }
    // Get the latest status
    load_properties();
    if (SDK::CrCameraSettingsResetEnableStatus::CrCameraSettingsReset_Enable != m_prop.camera_setting_reset_enable_status.current) {
        tout << "Camera Setting Reset not executable.\n";
        return;
    }
    SDK::CrCommandId ptpFormatType = SDK::CrCommandId::CrCommandId_CameraSettingsReset;
    SDK::SendCommand(m_device_handle, ptpFormatType, SDK::CrCommandParam::CrCommandParam_Down);
    std::this_thread::sleep_for(35ms);
    SDK::SendCommand(m_device_handle, ptpFormatType, SDK::CrCommandParam::CrCommandParam_Up);
}

void CameraDevice::set_playback_media()
{
    if (false == get_playback_media())
        return;

    if (1 != m_prop.playback_media.writable) {
        tout << "Playback Media is not writable\n";
        return;
    }

    text input;
    tout << "Would you like to set a new Playback Media value? (y/n): ";
    std::getline(tin, input);
    if (input != TEXT("y")) {
        tout << "Skip setting a new value.\n";
        return;
    }

    tout << "Choose a number set a new Playback Media value:\n";
    tout << "[-1] Cancel input\n";
    tout << "[1] Slot1\n";
    tout << "[2] Slot2\n";
    tout << "[-1] Cancel input\n";
    tout << "Choose a number set a new Playback Media value:\n" << std::endl;

    tout << "input> ";
    std::getline(tin, input);
    text_stringstream ss(input);
    int selected_index = 0;
    ss >> selected_index;
    if (selected_index < 1 || 2 < selected_index) {
        tout << "Input cancelled.\n";
        return;
    }
    CrInt8u ptpValue= selected_index;
    SDK::CrDeviceProperty prop;
    prop.SetCode(SDK::CrDevicePropertyCode::CrDeviceProperty_PlaybackMedia);
    prop.SetCurrentValue((CrInt64u)ptpValue);
    prop.SetValueType(SDK::CrDataType::CrDataType_UInt8Array);
    SDK::SetDeviceProperty(m_device_handle, &prop);
}

void CameraDevice::set_gain_base_sensitivity()
{
    if (false == get_gain_base_sensitivity())
        return;

    if (1 != m_prop.gain_base_sensitivity.writable) {
        tout << "Gain Base Sensitivity is not writable\n";
        return;
    }

    text input;
    tout << "Would you like to set a new Gain Base Sensitivity value? (y/n): ";
    std::getline(tin, input);
    if (input != TEXT("y")) {
        tout << "Skip setting a new value.\n";
        return;
    }

    tout << "Choose a number set a new Gain Base Sensitivity Setting value:\n";
    tout << "[-1] Cancel input\n";
    tout << "[1] High Level  \n";
    tout << "[2] Low Level \n";
    tout << "[-1] Cancel input\n";
    tout << "Choose a number set a new Gain Base Sensitivity Setting value:\n" << std::endl;

    tout << "input> ";
    std::getline(tin, input);
    text_stringstream ss(input);
    int selected_index = 0;
    ss >> selected_index;

    if (selected_index < 1 ||2 < selected_index) {
        tout << "Input cancelled.\n";
        return;
    }
    CrInt8u ptpValue = selected_index;

    SDK::CrDeviceProperty prop;
    prop.SetCode(SDK::CrDevicePropertyCode::CrDeviceProperty_GainBaseSensitivity);
    prop.SetCurrentValue((CrInt64u)ptpValue);
    prop.SetValueType(SDK::CrDataType::CrDataType_UInt8Array);
    SDK::SetDeviceProperty(m_device_handle, &prop);
}

void CameraDevice::set_gain_base_iso_sensitivity()
{
    if (false == get_gain_base_iso_sensitivity())
        return;

    if (1 != m_prop.gain_base_iso_sensitivity.writable) {
        // Not a settable property
        tout << "Gain Base ISO Sensitivity is not writable\n";
        return;
    }

    text input;
    tout << "Would you like to set a new Gain Base ISO Sensitivity value? (y/n): ";
    std::getline(tin, input);
    if (input != TEXT("y")) {
        tout << "Skip setting a new value.\n";
        return;
    }

    tout << "Choose a number set a new Gain Base ISO Sensitivity Setting value:\n";
    tout << "[-1] Cancel input\n";
    tout << "[1] High Level  \n";
    tout << "[2] Low Level \n";
    tout << "[-1] Cancel input\n";
    tout << "Choose a number set a new Gain Base ISO Sensitivity Setting value:\n" << std::endl;

    tout << "input> ";
    std::getline(tin, input);
    text_stringstream ss(input);
    int selected_index = 0;
    ss >> selected_index;

    if (selected_index < 1 || 2 < selected_index) {
        tout << "Input cancelled.\n";
        return;
    }
    CrInt8u ptpValue = selected_index;

    SDK::CrDeviceProperty prop;
    prop.SetCode(SDK::CrDevicePropertyCode::CrDeviceProperty_GainBaseIsoSensitivity);
    prop.SetCurrentValue((CrInt64u)ptpValue);
    prop.SetValueType(SDK::CrDataType::CrDataType_UInt8Array);
    SDK::SetDeviceProperty(m_device_handle, &prop);
}

void CameraDevice::set_monitor_lut_setting()
{
    if (false == get_monitor_lut_setting())
        return;

    if (1 != m_prop.monitor_lut_setting.writable) {
        tout << "Monitor LUT Setting is not writable\n";
        return;
    }

    text input;
    tout << "Would you like to set a new Monitor LUT Setting? (y/n): ";
    std::getline(tin, input);
    if (input != TEXT("y")) {
        tout << "Skip setting a new value.\n";
        return;
    }

    tout << "Choose a number set a new Monitor LUT Setting value:\n";
    tout << "[-1] Cancel input\n";
    tout << "[1] OFF\n";
    tout << "[2] ON\n";
    tout << "[-1] Cancel input\n";
    tout << "Choose a number set a new Monitor LUT Setting value:\n" << std::endl;

    tout << "input> ";
    std::getline(tin, input);
    text_stringstream ss(input);
    int selected_index = 0;
    ss >> selected_index;
    if (selected_index < 1 || 2 < selected_index) {
        tout << "Input cancelled.\n";
        return;
    }
    CrInt8u ptpValue = selected_index;
    SDK::CrDeviceProperty prop;
    prop.SetCode(SDK::CrDevicePropertyCode::CrDeviceProperty_MonitorLUTSetting);
    prop.SetCurrentValue((CrInt64u)ptpValue);
    prop.SetValueType(SDK::CrDataType::CrDataType_UInt8Array);

    SDK::SetDeviceProperty(m_device_handle, &prop);
}

void CameraDevice::set_exposure_index()
{
    if (false == get_exposure_index())
        return;

    if (1 != m_prop.exposure_index.writable) {
        tout << "Exposure Index is not writable\n";
        return;
    }

    text input;
    tout << "Would you like to set a new Exposure Index value? (y/n): ";
    std::getline(tin, input);
    if (input != TEXT("y")) {
        tout << "Skip setting a new value.\n";
        return;
    }

    tout << "Choose a number set a new Exposure Index:\n";
    tout << "[-1] Cancel input\n";

    auto& values = m_prop.exposure_index.possible;
    for (std::size_t i = 0; i < values.size(); ++i) {
        tout << '[' << i << "] " << values[i] << '\n';
    }
    tout << "[-1] Cancel input\n";
    tout << "Choose a number set a new Exposure Index:\n" << std::endl;

    tout << "input> ";
    std::getline(tin, input);
    text_stringstream ss(input);
    int selected_index = 0;
    ss >> selected_index;

    if (selected_index < 0 || values.size() <= selected_index) {
        tout << "Input cancelled.\n";
        return;
    }

    SDK::CrDeviceProperty prop;
    prop.SetCode(SDK::CrDevicePropertyCode::CrDeviceProperty_ExposureIndex);
    prop.SetCurrentValue(values[selected_index]);
    prop.SetValueType(SDK::CrDataType::CrDataType_UInt16Array);
    SDK::SetDeviceProperty(m_device_handle, &prop);
}

void CameraDevice::set_baselook_value()
{
    if (false == get_baselook_value())
        return;

    if (1 != m_prop.baselook_value.writable) {
        tout << "BaseLook Value is not writable\n";
        return;
    }

    text input;
    tout << "Would you like to set a new BaseLook value? (y/n): ";
    std::getline(tin, input);
    if (input != TEXT("y")) {
        tout << "Skip setting a new value.\n";
        return;
    }

    tout << "Choose a number set a new BaseLook value:\n";
    tout << "[-1] Cancel input\n";
    auto& values = m_prop.baselook_value.possible;

    //If you want to display a string, you need to execute RequestDisplayStringList() in advance.
    SDK::CrDisplayStringListInfo* pProperty = nullptr;
    SDK::CrDisplayStringType type;
    CrInt32u numOfList = 0;

    SDK::CrError err = SDK::GetDisplayStringList(m_device_handle, SDK::CrDisplayStringType_BaseLook_Name_Display, &pProperty, &numOfList);

    if (CR_SUCCEEDED(err) && numOfList != 0) {
        for (std::size_t i = 0; i < values.size(); ++i) {
            for (int j = 0; j < numOfList; ++j) {
                if (pProperty[j].value == values[i]) {
                    tout << '[' << i << "] " << format_dispstrlist(pProperty[j])<< '\n';
                    break;
                }
            }
        }
        ReleaseDisplayStringList(m_device_handle, pProperty);
    }
    else {
        CrInt8u baseValueSetter;
        CrInt8u get_baseLookValue;
        for (std::size_t i = 0; i < values.size(); ++i) {
            baseValueSetter = (values[i] >> 8 ) & 0x01;
            get_baseLookValue = values[i] & 0xFF;
            tout << '[' << i << "] Index" << (int)get_baseLookValue << " " << format_baselook_value(baseValueSetter) << '\n';
        }
    }

    tout << "[-1] Cancel input\n";
    tout << "Choose a number set a new BaseLook value:\n" << std::endl;
    tout << "input> ";
    std::getline(tin, input);
    text_stringstream ss(input);
    int selected_index = 0;
    ss >> selected_index;

    if (selected_index < 0 || values.size() <= selected_index) {
        tout << "Input cancelled.\n";
        return;
    }

    SDK::CrDeviceProperty prop;
    prop.SetCode(SDK::CrDevicePropertyCode::CrDeviceProperty_BaseLookValue);
    prop.SetCurrentValue(values[selected_index]);
    prop.SetValueType(SDK::CrDataType::CrDataType_UInt16Array);
    SDK::SetDeviceProperty(m_device_handle, &prop);
 }

void CameraDevice::set_iris_mode_setting()
{
    if (false == get_iris_mode_setting())
        return;

    if (1 != m_prop.iris_mode_setting.writable) {
        // Not a settable property
        tout << "Iris Mode Setting is not writable\n";
        return;
    }

    text input;
    tout << "Would you like to set a new Iris Mode Setting value? (y/n): ";
    std::getline(tin, input);
    if (input != TEXT("y")) {
        tout << "Skip setting a new value.\n";
        return;
    }

    tout << "Choose a number set a new Iris Mode Setting value:\n";
    tout << "[-1] Cancel input\n";
    tout << "[1] Automatic \n";
    tout << "[2] Manual \n";
    tout << "[-1] Cancel input\n";
    tout << "Choose a number set a new Iris Mode Setting value:\n" << std::endl;

    tout << "input> ";
    std::getline(tin, input);
    text_stringstream ss(input);
    int selected_index = 0;
    ss >> selected_index;

    if (selected_index < 1 || 2 < selected_index) {
        tout << "Input cancelled.\n";
        return;
    }
    CrInt8u ptpValue = selected_index;

    SDK::CrDeviceProperty prop;
    prop.SetCode(SDK::CrDevicePropertyCode::CrDeviceProperty_IrisModeSetting);
    prop.SetCurrentValue((CrInt64u)ptpValue);
    prop.SetValueType(SDK::CrDataType::CrDataType_UInt8Array);
    SDK::SetDeviceProperty(m_device_handle, &prop);
}

void CameraDevice::set_shutter_mode_setting()
{
    if (false == get_shutter_mode_setting())
        return;

    if (1 != m_prop.shutter_mode_setting.writable) {
        // Not a settable property
        tout << "Shutter Mode Setting is not writable\n";
        return;
    }

    text input;
    tout << "Would you like to set a new Shutter Mode Setting value? (y/n): ";
    std::getline(tin, input);
    if (input != TEXT("y")) {
        tout << "Skip setting a new value.\n";
        return;
    }

    tout << "Choose a number set a new Shutter Mode Setting value:\n";
    tout << "[-1] Cancel input\n";
    tout << "[1] Automatic \n";
    tout << "[2] Manual \n";
    tout << "[-1] Cancel input\n";
    tout << "Choose a number set a new Shutter Mode Setting value:\n" << std::endl;

    tout << "input> ";
    std::getline(tin, input);
    text_stringstream ss(input);
    int selected_index = 0;
    ss >> selected_index;

    if (selected_index < 1 || 2 < selected_index) {
        tout << "Input cancelled.\n";
        return;
    }
    CrInt8u ptpValue = selected_index;

    SDK::CrDeviceProperty prop;
    prop.SetCode(SDK::CrDevicePropertyCode::CrDeviceProperty_ShutterModeSetting);
    prop.SetCurrentValue((CrInt64u)ptpValue);
    prop.SetValueType(SDK::CrDataType::CrDataType_UInt8Array);
    SDK::SetDeviceProperty(m_device_handle, &prop);
}

void CameraDevice::set_exposure_control_type()
{
    if (false == get_exposure_control_type())
        return;

    if (1 != m_prop.exposure_control_type.writable) {
        // Not a settable property
        tout << "Exposure Control Type is not writable\n";
        return;
    }

    text input;
    tout << "Would you like to set a new Exposure Control Type is value? (y/n): ";
    std::getline(tin, input);
    if (input != TEXT("y")) {
        tout << "Skip setting a new value.\n";
        return;
    }

    tout << "Choose a number set a new Exposure Control Type is value:\n";
    tout << "[-1] Cancel input\n";
    tout << "[1] P/A/S/M Mode \n";
    tout << "[2] Flexible Exposure Mode \n";
    tout << "[-1] Cancel input\n";
    tout << "Choose a number set a new Exposure Control Type is:\n" << std::endl;

    tout << "input> ";
    std::getline(tin, input);
    text_stringstream ss(input);
    int selected_index = 0;
    ss >> selected_index;

    if (selected_index < 1 || 2 < selected_index) {
        tout << "Input cancelled.\n";
        return;
    }
    CrInt8u ptpValue = selected_index;

    SDK::CrDeviceProperty prop;
    prop.SetCode(SDK::CrDevicePropertyCode::CrDeviceProperty_ExposureCtrlType);
    prop.SetCurrentValue((CrInt64u)ptpValue);
    prop.SetValueType(SDK::CrDataType::CrDataType_UInt8Array);
    SDK::SetDeviceProperty(m_device_handle, &prop);
}

void CameraDevice::set_recording_setting()
{
    if (false == get_recording_setting())
        return;

    if (1 != m_prop.recording_setting.writable) {
        // Not a settable property
        tout << "Recording Setting is not writable\n";
        return;
    }

    text input;
    tout << "Would you like to set a new Recording Setting is value? (y/n): ";
    std::getline(tin, input);
    if (input != TEXT("y")) {
        tout << "Skip setting a new value.\n";
        return;
    }

    tout << "Choose a number set a new Recording Setting is value:\n";
    tout << "[-1] Cancel input\n";

    auto& values = m_prop.recording_setting.possible;
    for (std::size_t i = 0; i < values.size(); ++i) {
        tout << '[' << i << "] " << format_recording_setting(values[i]) << '\n';
    }

    tout << "[-1] Cancel input\n";
    tout << "Choose a number set Recording Setting is value:\n";

    tout << "input> ";
    std::getline(tin, input);
    text_stringstream ss(input);
    int selected_index = 0;
    ss >> selected_index;

    if (selected_index < 0 || values.size() <= selected_index) {
        tout << "Input cancelled.\n";
        return;
    }

    SDK::CrDeviceProperty prop;
    prop.SetCode(SDK::CrDevicePropertyCode::CrDeviceProperty_Movie_Recording_Setting);
    prop.SetCurrentValue(values[selected_index]);
    prop.SetValueType(SDK::CrDataType::CrDataType_UInt16Array);

    SDK::SetDeviceProperty(m_device_handle, &prop);
}

void CameraDevice::set_gain_control_setting()
{
    if (false == get_gain_control_setting())
        return;

    if (1 != m_prop.gain_control_setting.writable) {
        // Not a settable property
        tout << "Gain Control Setting is not writable\n";
        return;
    }

    text input;
    tout << "Would you like to set a new Gain Control Setting value? (y/n): ";
    std::getline(tin, input);
    if (input != TEXT("y")) {
        tout << "Skip setting a new value.\n";
        return;
    }

    tout << "Choose a number set a new Gain Control Setting value:\n";
    tout << "[-1] Cancel input\n";
    tout << "[1] Automatic \n";
    tout << "[2] Manual \n";
    tout << "[-1] Cancel input\n";
    tout << "Choose a number set a new Gain Control Setting value:\n" << std::endl;

    tout << "input> ";
    std::getline(tin, input);
    text_stringstream ss(input);
    int selected_index = 0;
    ss >> selected_index;

    if (selected_index < 0 || 2 < selected_index) {
        tout << "Input cancelled.\n";
        return;
    }
    CrInt8u ptpValue = selected_index;

    SDK::CrDeviceProperty prop;
    prop.SetCode(SDK::CrDevicePropertyCode::CrDeviceProperty_GainControlSetting);
    prop.SetCurrentValue((CrInt64u)ptpValue);
    prop.SetValueType(SDK::CrDataType::CrDataType_UInt8Array);
    SDK::SetDeviceProperty(m_device_handle, &prop);
}

void CameraDevice::set_dispmode()
{
    load_properties();
    if (-1 == m_prop.dispmode_candidate.writable||
        -1 == m_prop.dispmode_setting.writable||
        -1 == m_prop.dispmode.writable) {
        tout << "DISP Mode Setting is not supported\n";
        return ;
    }

    if (1 != m_prop.dispmode_setting.writable || 1 != m_prop.dispmode.writable) {
        // Not a settable property
        tout << "DISP Mode Setting is not writable\n";
        return;
    }

    tout << "DISP Mode: " << format_dispmode(m_prop.dispmode.current) << '\n';

    auto& disp_candidate_values = m_prop.dispmode_candidate.possible;
    CrInt32u set_dispmode_setting = 0;

    // Select all configurable Disp mode
    for (int i = 0; i < disp_candidate_values.size(); i++)
    {
        set_dispmode_setting |= disp_candidate_values[i];
    }

    SDK::CrDispMode set_dispmode = SDK::CrDispMode_DisplayAllInfo;

    if (m_prop.dispmode.current == SDK::CrDispMode_DisplayAllInfo) {
         set_dispmode = SDK::CrDispMode_Histogram;
     }

    text input;
    tout << "When set, it will switch to the next Disp Mode: "<< format_dispmode(set_dispmode)<<"\n\n";
    tout << "Would you like to set a new DISP Mode Setting is value? (y/n): ";
    std::getline(tin, input);
    if (input != TEXT("y")) {
        tout << "Skip setting a new value.\n";
        return;
    }

    // set Disp Mode Setting 
    SDK::CrDeviceProperty disp_state_prop;
    disp_state_prop.SetCode(SDK::CrDevicePropertyCode::CrDeviceProperty_DispModeSetting);
    disp_state_prop.SetCurrentValue((CrInt64u)set_dispmode_setting);
    disp_state_prop.SetValueType(SDK::CrDataType::CrDataType_UInt32Array);

    SDK::SetDeviceProperty(m_device_handle, &disp_state_prop);

    std::this_thread::sleep_for(500ms);

    // set Disp Mode
    SDK::CrDeviceProperty disp_mode_prop;
    disp_mode_prop.SetCode(SDK::CrDevicePropertyCode::CrDeviceProperty_DispMode);
    disp_mode_prop.SetCurrentValue((CrInt64u)set_dispmode);
    disp_mode_prop.SetValueType(SDK::CrDataType::CrDataType_UInt8Array);

    SDK::SetDeviceProperty(m_device_handle, &disp_mode_prop);
}

void CameraDevice::set_gain_db_value()
{
    if (false == get_gain_db_value())
        return;

    if (1 != m_prop.gain_db_value.writable) {
        // Not a settable property
        tout << "Gain dB Value is not writable\n";
        return;
    }

    text input;
    tout << "\nWould you like to set a Gain dB Value? (y/n): ";
    std::getline(tin, input);
    if (input != TEXT("y")) {
        tout << "Skip.\n";
        return;
    }
    tout << "Gain dB Value: " << (int)m_prop.gain_db_value.possible.at(0) << " to " << (int)m_prop.gain_db_value.possible.at(1) << std::endl;
    tout << std::endl << "input Number> ";
    std::getline(tin, input);
    text_stringstream ss(input);
    int num = 0;
    ss >> num;

    if ((num < (int)m_prop.gain_db_value.possible.at(0)) || ((int)m_prop.gain_db_value.possible.at(1) < num)) {
        tout << "Input cancelled.\n";
        return;
    }

    SDK::CrDeviceProperty prop;
    prop.SetCode(SDK::CrDevicePropertyCode::CrDeviceProperty_GaindBValue);
    prop.SetCurrentValue((CrInt64u)num);
    prop.SetValueType(SDK::CrDataType::CrDataType_Int8);

    SDK::SetDeviceProperty(m_device_handle, &prop);
}

void CameraDevice::set_white_balance_tint()
{
    if (false == get_white_balance_tint())
        return;

#if 0 // White Balance Tint cannot be set directly
    if (1 != m_prop.white_balance_tint.writable) {
        // Not a settable property
        tout << "White Balance Tint is not writable\n";
        return;
    }
#else
    if (1 != m_prop.white_balance_tint_step.writable) {
        // Not a settable property
        tout << "White Balance Tint Step is not writable\n";
        return;
    }
#endif

    text input;
    tout << "\nWould you like to set a White Balance Tint Step ? (y/n): ";
    std::getline(tin, input);
    if (input != TEXT("y")) {
        tout << "Skip.\n";
        return;
    }

    tout << std::endl << "Set a value within the White Balance Tint Range" << std::endl;
    tout << "input> ";
    std::getline(tin, input);
    text_stringstream ss(input);
    int num = 0;
    ss >> num;

    if (((int)m_prop.white_balance_tint_step.possible.at(0) > num) || ((int)m_prop.white_balance_tint_step.possible.at(1) < num)) {
        tout << "\nWhite Balance Tint Step is out of range.\n";
        return;
    }

    int finalValue = (int)m_prop.white_balance_tint.current + num;
    int backNum = num;

    // Do not exceed the minimum value
    if (finalValue < (int)m_prop.white_balance_tint.possible.at(0)) {
        // Set the value to reach the minumum value
        num = (int)m_prop.white_balance_tint.possible.at(0) - (int)m_prop.white_balance_tint.current;
        if (0 == num) {
            tout << "The set value is out of range." << std::endl;
            return;
        }
        tout << "Change the value to be set from " << backNum << " to " << num << "." << std::endl;
    }

    // Do not exceed the maximum value
    if ((int)m_prop.white_balance_tint.possible.at(1) < finalValue) {
        // Set the value to reach the maximum value
        num = (int)m_prop.white_balance_tint.possible.at(1) - (int)m_prop.white_balance_tint.current;
        if (0 == num) {
            tout << "The set value is out of range." << std::endl;
            return;
        }
        tout << "Change the value to be set from " << backNum << " to " << num << "." << std::endl;
    }

    SDK::CrDeviceProperty prop;
    prop.SetCode(SDK::CrDevicePropertyCode::CrDeviceProperty_WhiteBalanceTintStep);
    prop.SetCurrentValue((CrInt64u)num);
    prop.SetValueType(SDK::CrDataType::CrDataType_Int16);
    SDK::SetDeviceProperty(m_device_handle, &prop);
}

void CameraDevice::set_shutter_speed_value()
{
    if(false == get_shutter_speed_value())
        return;

    if (1 != m_prop.shutter_speed_value.writable) {
        // Not a settable property
        tout << "Shutter Speed Value is not writable\n";
        return;
    }

    text input;
    tout << "Would you like to set a new Shutter Speed value? (y/n): ";
    std::getline(tin, input);
    if (input != TEXT("y")) {
        tout << "Skip setting a new value.\n";
        return;
    }

    tout << "Choose a number set a new Shutter Speed value:\n";
    tout << "[-1] Cancel input\n";

    auto& values = m_prop.shutter_speed_value.possible;
    for (std::size_t i = 0; i < values.size(); ++i) {
        tout << '[' << i << "] " << format_shutter_speed_value(values[i]) << '\n';
    }

    tout << "[-1] Cancel input\n";
    tout << "Choose a number set a new Shutter Speed value:\n";

    tout << "input> ";
    std::getline(tin, input);
    text_stringstream ss(input);
    int selected_index = 0;
    ss >> selected_index;

    if (selected_index < 0 || values.size() <= selected_index) {
        tout << "Input cancelled.\n";
        return;
    }

    SDK::CrDeviceProperty prop;
    prop.SetCode(SDK::CrDevicePropertyCode::CrDeviceProperty_ShutterSpeedValue);
    prop.SetCurrentValue(values[selected_index]);
    prop.SetValueType(SDK::CrDataType::CrDataType_UInt64Array);
    SDK::SetDeviceProperty(m_device_handle, &prop);
}

void CameraDevice::set_focus_bracket_shot_num()
{
    if (false == get_focus_bracket_shot_num())
        return;

    if (1 != m_prop.focus_bracket_shot_num.writable) {
        tout << "Focus Bracket is not writable\n";
        return;
    }

    text input;
    tout << "\nWould you like to set a new Focus Bracket Shot Number value? (y/n): ";
    std::getline(tin, input);
    if (input != TEXT("y")) {
        tout << "Skip.\n";
        return;
    }

    tout << std::endl << "input Number> ";
    std::getline(tin, input);
    text_stringstream ss(input);
    CrInt32u num = 0;
    ss >> num;

    // range
    if (num < m_prop.focus_bracket_shot_num.possible.at(0) || m_prop.focus_bracket_shot_num.possible.at(1) < num)
    {
        tout << "Input cancelled.\n";
        return;
    }

    SDK::CrDeviceProperty prop;
    prop.SetCode(SDK::CrDevicePropertyCode::CrDeviceProperty_FocusBracketShotNumber);
    prop.SetCurrentValue((CrInt64u)num);
    prop.SetValueType(SDK::CrDataType::CrDataType_UInt16Array);
    SDK::SetDeviceProperty(m_device_handle, &prop);
}

void CameraDevice::set_focus_bracket_focus_range()
{
    if (false == get_focus_bracket_focus_range())
        return;

    if (1 != m_prop.focus_bracket_focus_range.writable) {
        tout << "Focus Bracket is not writable\n";
        return;
    }

    text input;
    tout << "\nWould you like to set a new Focus Bracket Range value? (y/n): ";
    std::getline(tin, input);
    if (input != TEXT("y")) {
        tout << "Skip.\n";
        return;
    }

    tout << std::endl << "input Number> ";
    std::getline(tin, input);
    text_stringstream ss(input);
    CrInt32u num = 0;
    ss >> num;

    // range
    if (num < m_prop.focus_bracket_focus_range.possible.at(0) || m_prop.focus_bracket_focus_range.possible.at(1) < num)
    {
        tout << "Input cancelled.\n";
        return;
    }

    SDK::CrDeviceProperty prop;
    prop.SetCode(SDK::CrDevicePropertyCode::CrDeviceProperty_FocusBracketFocusRange);
    prop.SetCurrentValue((CrInt64u)num);
    prop.SetValueType(SDK::CrDataType::CrDataType_UInt8Array);
    SDK::SetDeviceProperty(m_device_handle, &prop);
}

void CameraDevice::set_image_stabilization_steady_shot()
{
    if (false == get_image_stabilization_steady_shot())
        return;

    if (1 != m_prop.image_stabilization_steady_shot.writable) {
        // Not a settable property
        tout << "Image Stabilization Steady Shot(Still) is not writable\n";
        return;
    }

    text input;
    tout << "Would you like to set a new Image Stabilization Steady Shot(Still) value? (y/n): ";
    std::getline(tin, input);
    if (input != TEXT("y")) {
        tout << "Skip setting a new value.\n";
        return;
    }

    tout << "Choose a number set a new Image Stabilization Steady Shot(Still) value:\n";
    tout << "[-1] Cancel input\n";

    auto& values = m_prop.image_stabilization_steady_shot.possible;
    for (std::size_t i = 0; i < values.size(); ++i) {
        tout << '[' << i << "] " << format_image_stabilization_steady_shot(values[i]) << '\n';
    }

    tout << "[-1] Cancel input\n";
    tout << "Choose a number set a new Image Stabilization Steady Shot(Still) value:\n";

    tout << "input> ";
    std::getline(tin, input);
    text_stringstream ss(input);
    int selected_index = 0;
    ss >> selected_index;

    if (selected_index < 0 || values.size() <= selected_index) {
        tout << "Input cancelled.\n";
        return;
    }

    SDK::CrDeviceProperty prop;
    prop.SetCode(SDK::CrDevicePropertyCode::CrDeviceProperty_ImageStabilizationSteadyShot);
    prop.SetCurrentValue(values[selected_index]);
    prop.SetValueType(SDK::CrDataType::CrDataType_UInt8Array);
    SDK::SetDeviceProperty(m_device_handle, &prop);
}

void CameraDevice::set_movie_image_stabilization_steady_shot()
{
    if (false == get_movie_image_stabilization_steady_shot())
        return;

    if (1 != m_prop.movie_image_stabilization_steady_shot.writable) {
        // Not a settable property
        tout << "Image Stabilization Steady Shot(Movie) is not writable\n";
        return;
    }

    text input;
    tout << "Would you like to set a new Image Stabilization Steady Shot(Movie) value? (y/n): ";
    std::getline(tin, input);
    if (input != TEXT("y")) {
        tout << "Skip setting a new value.\n";
        return;
    }

    tout << "Choose a number set a new Image Stabilization Steady Shot(Movie) value:\n\n";
    tout << "[-1] Cancel input\n";

    auto& values = m_prop.movie_image_stabilization_steady_shot.possible;
    for (std::size_t i = 0; i < values.size(); ++i) {
        tout << '[' << i << "] " << format_movie_image_stabilization_steady_shot(values[i]) << '\n';
    }

    tout << "[-1] Cancel input\n";

    tout << "input> ";
    std::getline(tin, input);
    text_stringstream ss(input);
    int selected_index = 0;
    ss >> selected_index;

    if (selected_index < 0 || values.size() <= selected_index) {
        tout << "Input cancelled.\n";
        return;
    }

    SDK::CrDeviceProperty prop;
    prop.SetCode(SDK::CrDevicePropertyCode::CrDeviceProperty_Movie_ImageStabilizationSteadyShot);
    prop.SetCurrentValue(values[selected_index]);
    prop.SetValueType(SDK::CrDataType::CrDataType_UInt8Array);
    SDK::SetDeviceProperty(m_device_handle, &prop);
}

void CameraDevice::set_silent_mode() {

    if (false == get_silent_mode())
        return;
    if (1 != m_prop.silent_mode.writable) {
        // Not a settable property
        tout << "Silent Mode is not writable\n";
        return;
    }

    tout << "\n";
    text input;
    tout << "Would you like to set a new Silent Mode value? (y/n): ";
    std::getline(tin, input);
    if (input != TEXT("y")) {
        tout << "Skip setting a new value.\n";
        return;
    }

    tout << "Choose a number set a new Silent Mode value:\n";
    tout << "[-1] Cancel input\n";

    auto& values = m_prop.silent_mode.possible;
    for (std::size_t i = 0; i < values.size(); ++i) {
        tout << '[' << i << "] " << format_silent_mode(values[i]) << '\n';
    }

    tout << "[-1] Cancel input\n";
    tout << "Choose a number set a new Silent Mode value:\n";

    tout << "input> ";
    std::getline(tin, input);
    text_stringstream ss(input);
    int selected_index = 0;
    ss >> selected_index;

    if (selected_index < 0 || values.size() <= selected_index) {
        tout << "Input cancelled.\n";
        return;
    }

    SDK::CrDeviceProperty prop;
    prop.SetCode(SDK::CrDevicePropertyCode::CrDeviceProperty_SilentMode);
    prop.SetCurrentValue(values[selected_index]);
    prop.SetValueType(SDK::CrDataType::CrDataType_UInt8Array);
    SDK::SetDeviceProperty(m_device_handle, &prop);
}

void CameraDevice::set_silent_mode_aperture_drive_in_af()
{
    if (false == get_silent_mode_aperture_drive_in_af())
        return;

    tout << "\n";
    if (1 != m_prop.silent_mode_aperture_drive_in_af.writable) {
        // Not a settable property
        tout << "Silent Mode Aperture Drive in AF is not writable\n";
        return;
    }

    text input;
    tout << "Would you like to set an Aperture Drive in AF value? (y/n): ";
    std::getline(tin, input);
    if (input != TEXT("y")) {
        tout << "Skip setting a new value.\n";
        return;
    }

    tout << "Choose a number set a new Aperture Drive in AF value:\n";
    tout << "[-1] Cancel input\n";

    auto& values = m_prop.silent_mode_aperture_drive_in_af.possible;
    for (std::size_t i = 0; i < values.size(); ++i) {
        tout << '[' << i << "] " << format_silent_mode_aperture_drive_in_af(values[i]) << '\n';
    }

    tout << "[-1] Cancel input\n";
    tout << "Choose a number set a new Aperture Drive in AF value:\n";

    tout << "input> ";
    std::getline(tin, input);
    text_stringstream ss(input);
    int selected_index = 0;
    ss >> selected_index;

    if (selected_index < 0 || values.size() <= selected_index) {
        tout << "Input cancelled.\n";
        return;
    }

    SDK::CrDeviceProperty prop;
    prop.SetCode(SDK::CrDevicePropertyCode::CrDeviceProperty_SilentModeApertureDriveInAF);
    prop.SetCurrentValue(values[selected_index]);
    prop.SetValueType(SDK::CrDataType::CrDataType_UInt8Array);
    SDK::SetDeviceProperty(m_device_handle, &prop);
}

void CameraDevice::set_silent_mode_shutter_when_power_off()
{
    if (false == get_silent_mode_shutter_when_power_off())
        return;

    tout << "\n";
    if (1 != m_prop.silent_mode_shutter_when_power_off.writable) {
        // Not a settable property
        tout << "Silent Mode Shutter When Power OFF is not writable\n";
        return;
    }

    text input;
    tout << "Would you like to set a new Shutter When Power OFF value? (y/n): ";
    std::getline(tin, input);
    if (input != TEXT("y")) {
        tout << "Skip setting a new value.\n";
        return;
    }

    tout << "Choose a number set a new Shutter When Power OFF value:\n";
    tout << "[-1] Cancel input\n";

    auto& values = m_prop.silent_mode_shutter_when_power_off.possible;
    for (std::size_t i = 0; i < values.size(); ++i) {
        tout << '[' << i << "] " << format_silent_mode_shutter_when_power_off(values[i]) << '\n';
    }

    tout << "[-1] Cancel input\n";
    tout << "Choose a number set a new Shutter When Power OFF value:\n";

    tout << "input> ";
    std::getline(tin, input);
    text_stringstream ss(input);
    int selected_index = 0;
    ss >> selected_index;

    if (selected_index < 0 || values.size() <= selected_index) {
        tout << "Input cancelled.\n";
        return;
    }

    SDK::CrDeviceProperty prop;
    prop.SetCode(SDK::CrDevicePropertyCode::CrDeviceProperty_SilentModeShutterWhenPowerOff);
    prop.SetCurrentValue(values[selected_index]);
    prop.SetValueType(SDK::CrDataType::CrDataType_UInt8Array);
    SDK::SetDeviceProperty(m_device_handle, &prop);
}

void CameraDevice::set_silent_mode_auto_pixel_mapping()
{
    if (false == get_silent_mode_auto_pixel_mapping())
        return;

    tout << "\n";
    if (1 != m_prop.silent_mode_auto_pixel_mapping.writable) {
        // Not a settable property
        tout << "Silent Mode Auto Pixel Mapping is not writable\n";
        return;
    }

    text input;
    tout << "Would you like to set a new Auto Pixel Mapping value? (y/n): ";
    std::getline(tin, input);
    if (input != TEXT("y")) {
        tout << "Skip setting a new value.\n";
        return;
    }

    tout << "Choose a number set a new Auto Pixel Mapping value:\n";
    tout << "[-1] Cancel input\n";

    auto& values = m_prop.silent_mode_auto_pixel_mapping.possible;
    for (std::size_t i = 0; i < values.size(); ++i) {
        tout << '[' << i << "] " << format_silent_mode_auto_pixel_mapping(values[i]) << '\n';
    }

    tout << "[-1] Cancel input\n";
    tout << "Choose a number set a new Auto Pixel Mapping value:\n";

    tout << "input> ";
    std::getline(tin, input);
    text_stringstream ss(input);
    int selected_index = 0;
    ss >> selected_index;

    if (selected_index < 0 || values.size() <= selected_index) {
        tout << "Input cancelled.\n";
        return;
    }

    SDK::CrDeviceProperty prop;
    prop.SetCode(SDK::CrDevicePropertyCode::CrDeviceProperty_SilentModeAutoPixelMapping);
    prop.SetCurrentValue(values[selected_index]);
    prop.SetValueType(SDK::CrDataType::CrDataType_UInt8Array);
    SDK::SetDeviceProperty(m_device_handle, &prop);
}

void CameraDevice::set_shutter_type()
{
    if (false == get_shutter_type())
        return;

    if (1 != m_prop.shutter_type.writable) {
        // Not a settable property
        tout << "Shutter Type is not writable\n";
        return;
    }

    text input;
    tout << "Would you like to set a new Shutter Type value? (y/n): ";
    std::getline(tin, input);
    if (input != TEXT("y")) {
        tout << "Skip setting a new value.\n";
        return;
    }

    tout << "Choose a number set a new Shutter Type value:\n";
    tout << "[-1] Cancel input\n";

    auto& values = m_prop.shutter_type.possible;
    for (std::size_t i = 0; i < values.size(); ++i) {
        tout << '[' << i << "] " << format_shutter_type(values[i]) << '\n';
    }

    tout << "[-1] Cancel input\n";
    tout << "Choose a number set a new Shutter Type value:\n";

    tout << "input> ";
    std::getline(tin, input);
    text_stringstream ss(input);
    int selected_index = 0;
    ss >> selected_index;

    if (selected_index < 0 || values.size() <= selected_index) {
        tout << "Input cancelled.\n";
        return;
    }

    SDK::CrDeviceProperty prop;
    prop.SetCode(SDK::CrDevicePropertyCode::CrDeviceProperty_ShutterType);
    prop.SetCurrentValue(values[selected_index]);
    prop.SetValueType(SDK::CrDataType::CrDataType_UInt8Array);
    SDK::SetDeviceProperty(m_device_handle, &prop);
}

void CameraDevice::set_movie_shooting_mode()
{
    if (false == get_movie_shooting_mode())
        return;

    if (1 != m_prop.movie_shooting_mode.writable) {
        // Not a settable property
        tout << "Movie Shooting Mode is not writable\n";
        return;
    }

    text input;
    tout << "Would you like to set a new Movie Shooting Mode? (y/n): ";
    std::getline(tin, input);
    if (input != TEXT("y")) {
        tout << "Skip setting a new value.\n";
        return;
    }

    tout << "Choose a number set a new Movie Shooting Mode:\n";
    tout << "[-1] Cancel input\n";

    auto& values = m_prop.movie_shooting_mode.possible;
    for (std::size_t i = 0; i < values.size(); ++i) {
        tout << '[' << i << "] " << format_movie_shooting_mode(values[i]) << '\n';
    }

    tout << "[-1] Cancel input\n";
    tout << "Choose a number set a new Movie Shooting Mode:\n";

    tout << "input> ";
    std::getline(tin, input);
    text_stringstream ss(input);
    int selected_index = 0;
    ss >> selected_index;

    if (selected_index < 0 || values.size() <= selected_index) {
        tout << "Input cancelled.\n";
        return;
    }

    SDK::CrDeviceProperty prop;
    prop.SetCode(SDK::CrDevicePropertyCode::CrDeviceProperty_MovieShootingMode);
    prop.SetCurrentValue(values[selected_index]);
    prop.SetValueType(SDK::CrDataType::CrDataType_UInt16Array);
    SDK::SetDeviceProperty(m_device_handle, &prop);
}

void CameraDevice::execute_downup_property(CrInt16u code)
{
    SDK::CrDeviceProperty prop;
    prop.SetCode(code);
    prop.SetValueType(SDK::CrDataType::CrDataType_UInt16Array);

    // Down
    prop.SetCurrentValue(SDK::CrPropertyCustomWBCaptureButton::CrPropertyCustomWBCapture_Down);
    SDK::SetDeviceProperty(m_device_handle, &prop);

    std::this_thread::sleep_for(500ms);

    // Up
    prop.SetCurrentValue(SDK::CrPropertyCustomWBCaptureButton::CrPropertyCustomWBCapture_Up);
    SDK::SetDeviceProperty(m_device_handle, &prop);

    std::this_thread::sleep_for(500ms);
}

void CameraDevice::execute_pos_xy(CrInt16u code)
{
    load_properties();

    text input;
    tout << std::endl << "Change position ? (y/n): ";
    std::getline(tin, input);
    if (input != TEXT("y")) {
        tout << "Skip.\n";
        return;
    }

    tout << std::endl << "Set the value of X (decimal)" << std::endl;
    tout << "Regarding details of usage, please check API doc." << std::endl;

    tout << std::endl << "input X> ";
    std::getline(tin, input);
    text_stringstream ss1(input);
    CrInt32u x = 0;
    ss1 >> x;

    if (x < 0 || x > 639) {
        tout << "Input cancelled.\n";
        return;
    }

    tout << "input X = " << x << '\n';

    std::this_thread::sleep_for(1000ms);

    tout << std::endl << "Set the value of Y (decimal)" << std::endl;

    tout << std::endl << "input Y> ";
    std::getline(tin, input);
    text_stringstream ss2(input);
    CrInt32u y = 0;
    ss2 >> y;

    if (y < 0 || y > 479 ) {
        tout << "Input cancelled.\n";
        return;
    }

    tout << "input Y = "<< y << '\n';

    std::this_thread::sleep_for(1000ms);

    int x_y = x << 16 | y;

    tout << std::endl << "input X_Y = 0x" << std::hex << x_y << std::dec << '\n';

    SDK::CrDeviceProperty prop;
    prop.SetCode(code);
    prop.SetCurrentValue((CrInt64u)x_y);
    prop.SetValueType(SDK::CrDataType::CrDataType_UInt32);

    SDK::SetDeviceProperty(m_device_handle, &prop);
}

void CameraDevice::execute_preset_focus()
{
    load_properties();

    auto& values_save = m_prop.save_zoom_and_focus_position.possible;
    auto& values_load = m_prop.load_zoom_and_focus_position.possible;

    if ((1 != m_prop.save_zoom_and_focus_position.writable) &&
        (1 != m_prop.load_zoom_and_focus_position.writable)){
        // Not a settable property
        tout << "Preset Focus and Zoom is not supported.\n";
        return;
    }

    tout << std::endl << "Save Zoom and Focus Position Enable Preset number: " << std::endl;
    for (int i = 0; i < values_save.size(); i++)
    {
        tout << " " << (int)values_save.at(i) << std::endl;
    }

    tout << std::endl << "Load Zoom and Focus Position Enable Preset number: " << std::endl;
    for (int i = 0; i < values_load.size(); i++)
    {
        tout << " " << (int)values_load.at(i) << std::endl;
    }

    tout << std::endl << "Set the value of operation:\n";
    tout << "[-1] Cancel input\n";

    tout << "[1] Save Zoom and Focus Position\n";
    tout << "[2] Load Zoom and Focus Position\n";

    tout << "[-1] Cancel input\n";
    tout << "Choose a number:\n";

    text input;
    tout << std::endl << "input> ";
    std::getline(tin, input);
    text_stringstream ss(input);
    int selected_index = 0;
    ss >> selected_index;

    CrInt32u code = 0;
    if ((1 == selected_index) && (1 == m_prop.save_zoom_and_focus_position.writable)) {
        code = SDK::CrDevicePropertyCode::CrDeviceProperty_ZoomAndFocusPosition_Save;
    }
    else if ((2 == selected_index) && (1 == m_prop.load_zoom_and_focus_position.writable)) {
        code = SDK::CrDevicePropertyCode::CrDeviceProperty_ZoomAndFocusPosition_Load;
    }
    else {
        tout << "The Selected operation is not supported.\n";
        return;
    }

    tout << "Set the value of Preset number:\n";

    tout << std::endl << "input> ";
    std::getline(tin, input);
    text_stringstream ss_slot(input);
    int input_value = 0;
    ss_slot >> input_value;

    if (code == SDK::CrDevicePropertyCode::CrDeviceProperty_ZoomAndFocusPosition_Save) {
        if (find(values_save.begin(), values_save.end(), input_value) == values_save.end()) {
            tout << "Input cancelled.\n";
            return;
        }
    }
    else {
        if (find(values_load.begin(), values_load.end(), input_value) == values_load.end()) {
            tout << "Input cancelled.\n";
            return;
        }
    }

    SDK::CrDeviceProperty prop;
    prop.SetCode(code);
    prop.SetCurrentValue(input_value);
    prop.SetValueType(SDK::CrDataType::CrDataType_UInt8);
    SDK::SetDeviceProperty(m_device_handle, &prop);
}

void CameraDevice::execute_APS_C_or_Full()
{
    if (false == get_aps_c_or_full_switching_setting())
        return;

    text input;
    tout << std::endl << "Execute APS-C or FULL switching control of the camera ? (y/n): ";
    std::getline(tin, input);
    if (input != TEXT("y")) {
        tout << "Skip.\n";
        return;
    }
    // Get the latest status
    load_properties();
    if (SDK::CrAPS_C_or_Full_SwitchingEnableStatus::CrAPS_C_or_Full_Switching_Enable != m_prop.aps_c_of_full_switching_enable_status.current) {
        tout << std::endl << "APS-C/Full switching is not executable.\n";
        return;
    }
    SDK::SendCommand(m_device_handle, SDK::CrCommandId::CrCommandId_APS_C_or_Full_Switching, SDK::CrCommandParam_Down);
    std::this_thread::sleep_for(100ms);
    SDK::SendCommand(m_device_handle, SDK::CrCommandId::CrCommandId_APS_C_or_Full_Switching, SDK::CrCommandParam_Up);
    tout << std::endl << "Executed the switch between APS-C or Full.\n";
}

void CameraDevice::execute_movie_rec_toggle()
{
    if (false == get_movie_rec_button_toggle_enable_status()) {
        return;
    }

    text input;
    tout << std::endl << "Operate the movie recording button toggle? (y/n): ";
    std::getline(tin, input);
    if (input != TEXT("y")) {
        tout << "Skip .\n";
        return;
    }

    tout << "Choose a number:\n";
    tout << "[-1] Cancel input\n";

    tout << "[1] Up" << '\n';
    tout << "[2] Down" << '\n';

    tout << "[-1] Cancel input\n";
    tout << "Choose a number:\n";

    tout << std::endl << "input> ";
    std::getline(tin, input);
    text_stringstream ss(input);
    int selected_index = 0;
    ss >> selected_index;

    if (selected_index < 0) {
        tout << "Input cancelled.\n";
        return;
    }

    CrInt64u ptpValue = 0;
    switch (selected_index) {
    case 1:
        ptpValue = SDK::CrCommandParam::CrCommandParam_Up;
        break;
    case 2:
        ptpValue = SDK::CrCommandParam::CrCommandParam_Down;
        break;
    default:
        selected_index = -1;
        break;
    }

    if (-1 == selected_index) {
        tout << "Input cancelled.\n";
        return;
    }
    // Get the latest status
    load_properties();
    if (SDK::CrMovieRecButtonToggleEnableStatus::CrMovieRecButtonToggle_Enable != m_prop.movie_rec_button_toggle_enable_status.current) {
        tout << "Unable to execute the movie recording button toggle.\n";
        return;
    }
    SDK::SendCommand(m_device_handle, SDK::CrCommandId::CrCommandId_MovieRecButtonToggle, (SDK::CrCommandParam)ptpValue);

}

void CameraDevice::execute_focus_bracket()
{
    load_properties();

    if (1 != m_prop.focus_bracket_shot_num.writable || 1 != m_prop.focus_bracket_focus_range.writable) {
        tout << "Focus Bracket Shooting is not executable\n";
        return;
    }

    tout << "Execute Focus Bracket shooting \n";

    // Set, PriorityKeySettings property
    SDK::CrDeviceProperty priority;
    priority.SetCode(SDK::CrDevicePropertyCode::CrDeviceProperty_PriorityKeySettings);
    priority.SetCurrentValue(SDK::CrPriorityKeySettings::CrPriorityKey_PCRemote);
    priority.SetValueType(SDK::CrDataType::CrDataType_UInt32Array);
    auto err_priority = SDK::SetDeviceProperty(m_device_handle, &priority);
    if (CR_FAILED(err_priority)) {
        tout << "Priority Key setting FAILED\n";
        return;
    }
    else {
        tout << "Priority Key setting SUCCESS\n";
    }

    // Set, FocusBracket
    bool modeset_flg = set_drive_mode(SDK::CrDriveMode::CrDrive_FocusBracket);
    if (!modeset_flg) {
        tout << "Still Capture Mode setting FAILED\n";
        return;
    }

    bool continueFlag = false;
    for (int i = 0; i < 10; i++)
    {
        tout << ". ";
        std::this_thread::sleep_for(500ms);
        load_properties();
        if (SDK::CrDriveMode::CrDrive_FocusBracket == m_prop.still_capture_mode.current) {
            tout << "\nStill Capture Mode setting SUCCESS\n";
            continueFlag = true;
            break;
        }
    }
    if (false == continueFlag) {
        tout << "\nStill Capture Mode setting FAILED\n";
        return;
    }

    capture_image();
}

void CameraDevice::do_download_camera_setting_file()
{
    if (false == get_camera_setting_saveread_state())
        return;

    if (m_prop.camera_setting_save_operation.current == SDK::CrCameraSettingSaveOperation::CrCameraSettingSaveOperation_Enable) {
        tout << "Camera-Setting Save Operation: Enable\n";
    } else {
        tout << "Camera-Setting Save Operation: Disable\n";
    }

    // File Name
    tout << "\nPlease Enter the name of the file you want to save. \n";
    tout << "(ex. CUMSET.DAT)\n";
    tout << "If you do not specify a file name, the file will be saved with the default save name.\n" << std::endl;
    tout << "[-1] Cancel input\n" << std::endl;
    tout << "file name > ";
    text name;
    std::getline(tin, name);
    text_stringstream ss(name);
    int selected_index = 0;
    ss >> selected_index;
    if (-1 == selected_index) {
        tout << "Input cancelled.\n";
        return;
    }

    // Get the latest status
    load_properties();
    if (SDK::CrCameraSettingSaveOperation::CrCameraSettingSaveOperation_Enable != m_prop.camera_setting_save_operation.current) {
        tout << "Unable to download Camera-Setting file. \n";
        return;
    }
    // File Path
#if defined(__APPLE__)
    char path[MAC_MAX_PATH]; /*MAX_PATH*/
    memset(path, 0, sizeof(path));
    if(NULL == getcwd(path, sizeof(path) - 1)){
        tout << "Folder path is too long.\n";
        return;
    }
    char* delimit = "/";
    if(strlen(path) + strlen(delimit) > MAC_MAX_PATH){
        tout << "Folder path is too long.\n";
        return;
    }
    strncat(path, delimit, strlen(delimit));
    auto err = SDK::DownloadSettingFile(m_device_handle, SDK::CrDownloadSettingFileType::CrDownloadSettingFileType_Setup,(CrChar*)path, (CrChar*)name.c_str());

if (CR_FAILED(err)) {
    tout << "Download Camera-Setting file FAILED\n";
}
#else
    auto path = fs::current_path();
    auto err = SDK::DownloadSettingFile(m_device_handle, SDK::CrDownloadSettingFileType::CrDownloadSettingFileType_Setup,(CrChar*)path.c_str(), (CrChar*)name.c_str());

    if (CR_FAILED(err)) {
        tout << "Download Camera-Setting file FAILED\n";
    }
#endif
}

void CameraDevice::do_upload_camera_setting_file()
{
    if (false == get_camera_setting_saveread_state())
        return;

    if (m_prop.camera_setting_read_operation.current == SDK::CrCameraSettingReadOperation::CrCameraSettingReadOperation_Enable) {
        tout << "Camera-Setting Read Operation: Enable\n";
    } else {
        tout << "Camera-Setting Read Operation: Disable\n";
    }

    tout << "Have the camera load the configuration file on the PC.\n";

    //Search for *.DAT in current_path
    std::vector<text> file_names;
    getFileNames(file_names);

    if (file_names.size() == 0) {
        tout << "DAT file not found.\n\n";
        return;
    }
    
    tout << std::endl << "Choose a number:\n";
    tout << "[-1] Cancel input" << std::endl;
    for (size_t i = 0; i < file_names.size(); i++)
    {
        tout << "[" << i << "]" << file_names[i] << '\n';
    }
    tout << "[-1] Cancel input\n" << std::endl;

    tout << "input>";
    text input;
    std::getline(tin, input);

    text_stringstream ss(input);
    int selected_index = 0;
    ss >> selected_index;
    if ((selected_index == 0 && input != TEXT("0")) || selected_index < 0 || selected_index >= file_names.size()) {
        tout << "Input cancelled.\n";
        return;
    }

#if defined(__APPLE__)
    char filename[MAC_MAX_PATH]; /*MAX_PATH*/
    memset(filename, 0, sizeof(filename));
    if(NULL == getcwd(filename, sizeof(filename) - 1)){
        tout << "Folder path is too long.\n";
        return;
    }
    char* delimit = "/";
    if(strlen(filename) + strlen(delimit) + file_names[selected_index].length() > MAC_MAX_PATH){
        tout << "Please shorten the file path. \n";
        return;
    }
    strncat(filename, delimit, strlen(delimit));
    strncat(filename, file_names[selected_index].c_str(), file_names[selected_index].length());
#else 
    auto filepath = fs::current_path();
    filepath.append(file_names[selected_index]);
    CrChar* filename = (CrChar*)filepath.c_str();
#endif

    tout << filename << "\n";
    // Get the latest status
    load_properties();
    if (SDK::CrCameraSettingReadOperation::CrCameraSettingReadOperation_Enable != m_prop.camera_setting_read_operation.current) {
        tout << "Unable to upload Camera-Setting file. \n";
        return;
    }

    auto err = SDK::UploadSettingFile(m_device_handle, SDK::CrUploadSettingFileType::CrUploadSettingFileType_Setup, filename);

    if (CR_FAILED(err)) {
        tout << "Upload Camera-Setting file FAILED\n";
    }
}

void CameraDevice::getFileNames(std::vector<text> &file_names)
{
#if defined(__APPLE__)
    char search_name[MAC_MAX_PATH]; /*MAX_PATH*/
    memset(search_name, 0, sizeof(search_name));
    if(NULL == getcwd(search_name, sizeof(search_name) - 1)){
        tout << "Folder path is too long.\n";
        return;
    }
#else
    auto search_name = fs::current_path();
#if defined (WIN32) || defined(WIN64)
    search_name.append(TEXT("*.DAT"));
#endif
#endif

#if defined(__APPLE__) || defined(__linux__)
    DIR* dp;
    int i = 0;
    struct dirent* ep;
#if defined(__APPLE__)
    dp = opendir(search_name);
#else
    dp = opendir(search_name.c_str());
#endif
    if (dp != NULL)
    {
        while ((ep = readdir(dp)) != NULL) {
            if (ep->d_name[0] == '.') {
            }
            else {
                text    str(ep->d_name);
                if(((ep->d_name[str.length()-4]=='.')
                	&&(ep->d_name[str.length()-3]=='d')
                	&&(ep->d_name[str.length()-2]=='a')
                	&&(ep->d_name[str.length()-1]=='t'))
                	||((ep->d_name[str.length()-4]=='.')
                	&&(ep->d_name[str.length()-3]=='D')
                	&&(ep->d_name[str.length()-2]=='A')
                	&&(ep->d_name[str.length()-1]=='T'))){
                file_names.push_back(str);
                i++;
                }
            }
        }
        (void)closedir(dp);
    }
    // End Mac & Linux implementation
#else

    WIN32_FIND_DATA win32fd;
    HANDLE hff = FindFirstFile(search_name.c_str(), &win32fd);

    if (hff != INVALID_HANDLE_VALUE) {
        do {
            if (win32fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            }
            else {
                text str(win32fd.cFileName);
                file_names.push_back(str);
            }
        } while (FindNextFile(hff, &win32fd));
        FindClose(hff);
    }
    else {
        return ;
    }
    return ;
#endif
}

void CameraDevice::change_live_view_enable()
{
    m_lvEnbSet = !m_lvEnbSet;
    SDK::SetDeviceSetting(m_device_handle, SDK::Setting_Key_EnableLiveView, (CrInt32u)m_lvEnbSet);
}

bool CameraDevice::is_connected() const
{
    return m_connected.load();
}

std::uint32_t CameraDevice::ip_address() const
{
    if (m_conn_type == ConnectionType::NETWORK)
        return m_net_info.ip_address;
    return 0;
}

text CameraDevice::ip_address_fmt() const
{
    if (m_conn_type == ConnectionType::NETWORK)
    {
        return m_net_info.ip_address_fmt;
    }
    return text(TEXT("N/A"));
}

text CameraDevice::mac_address() const
{
    if (m_conn_type == ConnectionType::NETWORK)
        return m_net_info.mac_address;
    return text(TEXT("N/A"));
}

std::int16_t CameraDevice::pid() const
{
    if (m_conn_type == ConnectionType::USB)
        return m_usb_info.pid;
    return 0;
}

text CameraDevice::get_id()
{
    if (ConnectionType::NETWORK == m_conn_type) {
        return m_net_info.mac_address;
    }
    else
        return text((TCHAR*)m_info->GetId());
}

void CameraDevice::OnConnected(SDK::DeviceConnectionVersioin version)
{
    m_connected.store(true);
    text id(this->get_id());
    tout << "Connected to " << m_info->GetModel() << " (" << id.data() << ")\n";
}

void CameraDevice::OnDisconnected(CrInt32u error)
{
    m_connected.store(false);
    text id(this->get_id());
    tout << "Disconnected from " << m_info->GetModel() << " (" << id.data() << ")\n";
    if ((false == m_spontaneous_disconnection) && (SDK::CrSdkControlMode_ContentsTransfer == m_modeSDK))
    {
        tout << "Please input '0' to return to the TOP-MENU\n";
    }
}

void CameraDevice::OnPropertyChanged()
{
    // tout << "Property changed.\n";
}

void CameraDevice::OnLvPropertyChanged()
{
    // tout << "LvProperty changed.\n";
}

void CameraDevice::OnCompleteDownload(CrChar* filename, CrInt32u type )
{
    text file(filename);
    switch (type)
    {
    case SCRSDK::CrDownloadSettingFileType_None:
       tout << "Complete download. File: " << file.data() << '\n';
        break;
    case SCRSDK::CrDownloadSettingFileType_Setup:
        tout << "Complete download. Camera Setting File: " << file.data() << '\n';
        break;
    default:
        break;
    }
}

void CameraDevice::OnNotifyContentsTransfer(CrInt32u notify, SDK::CrContentHandle contentHandle, CrChar* filename)
{
    // Start
    if (SDK::CrNotify_ContentsTransfer_Start == notify)
    {
        tout << "[START] Contents Handle: 0x " << std::hex << contentHandle << std::dec << std::endl;
    }
    // Complete
    else if (SDK::CrNotify_ContentsTransfer_Complete == notify)
    {
        text file(filename);
        tout << "[COMPLETE] Contents Handle: 0x" << std::hex << contentHandle << std::dec << ", File: " << file.data() << std::endl;
    }
    // Other
    else
    {
        text msg = get_message_desc(notify);
        if (msg.empty()) {
            tout << "[-] Content transfer failure. 0x" << std::hex << notify << ", handle: 0x" << contentHandle << std::dec << std::endl;
        } else {
            tout << "[-] Content transfer failure. handle: 0x" << std::hex << contentHandle  << std::dec << std::endl << "    -> ";
            tout << msg.data() << std::endl;
        }
    }
}

void CameraDevice::OnWarning(CrInt32u warning)
{
    text id(this->get_id());
    if (SDK::CrWarning_Connect_Reconnecting == warning) {
        tout << "Device Disconnected. Reconnecting... " << m_info->GetModel() << " (" << id.data() << ")\n";
        return;
    }
    switch (warning)
    {
    case SDK::CrWarning_ContentsTransferMode_Invalid:
    case SDK::CrWarning_ContentsTransferMode_DeviceBusy:
    case SDK::CrWarning_ContentsTransferMode_StatusError:
        tout << "\nThe camera is in a condition where it cannot transfer content.\n\n";
        tout << "Please input '0' to return to the TOP-MENU and connect again.\n";
        break;
    case SDK::CrWarning_ContentsTransferMode_CanceledFromCamera:
        tout << "\nContent transfer mode canceled.\n";
        tout << "If you want to continue content transfer, input '0' to return to the TOP-MENU and connect again.\n\n";
        break;
    case SDK::CrWarning_CameraSettings_Read_Result_OK:
        tout << "\nConfiguration file read successfully.\n\n";
        break;
    case SDK::CrWarning_CameraSettings_Read_Result_NG:
        tout << "\nFailed to load configuration file\n\n";
        break;
    case SDK::CrWarning_CameraSettings_Save_Result_NG:
        tout << "\nConfiguration file save request failed.\n\n";
        break;
    case SDK::CrWarning_RequestDisplayStringList_Success:
        tout << "\nRequest for DisplayStringList  successfully\n\n";        
        break;
    case SDK::CrWarning_RequestDisplayStringList_Error: 
        tout << "\nFailed to Request for DisplayStringList\n\n";
        break;
    case SDK::CrWarning_CustomWBCapture_Result_OK:
        tout << "\nCustom WB capture successful.\n\n";
        break;
    case SDK::CrWarning_CustomWBCapture_Result_Invalid:
    case SDK::CrWarning_CustomWBCapture_Result_NG:
        tout << "\nCustom WB capture failure.\n\n";
        break;
    default:
        return;
    }
}

void CameraDevice::OnPropertyChangedCodes(CrInt32u num, CrInt32u* codes)
{
    //tout << "Property changed.  num = " << std::dec << num;
    //tout << std::hex;
    //for (std::int32_t i = 0; i < num; ++i)
    //{
    //    tout << ", 0x" << codes[i];
    //}
    //tout << std::endl << std::dec;
}

void CameraDevice::OnLvPropertyChangedCodes(CrInt32u num, CrInt32u* codes)
{
    //tout << "LvProperty changed.  num = " << std::dec << num;
    //tout << std::hex;
    //for (std::int32_t i = 0; i < num; ++i)
    //{
    //    tout << ", 0x" << codes[i];
    //}
    //tout << std::endl;
#if 0 
    SDK::CrLiveViewProperty* lvProperty = nullptr;
    int32_t nprop = 0;
    SDK::CrError err = SDK::GetSelectLiveViewProperties(m_device_handle, num, codes, &lvProperty, &nprop);
    if (CR_SUCCEEDED(err) && lvProperty) {
        for (int32_t i=0 ; i<nprop ; i++) {
            auto prop = lvProperty[i];
            if (SDK::CrFrameInfoType::CrFrameInfoType_FocusFrameInfo == prop.GetFrameInfoType()) {
                int sizVal = prop.GetValueSize();
                int count = sizVal / sizeof(SDK::CrFocusFrameInfo);
                SDK::CrFocusFrameInfo* pFrameInfo = (SDK::CrFocusFrameInfo*)prop.GetValue();
                if (0 == sizVal || nullptr == pFrameInfo) {
                    tout << "  FocusFrameInfo nothing\n";
                }
                else {
                    for (std::int32_t frame = 0; frame < count; ++frame) {
                        auto lvprop = pFrameInfo[frame];
                        char buff[512];
                        memset(buff, 0, sizeof(buff));
#if defined(_WIN32) || (_WIN64)
                        sprintf_s(buff, "  FocusFrameInfo no[%d] type[%d] state[%d] w[%d] h[%d] Deno[%d-%d] Nume[%d-%d]",
                            frame + 1,
                            lvprop.type,
                            lvprop.state,
                            lvprop.width, lvprop.height,
                            lvprop.xDenominator, lvprop.yDenominator,
                            lvprop.xNumerator, lvprop.yNumerator);
#else
                        snprintf(buff, sizeof(buff), "  FocusFrameInfo no[%d] type[%d] state[%d] w[%d] h[%d] Deno[%d-%d] Nume[%d-%d]",
                            frame + 1,
                            lvprop.type,
                            lvprop.state,
                            lvprop.width, lvprop.height,
                            lvprop.xDenominator, lvprop.yDenominator,
                            lvprop.xNumerator, lvprop.yNumerator);
#endif                           
                        tout << buff << std::endl;
                    }
                }
            }
            else if (SDK::CrFrameInfoType::CrFrameInfoType_FaceFrameInfo == prop.GetFrameInfoType()) {
                int sizVal = prop.GetValueSize();
                int count = sizVal / sizeof(SDK::CrFaceFrameInfo);
                SDK::CrFaceFrameInfo* pFrameInfo = (SDK::CrFaceFrameInfo*)prop.GetValue();
                if (0 == sizVal || nullptr == pFrameInfo) {
                    tout << "  FaceFrameInfo nothing\n";
                }
                else {
                    for (std::int32_t frame = 0; frame < count; ++frame) {
                        auto lvprop = pFrameInfo[frame];
                        char buff[512];
                        memset(buff, 0, sizeof(buff));
#if defined(_WIN32) || (_WIN64)
                        sprintf_s(buff, "  FaceFrameInfo no[%d] type[%d] state[%d] w[%d] h[%d] Deno[%d-%d] Nume[%d-%d]",
                            frame + 1,
                            lvprop.type,
                            lvprop.state,
                            lvprop.width, lvprop.height,
                            lvprop.xDenominator, lvprop.yDenominator,
                            lvprop.xNumerator, lvprop.yNumerator);
#else
                        snprintf(buff, sizeof(buff), "  FaceFrameInfo no[%d] type[%d] state[%d] w[%d] h[%d] Deno[%d-%d] Nume[%d-%d]",
                            frame + 1,
                            lvprop.type,
                            lvprop.state,
                            lvprop.width, lvprop.height,
                            lvprop.xDenominator, lvprop.yDenominator,
                            lvprop.xNumerator, lvprop.yNumerator);
#endif                           
                        tout << buff << std::endl;
                    }
                }
            }
            else if (SDK::CrFrameInfoType::CrFrameInfoType_TrackingFrameInfo == prop.GetFrameInfoType()) {
                int sizVal = prop.GetValueSize();
                int count = sizVal / sizeof(SDK::CrTrackingFrameInfo);
                SDK::CrTrackingFrameInfo* pFrameInfo = (SDK::CrTrackingFrameInfo*)prop.GetValue();
                if (0 == sizVal || nullptr == pFrameInfo) {
                    tout << "  TrackingFrameInfo nothing\n";
                }
                else {
                    for (std::int32_t frame = 0; frame < count; ++frame) {
                        auto lvprop = pFrameInfo[frame];
                        char buff[512];
                        memset(buff, 0, sizeof(buff));
#if defined(_WIN32) || (_WIN64)
                        sprintf_s(buff, "  TrackingFrameInfo no[%d] type[%d] state[%d] w[%d] h[%d] Deno[%d-%d] Nume[%d-%d]",
                            frame + 1,
                            lvprop.type,
                            lvprop.state,
                            lvprop.width, lvprop.height,
                            lvprop.xDenominator, lvprop.yDenominator,
                            lvprop.xNumerator, lvprop.yNumerator);
#else
                        snprintf(buff, sizeof(buff), "  TrackingFrameInfo no[%d] type[%d] state[%d] w[%d] h[%d] Deno[%d-%d] Nume[%d-%d]",
                            frame + 1,
                            lvprop.type,
                            lvprop.state,
                            lvprop.width, lvprop.height,
                            lvprop.xDenominator, lvprop.yDenominator,
                            lvprop.xNumerator, lvprop.yNumerator);
#endif                           
                        tout << buff << std::endl;
                    }
                }
            }
            else if (SDK::CrFrameInfoType::CrFrameInfoType_Magnifier_Position == prop.GetFrameInfoType()) {
                int sizVal = prop.GetValueSize();
                int count = sizVal / sizeof(SDK::CrMagPosInfo);
                SDK::CrMagPosInfo* pMagPosInfo = (SDK::CrMagPosInfo*)prop.GetValue();
                if (0 == sizVal || nullptr == pMagPosInfo) {
                    tout << "  MagPosInfo nothing\n";
                }
                else {
                    char buff[512];
                    memset(buff, 0, sizeof(buff));
#if defined(_WIN32) || (_WIN64)
                    sprintf_s(buff, "  MagPosInfo w[%d] h[%d] Deno[%d-%d] Nume[%d-%d]",
                        pMagPosInfo->width, pMagPosInfo->height,
                        pMagPosInfo->xDenominator, pMagPosInfo->yDenominator,
                        pMagPosInfo->xNumerator, pMagPosInfo->yNumerator);
#else
                    snprintf(buff, sizeof(buff), "  MagPosInfo w[%d] h[%d] Deno[%d-%d] Nume[%d-%d]",
                        pMagPosInfo->width, pMagPosInfo->height,
                        pMagPosInfo->xDenominator, pMagPosInfo->yDenominator,
                        pMagPosInfo->xNumerator, pMagPosInfo->yNumerator);
#endif
                    tout << buff << std::endl;
                }
            }
        }
        SDK::ReleaseLiveViewProperties(m_device_handle, lvProperty);
    }
#endif
    tout << std::dec;
}

void CameraDevice::OnError(CrInt32u error)
{
    text id(this->get_id());
    text msg = get_message_desc(error);
    if (!msg.empty()) {
        // output is 2 line
        tout << std::endl << msg.data() << std::endl;
        tout << m_info->GetModel() << " (" << id.data() << ")" << std::endl;
        if (SDK::CrError_Connect_TimeOut == error) {
            // append 1 line
            tout << "Please input '0' after Connect camera" << std::endl;
            return;
        }
        if (SDK::CrError_Connect_Disconnected == error)
        {
            return;
        }
        if (SDK::CrError_Connect_SSH_ServerConnectFailed == error
            || SDK::CrError_Connect_SSH_InvalidParameter == error
            || SDK::CrError_Connect_SSH_ServerAuthenticationFailed == error
            || SDK::CrError_Connect_SSH_UserAuthenticationFailed == error
            || SDK::CrError_Connect_SSH_PortForwardFailed == error
            || SDK::CrError_Connect_SSH_GetFingerprintFailed == error)
        {
            m_fingerprint.clear();
            m_userPassword.clear();
        }
        tout << "Please input '0' to return to the TOP-MENU\n";
    }
}

void CameraDevice::load_properties(CrInt32u num, CrInt32u* codes)
{
    std::int32_t nprop = 0;
    SDK::CrDeviceProperty* prop_list = nullptr;

    m_prop.media_slot1_quick_format_enable_status.writable = -1;
    m_prop.media_slot2_quick_format_enable_status.writable = -1;

    SDK::CrError status = SDK::CrError_Generic;
    if (0 == num){
        // Get all
        status = SDK::GetDeviceProperties(m_device_handle, &prop_list, &nprop);
    }
    else {
        // Get difference
        status = SDK::GetSelectDeviceProperties(m_device_handle, num, codes, &prop_list, &nprop);
    }

    if (CR_FAILED(status)) {
        tout << "Failed to get device properties.\n";
        return;
    }

    if (prop_list && nprop > 0) {
        // Got properties list
        for (std::int32_t i = 0; i < nprop; ++i) {
            auto prop = prop_list[i];
            int nval = 0;

            switch (prop.GetCode()) {
            case SDK::CrDevicePropertyCode::CrDeviceProperty_SdkControlMode:
                m_prop.sdk_mode.writable = prop.IsSetEnableCurrentValue();
                m_prop.sdk_mode.current = static_cast<std::uint32_t>(prop.GetCurrentValue());
                m_modeSDK = (SDK::CrSdkControlMode)m_prop.sdk_mode.current;
                break;
            case SDK::CrDevicePropertyCode::CrDeviceProperty_FNumber:
                nval = prop.GetValueSize() / sizeof(std::uint16_t);
                m_prop.f_number.writable = prop.IsSetEnableCurrentValue();
                m_prop.f_number.current = static_cast<std::uint16_t>(prop.GetCurrentValue());
                if (0 < nval) {
                    auto parsed_values = parse_f_number(prop.GetValues(), nval);
                    m_prop.f_number.possible.swap(parsed_values);
                }
                break;
            case SDK::CrDevicePropertyCode::CrDeviceProperty_IsoSensitivity:
                nval = prop.GetValueSize() / sizeof(std::uint32_t);
                m_prop.iso_sensitivity.writable = prop.IsSetEnableCurrentValue();
                m_prop.iso_sensitivity.current = static_cast<std::uint32_t>(prop.GetCurrentValue());
                if (0 < nval) {
                    auto parsed_values = parse_iso_sensitivity(prop.GetValues(), nval);
                    m_prop.iso_sensitivity.possible.swap(parsed_values);
                }
                break;
            case SDK::CrDevicePropertyCode::CrDeviceProperty_ShutterSpeed:
                nval = prop.GetValueSize() / sizeof(std::uint32_t);
                m_prop.shutter_speed.writable = prop.IsSetEnableCurrentValue();
                m_prop.shutter_speed.current = static_cast<std::uint32_t>(prop.GetCurrentValue());
                if (0 < nval) {
                    auto parsed_values = parse_shutter_speed(prop.GetValues(), nval);
                    m_prop.shutter_speed.possible.swap(parsed_values);
                }
                break;
            case SDK::CrDevicePropertyCode::CrDeviceProperty_PriorityKeySettings:
                nval = prop.GetValueSize() / sizeof(std::uint16_t);
                m_prop.position_key_setting.writable = prop.IsSetEnableCurrentValue();
                m_prop.position_key_setting.current = static_cast<std::uint16_t>(prop.GetCurrentValue());
                if (nval != m_prop.position_key_setting.possible.size()) {
                    auto parsed_values = parse_position_key_setting(prop.GetValues(), nval);
                    m_prop.position_key_setting.possible.swap(parsed_values);
                }
                break;
            case SDK::CrDevicePropertyCode::CrDeviceProperty_ExposureProgramMode:
                nval = prop.GetValueSize() / sizeof(std::uint32_t);
                m_prop.exposure_program_mode.writable = prop.IsSetEnableCurrentValue();
                m_prop.exposure_program_mode.current = static_cast<std::uint32_t>(prop.GetCurrentValue());
                if (0 < nval) {
                    auto parsed_values = parse_exposure_program_mode(prop.GetValues(), nval);
                    m_prop.exposure_program_mode.possible.swap(parsed_values);
                }
                break;
            case SDK::CrDevicePropertyCode::CrDeviceProperty_DriveMode:
                nval = prop.GetValueSize() / sizeof(std::uint32_t);
                m_prop.still_capture_mode.writable = prop.IsSetEnableCurrentValue();
                m_prop.still_capture_mode.current = static_cast<std::uint32_t>(prop.GetCurrentValue());
                if (0 < nval) {
                    auto parsed_values = parse_still_capture_mode(prop.GetValues(), nval);
                    m_prop.still_capture_mode.possible.swap(parsed_values);
                }
                break;
            case SDK::CrDevicePropertyCode::CrDeviceProperty_FocusMode:
                nval = prop.GetValueSize() / sizeof(std::uint16_t);
                m_prop.focus_mode.writable = prop.IsSetEnableCurrentValue();
                m_prop.focus_mode.current = static_cast<std::uint16_t>(prop.GetCurrentValue());
                if (0 < nval) {
                    auto parsed_values = parse_focus_mode(prop.GetValues(), nval);
                    m_prop.focus_mode.possible.swap(parsed_values);
                }
                break;
            case SDK::CrDevicePropertyCode::CrDeviceProperty_FocusArea:
                nval = prop.GetValueSize() / sizeof(std::uint16_t);
                m_prop.focus_area.writable = prop.IsSetEnableCurrentValue();
                m_prop.focus_area.current = static_cast<std::uint16_t>(prop.GetCurrentValue());
                if (0 < nval) {
                    auto parsed_values = parse_focus_area(prop.GetValues(), nval);
                    m_prop.focus_area.possible.swap(parsed_values);
                }
                break;
            case SDK::CrDevicePropertyCode::CrDeviceProperty_LiveView_Image_Quality:
                nval = prop.GetValueSize() / sizeof(std::uint16_t);
                m_prop.live_view_image_quality.writable = prop.IsSetEnableCurrentValue();
                m_prop.live_view_image_quality.current = static_cast<std::uint16_t>(prop.GetCurrentValue());
                if (0 < nval) {
                    auto view = parse_live_view_image_quality(prop.GetValues(), nval);
                    m_prop.live_view_image_quality.possible.swap(view);
                }
                break;
            case SDK::CrDevicePropertyCode::CrDeviceProperty_MediaSLOT1_FormatEnableStatus:
                nval = prop.GetValueSize() / sizeof(std::uint8_t);
                m_prop.media_slot1_full_format_enable_status.writable = prop.IsSetEnableCurrentValue();
                m_prop.media_slot1_full_format_enable_status.current = static_cast<std::uint8_t>(prop.GetCurrentValue());
                if (nval != m_prop.media_slot1_full_format_enable_status.possible.size()) {
                    auto parsed_values = parse_media_slotx_format_enable_status(prop.GetValues(), nval);
                    m_prop.media_slot1_full_format_enable_status.possible.swap(parsed_values);
                }
                break;
            case SDK::CrDevicePropertyCode::CrDeviceProperty_MediaSLOT2_FormatEnableStatus:
                nval = prop.GetValueSize() / sizeof(std::uint8_t);
                m_prop.media_slot2_full_format_enable_status.writable = prop.IsSetEnableCurrentValue();
                m_prop.media_slot2_full_format_enable_status.current = static_cast<std::uint8_t>(prop.GetCurrentValue());
                if (nval != m_prop.media_slot2_full_format_enable_status.possible.size()) {
                    auto parsed_values = parse_media_slotx_format_enable_status(prop.GetValues(), nval);
                    m_prop.media_slot2_full_format_enable_status.possible.swap(parsed_values);
                }
                break;
            case SDK::CrDevicePropertyCode::CrDeviceProperty_MediaSLOT1_QuickFormatEnableStatus:
                nval = prop.GetValueSize() / sizeof(std::uint8_t);
                m_prop.media_slot1_quick_format_enable_status.writable = prop.IsSetEnableCurrentValue();
                m_prop.media_slot1_quick_format_enable_status.current = static_cast<std::uint8_t>(prop.GetCurrentValue());
                if (nval != m_prop.media_slot1_quick_format_enable_status.possible.size()) {
                    auto parsed_values = parse_media_slotx_format_enable_status(prop.GetValues(), nval);
                    m_prop.media_slot1_quick_format_enable_status.possible.swap(parsed_values);
                }
                break;
            case SDK::CrDevicePropertyCode::CrDeviceProperty_MediaSLOT2_QuickFormatEnableStatus:
                nval = prop.GetValueSize() / sizeof(std::uint8_t);
                m_prop.media_slot2_quick_format_enable_status.writable = prop.IsSetEnableCurrentValue();
                m_prop.media_slot2_quick_format_enable_status.current = static_cast<std::uint8_t>(prop.GetCurrentValue());
                if (nval != m_prop.media_slot2_quick_format_enable_status.possible.size()) {
                    auto parsed_values = parse_media_slotx_format_enable_status(prop.GetValues(), nval);
                    m_prop.media_slot2_quick_format_enable_status.possible.swap(parsed_values);
                }
                break;
            case SDK::CrDevicePropertyCode::CrDeviceProperty_WhiteBalance:
                nval = prop.GetValueSize() / sizeof(std::uint16_t);
                m_prop.white_balance.writable = prop.IsSetEnableCurrentValue();
                m_prop.white_balance.current = static_cast<std::uint16_t>(prop.GetCurrentValue());
                if (0 < nval) {
                    auto parsed_values = parse_white_balance(prop.GetValues(), nval);
                    m_prop.white_balance.possible.swap(parsed_values);
                }
                break;
            case SDK::CrDevicePropertyCode::CrDeviceProperty_CustomWB_Capture_Standby:
                nval = prop.GetValueSize() / sizeof(std::uint16_t);
                m_prop.customwb_capture_standby.writable = prop.IsSetEnableCurrentValue();
                m_prop.customwb_capture_standby.current = static_cast<std::uint16_t>(prop.GetCurrentValue());
                if (nval != m_prop.white_balance.possible.size()) {
                    auto parsed_values = parse_customwb_capture_standby(prop.GetValues(), nval);
                    m_prop.customwb_capture_standby.possible.swap(parsed_values);
                }
                break;
            case SDK::CrDevicePropertyCode::CrDeviceProperty_CustomWB_Capture_Standby_Cancel:
                nval = prop.GetValueSize() / sizeof(std::uint16_t);
                m_prop.customwb_capture_standby_cancel.writable = prop.IsSetEnableCurrentValue();
                m_prop.customwb_capture_standby_cancel.current = static_cast<std::uint16_t>(prop.GetCurrentValue());
                if (nval != m_prop.customwb_capture_standby_cancel.possible.size()) {
                    auto parsed_values = parse_customwb_capture_standby_cancel(prop.GetValues(), nval);
                    m_prop.customwb_capture_standby_cancel.possible.swap(parsed_values);
                }
                break;
            case SDK::CrDevicePropertyCode::CrDeviceProperty_CustomWB_Capture_Operation:
                nval = prop.GetValueSize() / sizeof(std::uint16_t);
                m_prop.customwb_capture_operation.writable = prop.IsSetEnableCurrentValue();
                m_prop.customwb_capture_operation.current = static_cast<std::uint16_t>(prop.GetCurrentValue());
                if (0 < nval) {
                    auto parsed_values = parse_customwb_capture_operation(prop.GetValues(), nval);
                    m_prop.customwb_capture_operation.possible.swap(parsed_values);
                }
                break;
            case SDK::CrDevicePropertyCode::CrDeviceProperty_CustomWB_Execution_State:
                nval = prop.GetValueSize() / sizeof(std::uint16_t);
                m_prop.customwb_capture_execution_state.writable = prop.IsSetEnableCurrentValue();
                m_prop.customwb_capture_execution_state.current = static_cast<std::uint16_t>(prop.GetCurrentValue());
                if (nval != m_prop.customwb_capture_execution_state.possible.size()) {
                    auto parsed_values = parse_customwb_capture_execution_state(prop.GetValues(), nval);
                    m_prop.customwb_capture_execution_state.possible.swap(parsed_values);
                }
                break;
            case SDK::CrDevicePropertyCode::CrDeviceProperty_Zoom_Operation_Status:
                nval = prop.GetValueSize() / sizeof(std::uint8_t);
                m_prop.zoom_operation_status.writable = prop.IsSetEnableCurrentValue();
                m_prop.zoom_operation_status.current = static_cast<std::uint8_t>(prop.GetCurrentValue());
                if (nval != m_prop.zoom_operation_status.possible.size()) {
                    auto parsed_values = parse_zoom_operation_status(prop.GetValues(), nval);
                    m_prop.zoom_operation_status.possible.swap(parsed_values);
                }
                break;
            case SDK::CrDevicePropertyCode::CrDeviceProperty_Zoom_Setting:
                nval = prop.GetValueSize() / sizeof(std::uint8_t);
                m_prop.zoom_setting_type.writable = prop.IsSetEnableCurrentValue();
                m_prop.zoom_setting_type.current = static_cast<std::uint8_t>(prop.GetCurrentValue());
                if (0 < nval) {
                    auto parsed_values = parse_zoom_setting_type(prop.GetValues(), nval);
                    m_prop.zoom_setting_type.possible.swap(parsed_values);
                }
                break;
            case SDK::CrDevicePropertyCode::CrDeviceProperty_Zoom_Type_Status:
                nval = prop.GetValueSize() / sizeof(std::uint8_t);
                m_prop.zoom_types_status.writable = prop.IsSetEnableCurrentValue();
                m_prop.zoom_types_status.current = static_cast<std::uint8_t>(prop.GetCurrentValue());
                if (nval != m_prop.zoom_types_status.possible.size()) {
                    auto parsed_values = parse_zoom_types_status(prop.GetValues(), nval);
                    m_prop.zoom_types_status.possible.swap(parsed_values);
                }
                break;
            case SDK::CrDevicePropertyCode::CrDeviceProperty_Zoom_Speed_Range:
                nval = prop.GetValueSize() / sizeof(std::uint8_t);
                m_prop.zoom_speed_range.writable = prop.IsSetEnableCurrentValue();
                if (0 < nval) {
                    auto parsed_values = parse_zoom_speed_range(prop.GetValues(), nval);
                    m_prop.zoom_speed_range.possible.swap(parsed_values);
                }
                break;
            case SDK::CrDevicePropertyCode::CrDeviceProperty_ZoomAndFocusPosition_Save:
                nval = prop.GetValueSize() / sizeof(std::uint8_t);
                m_prop.save_zoom_and_focus_position.writable = prop.IsSetEnableCurrentValue();
                if (0 < nval) {
                    auto parsed_values = parse_save_zoom_and_focus_position(prop.GetValues(), nval);
                    m_prop.save_zoom_and_focus_position.possible.swap(parsed_values);
                }
                break;
            case SDK::CrDevicePropertyCode::CrDeviceProperty_ZoomAndFocusPosition_Load:
                nval = prop.GetValueSize() / sizeof(std::uint8_t);
                m_prop.load_zoom_and_focus_position.writable = prop.IsSetEnableCurrentValue();
                if (0 < nval) {
                    auto parsed_values = parse_load_zoom_and_focus_position(prop.GetValues(), nval);
                    m_prop.load_zoom_and_focus_position.possible.swap(parsed_values);
                }
                break;
            case SDK::CrDevicePropertyCode::CrDeviceProperty_Remocon_Zoom_Speed_Type:
                nval = prop.GetValueSize() / sizeof(std::uint8_t);
                m_prop.remocon_zoom_speed_type.writable = prop.IsSetEnableCurrentValue();
                m_prop.remocon_zoom_speed_type.current = static_cast<std::uint8_t>(prop.GetCurrentValue());
                if (0 < nval) {
                    auto parsed_values = parse_remocon_zoom_speed_type(prop.GetValues(), nval);
                    m_prop.remocon_zoom_speed_type.possible.swap(parsed_values);
                }
                break;
            case SDK::CrDevicePropertyCode::CrDeviceProperty_APS_C_or_Full_SwitchingSetting:
                m_prop.aps_c_of_full_switching_setting.writable = prop.IsSetEnableCurrentValue();
                m_prop.aps_c_of_full_switching_setting.current = static_cast<std::uint8_t>(prop.GetCurrentValue());
                break;
            case SDK::CrDevicePropertyCode::CrDeviceProperty_APS_C_or_Full_SwitchingEnableStatus:
                m_prop.aps_c_of_full_switching_enable_status.writable = prop.IsSetEnableCurrentValue();
                m_prop.aps_c_of_full_switching_enable_status.current = static_cast<std::uint8_t>(prop.GetCurrentValue());
                break;
            case SDK::CrDevicePropertyCode::CrDeviceProperty_CameraSetting_SaveOperationEnableStatus:
                m_prop.camera_setting_save_operation.writable = prop.IsSetEnableCurrentValue();
                m_prop.camera_setting_save_operation.current = static_cast<std::uint8_t>(prop.GetCurrentValue());
                break;
            case SDK::CrDevicePropertyCode::CrDeviceProperty_CameraSetting_ReadOperationEnableStatus:
                m_prop.camera_setting_read_operation.writable = prop.IsSetEnableCurrentValue();
                m_prop.camera_setting_read_operation.current = static_cast<std::uint8_t>(prop.GetCurrentValue());
                break;
            case SDK::CrDevicePropertyCode::CrDeviceProperty_CameraSetting_SaveRead_State:
                m_prop.camera_setting_save_read_state.writable = prop.IsSetEnableCurrentValue();
                m_prop.camera_setting_save_read_state.current = static_cast<std::uint8_t>(prop.GetCurrentValue());
                break;
            case SDK::CrDevicePropertyCode::CrDeviceProperty_CameraSettingsResetEnableStatus:
                m_prop.camera_setting_reset_enable_status.writable = prop.IsSetEnableCurrentValue();
                m_prop.camera_setting_reset_enable_status.current = static_cast<std::uint8_t>(prop.GetCurrentValue());
                break;
            case SDK::CrDevicePropertyCode::CrDeviceProperty_PlaybackMedia:
                nval = prop.GetValueSize() / sizeof(std::uint8_t);
                m_prop.playback_media.writable = prop.IsSetEnableCurrentValue();
                m_prop.playback_media.current = static_cast<std::uint8_t>(prop.GetCurrentValue());
                if (0 < nval) {
                    auto parsed_values = parse_playback_media(prop.GetValues(), nval);
                    m_prop.playback_media.possible.swap(parsed_values);
                }
                break;
            case SDK::CrDevicePropertyCode::CrDeviceProperty_GainBaseSensitivity:
                nval = prop.GetValueSize() / sizeof(std::uint8_t);
                m_prop.gain_base_sensitivity.writable = prop.IsSetEnableCurrentValue();
                m_prop.gain_base_sensitivity.current = static_cast<std::uint8_t>(prop.GetCurrentValue());
                if (0 < nval) {
                    auto parsed_values = parse_gain_base_sensitivity(prop.GetValues(), nval);
                    m_prop.gain_base_sensitivity.possible.swap(parsed_values);
                }
                break;
            case SDK::CrDevicePropertyCode::CrDeviceProperty_GainBaseIsoSensitivity:
                nval = prop.GetValueSize() / sizeof(std::uint8_t);
                m_prop.gain_base_iso_sensitivity.writable = prop.IsSetEnableCurrentValue();
                m_prop.gain_base_iso_sensitivity.current = static_cast<std::uint8_t>(prop.GetCurrentValue());
                if (0 < nval) {
                    auto parsed_values = parse_gain_base_iso_sensitivity(prop.GetValues(), nval);
                    m_prop.gain_base_iso_sensitivity.possible.swap(parsed_values);
                }
                break;
            case SDK::CrDevicePropertyCode::CrDeviceProperty_MonitorLUTSetting:
                nval = prop.GetValueSize() / sizeof(std::uint8_t);
                m_prop.monitor_lut_setting.writable = prop.IsSetEnableCurrentValue();
                m_prop.monitor_lut_setting.current = static_cast<std::uint8_t>(prop.GetCurrentValue());
                if (0 < nval) {
                    auto parsed_values = parse_monitor_lut_setting(prop.GetValues(), nval);
                    m_prop.monitor_lut_setting.possible.swap(parsed_values);
                }
                break;
            case SDK::CrDevicePropertyCode::CrDeviceProperty_ExposureIndex:
                nval = prop.GetValueSize() / sizeof(std::uint16_t);
                m_prop.exposure_index.writable = prop.IsSetEnableCurrentValue();
                m_prop.exposure_index.current = static_cast<std::uint16_t>(prop.GetCurrentValue());
                if (0 < nval) {
                    auto parsed_values = parse_exposure_index(prop.GetValues(), nval);
                    m_prop.exposure_index.possible.swap(parsed_values);
                }
                break;
            case SDK::CrDevicePropertyCode::CrDeviceProperty_BaseLookValue:
                nval = prop.GetValueSize() / sizeof(std::uint16_t);
                m_prop.baselook_value.writable = prop.IsSetEnableCurrentValue();
                m_prop.baselook_value.current = static_cast<std::uint16_t>(prop.GetCurrentValue());
                if (0 < nval) {
                    auto parsed_values = parse_baselook_value(prop.GetValues(), nval);
                    m_prop.baselook_value.possible.swap(parsed_values);
                }
                break;
            case SDK::CrDevicePropertyCode::CrDeviceProperty_IrisModeSetting:
                nval = prop.GetValueSize() / sizeof(std::uint8_t);
                m_prop.iris_mode_setting.writable = prop.IsSetEnableCurrentValue();
                m_prop.iris_mode_setting.current = static_cast<std::uint8_t>(prop.GetCurrentValue());
                if (0 < nval) {
                    auto parsed_values = parse_iris_mode_setting(prop.GetValues(), nval);
                    m_prop.iris_mode_setting.possible.swap(parsed_values);
                }
                break;
            case SDK::CrDevicePropertyCode::CrDeviceProperty_ShutterModeSetting:
                nval = prop.GetValueSize() / sizeof(std::uint8_t);
                m_prop.shutter_mode_setting.writable = prop.IsSetEnableCurrentValue();
                m_prop.shutter_mode_setting.current = static_cast<std::uint8_t>(prop.GetCurrentValue());
                if (0 < nval) {
                    auto parsed_values = parse_shutter_mode_setting(prop.GetValues(), nval);
                    m_prop.shutter_mode_setting.possible.swap(parsed_values);
                }
                break;
            case SDK::CrDevicePropertyCode::CrDeviceProperty_GainControlSetting:
                nval = prop.GetValueSize() / sizeof(std::uint8_t);
                m_prop.gain_control_setting.writable = prop.IsSetEnableCurrentValue();
                m_prop.gain_control_setting.current = static_cast<std::uint8_t>(prop.GetCurrentValue());
                if (0 < nval) {
                    auto parsed_values = parse_gain_control_setting(prop.GetValues(), nval);
                    m_prop.gain_control_setting.possible.swap(parsed_values);
                }
                break;
            case SDK::CrDevicePropertyCode::CrDeviceProperty_ExposureCtrlType:
                nval = prop.GetValueSize() / sizeof(std::uint8_t);
                m_prop.exposure_control_type.writable = prop.IsSetEnableCurrentValue();
                m_prop.exposure_control_type.current = static_cast<std::uint8_t>(prop.GetCurrentValue());
                if (0 < nval) {
                    auto parsed_values = parse_exposure_control_type(prop.GetValues(), nval);
                    m_prop.exposure_control_type.possible.swap(parsed_values);
                }
                break;
            case SDK::CrDevicePropertyCode::CrDeviceProperty_IsoCurrentSensitivity:
                m_prop.iso_current_sensitivity.writable = prop.IsSetEnableCurrentValue();
                m_prop.iso_current_sensitivity.current = static_cast<std::uint32_t>(prop.GetCurrentValue());
                break;
            case SDK::CrDevicePropertyCode::CrDeviceProperty_Movie_Recording_Setting:
                nval = prop.GetValueSize() / sizeof(std::uint16_t);
                m_prop.recording_setting.writable = prop.IsSetEnableCurrentValue();
                m_prop.recording_setting.current = static_cast<std::uint16_t>(prop.GetCurrentValue());
                if (0 < nval) {
                    auto parsed_values = parse_recording_setting(prop.GetValues(), nval);
                    m_prop.recording_setting.possible.swap(parsed_values);
                }
                break;
            case SDK::CrDevicePropertyCode::CrDeviceProperty_DispModeCandidate:
                nval = prop.GetValueSize() / sizeof(std::uint32_t);
                m_prop.dispmode_candidate.writable = prop.IsSetEnableCurrentValue();
                m_prop.dispmode_candidate.current = static_cast<std::uint32_t>(prop.GetCurrentValue());
                if (0 < nval) {
                    auto parsed_values = parse_dispmode_candidate(prop.GetValues(), nval);
                    m_prop.dispmode_candidate.possible.swap(parsed_values);
                }
                break;
            case SDK::CrDevicePropertyCode::CrDeviceProperty_DispModeSetting:
                nval = prop.GetValueSize() / sizeof(std::uint32_t);
                m_prop.dispmode_setting.writable = prop.IsSetEnableCurrentValue();
                m_prop.dispmode_setting.current = static_cast<std::uint32_t>(prop.GetCurrentValue());
                if (0 < nval) {
                    auto parsed_values = parse_dispmode_setting(prop.GetValues(), nval);
                    m_prop.dispmode_setting.possible.swap(parsed_values);
                }
                break;
            case SDK::CrDevicePropertyCode::CrDeviceProperty_DispMode:
                nval = prop.GetValueSize() / sizeof(std::uint8_t);
                m_prop.dispmode.writable = prop.IsSetEnableCurrentValue();
                m_prop.dispmode.current = static_cast<std::uint8_t>(prop.GetCurrentValue());
                if (0 < nval) {
                    auto parsed_values = parse_dispmode(prop.GetValues(), nval);
                    m_prop.dispmode.possible.swap(parsed_values);
                }
                break;
            case SDK::CrDevicePropertyCode::CrDeviceProperty_GaindBValue:
                nval = prop.GetValueSize() / sizeof(std::int8_t);
                m_prop.gain_db_value.writable = prop.IsSetEnableCurrentValue();
                m_prop.gain_db_value.current = static_cast<std::int8_t>(prop.GetCurrentValue());
                if (0 < nval) {
                    auto parsed_values = parse_gain_db_value(prop.GetValues(), nval);
                    m_prop.gain_db_value.possible.swap(parsed_values);
                }
                break;
            case SDK::CrDevicePropertyCode::CrDeviceProperty_WhiteBalanceTint:
                nval = prop.GetValueSize() / sizeof(std::int8_t);
                m_prop.white_balance_tint.writable = prop.IsSetEnableCurrentValue();
                m_prop.white_balance_tint.current = static_cast<std::int8_t>(prop.GetCurrentValue());
                if (0 < nval) {
                    auto parsed_values = parse_white_balance_tint(prop.GetValues(), nval);
                    m_prop.white_balance_tint.possible.swap(parsed_values);
                }
                break;
            case SDK::CrDevicePropertyCode::CrDeviceProperty_WhiteBalanceTintStep:
                nval = prop.GetValueSize() / sizeof(std::int16_t);
                m_prop.white_balance_tint_step.writable = prop.IsSetEnableCurrentValue();
                m_prop.white_balance_tint_step.current = static_cast<std::int16_t>(prop.GetCurrentValue());
                if (0 < nval) {
                    auto parsed_values = parse_white_balance_tint_step(prop.GetValues(), nval);
                    m_prop.white_balance_tint_step.possible.swap(parsed_values);
                }
                break;
            case SDK::CrDevicePropertyCode::CrDeviceProperty_MovieRecButtonToggleEnableStatus:
                m_prop.movie_rec_button_toggle_enable_status.writable = prop.IsSetEnableCurrentValue();
                m_prop.movie_rec_button_toggle_enable_status.current = static_cast<std::uint8_t>(prop.GetCurrentValue());
                break;
            case SDK::CrDevicePropertyCode::CrDeviceProperty_ShutterSpeedValue:
                nval = prop.GetValueSize() / sizeof(std::uint64_t);
                m_prop.shutter_speed_value.writable = prop.IsSetEnableCurrentValue();
                m_prop.shutter_speed_value.current = static_cast<std::uint64_t>(prop.GetCurrentValue());
                if (0 < nval) {
                    auto parsed_values = parse_shutter_speed_value(prop.GetValues(), nval);
                    m_prop.shutter_speed_value.possible.swap(parsed_values);
                }
                break;
            case SDK::CrDevicePropertyCode::CrDeviceProperty_MediaSLOT1_Status:
                m_prop.media_slot1_status.writable = prop.IsSetEnableCurrentValue();
                m_prop.media_slot1_status.current = static_cast<std::uint16_t>(prop.GetCurrentValue());
                break;
            case SDK::CrDevicePropertyCode::CrDeviceProperty_MediaSLOT2_Status:
                m_prop.media_slot2_status.writable = prop.IsSetEnableCurrentValue();
                m_prop.media_slot2_status.current = static_cast<std::uint16_t>(prop.GetCurrentValue());
                break;
            case SDK::CrDevicePropertyCode::CrDeviceProperty_FocusBracketShotNumber:
                nval = prop.GetValueSize() / sizeof(std::uint16_t);
                m_prop.focus_bracket_shot_num.writable = prop.IsSetEnableCurrentValue();
                m_prop.focus_bracket_shot_num.current = static_cast<std::uint16_t>(prop.GetCurrentValue());
                if (0 < nval) {
                    auto parsed_values = parse_focus_bracket_shot_num(prop.GetValues(), nval);
                    m_prop.focus_bracket_shot_num.possible.swap(parsed_values);
                }
                break;
            case SDK::CrDevicePropertyCode::CrDeviceProperty_FocusBracketFocusRange:
                nval = prop.GetValueSize() / sizeof(std::uint8_t);
                m_prop.focus_bracket_focus_range.writable = prop.IsSetEnableCurrentValue();
                m_prop.focus_bracket_focus_range.current = static_cast<std::uint8_t>(prop.GetCurrentValue());
                if (0 < nval) {
                    auto parsed_values = parse_focus_bracket_focus_range(prop.GetValues(), nval);
                    m_prop.focus_bracket_focus_range.possible.swap(parsed_values);
                }
                break;
            case SDK::CrDevicePropertyCode::CrDeviceProperty_Movie_ImageStabilizationSteadyShot:
                nval = prop.GetValueSize() / sizeof(std::uint8_t);
                m_prop.movie_image_stabilization_steady_shot.writable = prop.IsSetEnableCurrentValue();
                m_prop.movie_image_stabilization_steady_shot.current = static_cast<std::uint8_t>(prop.GetCurrentValue());
                if (0 < nval) {
                    auto parsed_values = parse_movie_image_stabilization_steady_shot(prop.GetValues(), nval);
                    m_prop.movie_image_stabilization_steady_shot.possible.swap(parsed_values);
                }
                break;
            case SDK::CrDevicePropertyCode::CrDeviceProperty_ImageStabilizationSteadyShot:
                nval = prop.GetValueSize() / sizeof(std::uint8_t);
                m_prop.image_stabilization_steady_shot.writable = prop.IsSetEnableCurrentValue();
                m_prop.image_stabilization_steady_shot.current = static_cast<std::uint8_t>(prop.GetCurrentValue());
                if (0 < nval) {
                    auto parsed_values = parse_image_stabilization_steady_shot(prop.GetValues(), nval);
                    m_prop.image_stabilization_steady_shot.possible.swap(parsed_values);
                }
                break;
            case SDK::CrDevicePropertyCode::CrDeviceProperty_SilentMode:
                nval = prop.GetValueSize() / sizeof(std::uint8_t);
                m_prop.silent_mode.writable = prop.IsSetEnableCurrentValue();
                m_prop.silent_mode.current = static_cast<std::uint8_t>(prop.GetCurrentValue());
                if (0 < nval) {
                    auto parsed_values = parse_silent_mode(prop.GetValues(), nval);
                    m_prop.silent_mode.possible.swap(parsed_values);
                }
                break;
            case SDK::CrDevicePropertyCode::CrDeviceProperty_SilentModeApertureDriveInAF:
                nval = prop.GetValueSize() / sizeof(std::uint8_t);
                m_prop.silent_mode_aperture_drive_in_af.writable = prop.IsSetEnableCurrentValue();
                m_prop.silent_mode_aperture_drive_in_af.current = static_cast<std::uint8_t>(prop.GetCurrentValue());
                if (0 < nval) {
                    auto parsed_values = parse_silent_mode_aperture_drive_in_af(prop.GetValues(), nval);
                    m_prop.silent_mode_aperture_drive_in_af.possible.swap(parsed_values);
                }
                break;
            case SDK::CrDevicePropertyCode::CrDeviceProperty_SilentModeShutterWhenPowerOff:
                nval = prop.GetValueSize() / sizeof(std::uint8_t);
                m_prop.silent_mode_shutter_when_power_off.writable = prop.IsSetEnableCurrentValue();
                m_prop.silent_mode_shutter_when_power_off.current = static_cast<std::uint8_t>(prop.GetCurrentValue());
                if (0 < nval) {
                    auto parsed_values = parse_silent_mode_shutter_when_power_off(prop.GetValues(), nval);
                    m_prop.silent_mode_shutter_when_power_off.possible.swap(parsed_values);
                }
                break;
            case SDK::CrDevicePropertyCode::CrDeviceProperty_SilentModeAutoPixelMapping:
                nval = prop.GetValueSize() / sizeof(std::uint8_t);
                m_prop.silent_mode_auto_pixel_mapping.writable = prop.IsSetEnableCurrentValue();
                m_prop.silent_mode_auto_pixel_mapping.current = static_cast<std::uint8_t>(prop.GetCurrentValue());
                if (0 < nval) {
                    auto parsed_values = parse_silent_mode_auto_pixel_mapping(prop.GetValues(), nval);
                    m_prop.silent_mode_auto_pixel_mapping.possible.swap(parsed_values);
                }
                break;
            case SDK::CrDevicePropertyCode::CrDeviceProperty_ShutterType:
                nval = prop.GetValueSize() / sizeof(std::uint8_t);
                m_prop.shutter_type.writable = prop.IsSetEnableCurrentValue();
                m_prop.shutter_type.current = static_cast<std::uint8_t>(prop.GetCurrentValue());
                if (0 < nval) {
                    auto parsed_values = parse_shutter_type(prop.GetValues(), nval);
                    m_prop.shutter_type.possible.swap(parsed_values);
                }
                break;
            case SDK::CrDevicePropertyCode::CrDeviceProperty_MovieShootingMode:
                nval = prop.GetValueSize() / sizeof(std::uint16_t);
                m_prop.movie_shooting_mode.writable = prop.IsSetEnableCurrentValue();
                m_prop.movie_shooting_mode.current = static_cast<std::uint16_t>(prop.GetCurrentValue());
                if (0 < nval) {
                    auto parsed_values = parse_movie_shooting_mode(prop.GetValues(), nval);
                    m_prop.movie_shooting_mode.possible.swap(parsed_values);
                }
                break;
            default:
                break;
            }
        }
        SDK::ReleaseDeviceProperties(m_device_handle, prop_list);
    }
}

void CameraDevice::get_property(SDK::CrDeviceProperty& prop) const
{
    SDK::CrDeviceProperty* properties = nullptr;
    int nprops = 0;
    SDK::GetDeviceProperties(m_device_handle, &properties, &nprops);
}

bool CameraDevice::set_property(SDK::CrDeviceProperty& prop) const
{
    SDK::SetDeviceProperty(m_device_handle, &prop);
    return false;
}

void CameraDevice::getContentsList()
{
    // check status
    std::int32_t nprop = 0;
    SDK::CrDeviceProperty* prop_list = nullptr;
    CrInt32u getCode = SDK::CrDevicePropertyCode::CrDeviceProperty_ContentsTransferStatus;
    SDK::CrError res = SDK::GetSelectDeviceProperties(m_device_handle, 1, &getCode, &prop_list, &nprop);
    bool bExec = false;
    if (CR_SUCCEEDED(res) && (1 == nprop)) {
        if ((getCode == prop_list[0].GetCode()) && (SDK::CrContentsTransfer_ON == prop_list[0].GetCurrentValue()))
        {
            bExec = true;
        }
        SDK::ReleaseDeviceProperties(m_device_handle, prop_list);
    }
    if (false == bExec) {
        tout << "GetContentsListEnableStatus is Disable. Do it after it becomes Enable.\n";
        return;
    }

    for (CRFolderInfos* pF : m_foldList)
    {
        delete pF;
    }
    m_foldList.clear();
    for (SCRSDK::CrMtpContentsInfo* pC : m_contentList)
    {
        delete pC;
    }
    m_contentList.clear();

    CrInt32u f_nums = 0;
    CrInt32u c_nums = 0;
    SDK::CrMtpFolderInfo* f_list = nullptr;
    SDK::CrError err = SDK::GetDateFolderList(m_device_handle, &f_list, &f_nums);
    if (CR_SUCCEEDED(err) && 0 < f_nums)
    {
        if (f_list)
        {
            tout << "NumOfFolder [" << f_nums << "]" << std::endl;

            for (int i = 0; i < f_nums; ++i)
            {
                auto pFold = new SDK::CrMtpFolderInfo();
                pFold->handle = f_list[i].handle;
                pFold->folderNameSize = f_list[i].folderNameSize;
                CrInt32u lenByOS = sizeof(CrChar) * pFold->folderNameSize;
                pFold->folderName = new CrChar[lenByOS];
                MemCpyEx(pFold->folderName, f_list[i].folderName, lenByOS);
                CRFolderInfos* pCRF = new CRFolderInfos(pFold, 0); // 2nd : fill in later
                m_foldList.push_back(pCRF);
            }
            SDK::ReleaseDateFolderList(m_device_handle, f_list);
        }

        if (0 == m_foldList.size())
        {
            return;
        }

        MtpFolderList::iterator it = m_foldList.begin();
        for (int fcnt = 0; it != m_foldList.end(); ++fcnt, ++it)
        {
            SDK::CrContentHandle* c_list = nullptr;
            err = SDK::GetContentsHandleList(m_device_handle, (*it)->pFolder->handle, &c_list, &c_nums);
            if (CR_SUCCEEDED(err) && 0 < c_nums)
            {
                if (c_list)
                {
                    tout << "(" << (fcnt + 1) << "/" << f_nums << ") NumOfContents [" << c_nums << "]" << std::endl;
                    (*it)->numOfContents = c_nums;
                    for (int i = 0; i < c_nums; i++)
                    {
                        SDK::CrMtpContentsInfo* pContents = new SDK::CrMtpContentsInfo();
                        err = SDK::GetContentsDetailInfo(m_device_handle, c_list[i], pContents);
                        if (CR_SUCCEEDED(err))
                        {
                            m_contentList.push_back(pContents);
                            // progress
                            if (0 == ((i + 1) % 100))
                            {
                                tout << "  ... " << (i + 1) << "/" << c_nums << std::endl;
                            }
                        }
                        else
                        {
                            break;
                        }
                    }
                    SDK::ReleaseContentsHandleList(m_device_handle, c_list);
                }
            }
            if (CR_FAILED(err))
            {
                break;
            }
        }
    }
    else if (CR_SUCCEEDED(err) && 0 == f_nums)
    {
        tout << "No images in memory card." << std::endl;
        return;
    }
    else
    {
        // err
        tout << "Failed SDK::GetContentsList()" << std::endl;
        return;
    }

    if (CR_SUCCEEDED(err))
    {
        MtpFolderList::iterator itF = m_foldList.begin();
        for (std::int32_t f_sep = 0; itF != m_foldList.end(); ++f_sep, ++itF)
        {
            text fname((*itF)->pFolder->folderName);
 
            tout << "===== ";
            tout.fill(' ');
            tout.width(3);
            tout << (f_sep + 1) << ": ";
            
            tout << fname;

            tout << " (0x";
            std::ostringstream f_handle_hex;
            f_handle_hex << std::hex << std::uppercase << (*itF)->pFolder->handle;
            tout << std::dec;
            std::string f_handle_str = f_handle_hex.str();
            f_handle_str.erase(std::remove(f_handle_str.begin(), f_handle_str.end(), ','), f_handle_str.end());
            const char* f_handle_char = f_handle_str.c_str();
            tout.fill('0');
            tout.width(8);
            tout << f_handle_char << ") , ";
            tout << "contents[" << (*itF)->numOfContents << "] ===== \n";

            MtpContentsList::iterator itC = m_contentList.begin();
            for (std::int32_t i = 0; itC != m_contentList.end(); ++i, ++itC)
            {
                if ((*itC)->parentFolderHandle == (*itF)->pFolder->handle)
                {
                    text fname((*itC)->fileName);

                    tout << "  ";
                    tout.fill(' ');
                    tout.width(3);
                    tout << (i + 1);
                    tout << ": (0x";
                    std::ostringstream c_handle_hex;
                    c_handle_hex << std::hex << std::uppercase << (*itC)->handle;
                    tout << std::dec;
                    std::string c_handle_str = c_handle_hex.str();
                    c_handle_str.erase(std::remove(c_handle_str.begin(), c_handle_str.end(), ','), c_handle_str.end());
                    const char* c_handle_char = c_handle_str.c_str();
                    tout.fill('0');
                    tout.width(8);
                    tout << c_handle_char << "), ";
                 
                    tout << fname << std::endl;
                }
            }
        }

        while (1)
        {
            if (m_connected == false) {
                break;
            }
            text input;
            tout << std::endl << "Select the number of the contents you want to download:";
            tout << std::endl << "(Returns to the previous menu for invalid numbers)" << std::endl << std::endl;
            tout << std::endl << "input> ";
            std::getline(tin, input);
            text_stringstream ss(input);
            int selected_index = 0;
            ss >> selected_index;
            if (selected_index < 1 || m_contentList.size() < selected_index)
            {
                if (m_connected != false) {
                    tout << "Input cancelled.\n";
                }
                break;
            }
            else
            {
                while (1)
                {
                    if (m_connected == false) {
                        break;
                    }
                    auto targetHandle = m_contentList[selected_index - 1]->handle;

                    tout << "Selected(0x ";
                    std::ostringstream targetHandle_hex;
                    targetHandle_hex << std::hex << std::uppercase << targetHandle;
                    tout << std::dec;
                    std::string targetHandle_str = targetHandle_hex.str();
                    targetHandle_str.erase(std::remove(targetHandle_str.begin(), targetHandle_str.end(), ','), targetHandle_str.end());
                    const char* targetHandle_char = targetHandle_str.c_str();
                    tout.fill('0');
                    tout.width(4);
                    tout << targetHandle_char << ") ... \n";


                    text input;
                    tout << std::endl << "Select the number of the content size you want to download:";
                    tout << std::endl << "[-1] Cancel input";
                    tout << std::endl << "[1] Original";
                    tout << std::endl << "[2] Thumbnail";
                    text selected_filename(m_contentList[selected_index - 1]->fileName);
                    text ext = selected_filename.substr(selected_filename.length() - 4, 4);
                    int checkMax = 2;
                    if ((0 == ext.compare(TEXT(".JPG"))) || 
                        (0 == ext.compare(TEXT(".ARW"))) || 
                        (0 == ext.compare(TEXT(".HIF"))))
                    {
                        checkMax = 3;
                        tout << std::endl << "[3] 2M" << std::endl;
                    }

                    tout << std::endl << "input> ";
                    std::getline(tin, input);
                    text_stringstream ss(input);
                    int selected_contentSize = 0;
                    ss >> selected_contentSize;
                    if (m_connected == false) {
                        break;
                    }
                    if (selected_contentSize < 1 || checkMax < selected_contentSize)
                    {
                        if (m_connected != false) {
                            tout << "Input cancelled.\n";
                        }
                        break;
                    }
                    switch (selected_contentSize)
                    {
                    case 1:
                        // [async] get contents
                        pullContents(targetHandle);
                        break;
                    case 2:
                        // [sync] get thumbnail jpeg
                        getThumbnail(targetHandle);
                        break;
                    case 3:
                        // [async] [only still] get screennail jpeg
                        getScreennail(targetHandle);
                        break;
                    default:
                        break;
                    }
                    std::this_thread::sleep_for(2s);
                }
            }
        }
    }
}

void CameraDevice::pullContents(SDK::CrContentHandle content)
{
    SDK::CrError err = SDK::PullContentsFile(m_device_handle, content);

    if (SDK::CrError_None != err)
    {

        /*tout << "[Error] err=0x";
        std::ostringstream err_hex;
        err_hex << std::hex << std::uppercase << err;
        tout << std::dec;
        std::string err_str = err_hex.str();
        err_str.erase(std::remove(err_str.begin(), err_str.end(), ','), err_str.end());
        const char* err_char = err_str.c_str();
        tout.fill('0');
        tout.width(4);
        tout << err_char;
        tout << ", handle(0x";
        std::ostringstream content_hex;
        content_hex << std::hex << std::uppercase << content;
        tout << std::dec;
        std::string content_str = content_hex.str();
        content_str.erase(std::remove(content_str.begin(), content_str.end(), ','), content_str.end());
        const char* content_char = content_str.c_str();
        tout.fill('0');
        tout.width(8);
        tout << content_char << "), \n";*/

        text id(this->get_id());
        text msg = get_message_desc(err);
        if (!msg.empty()) {
            // output is 2 line
            tout << std::endl << msg.data() << ", handle=" << std::hex << content << std::dec << std::endl;
            tout << m_info->GetModel() << " (" << id.data() << ")" << std::endl;
        }
    }
}

void CameraDevice::getScreennail(SDK::CrContentHandle content)
{
    SDK::CrError err = SDK::PullContentsFile(m_device_handle, content, SDK::CrPropertyStillImageTransSize_SmallSize);

    if (SDK::CrError_None != err)
    {
        /*tout << "[Error] err=0x";
        std::ostringstream err_hex;
        err_hex << std::hex << std::uppercase << err;
        tout << std::dec;
        std::string err_str = err_hex.str();
        err_str.erase(std::remove(err_str.begin(), err_str.end(), ','), err_str.end());
        const char* err_char = err_str.c_str();
        tout.fill('0');
        tout.width(4);
        tout << err_char;
        tout << ", handle(0x";
        std::ostringstream content_hex;
        content_hex << std::hex << std::uppercase << content;
        tout << std::dec;
        std::string content_str = content_hex.str();
        content_str.erase(std::remove(content_str.begin(), content_str.end(), ','), content_str.end());
        const char* content_char = content_str.c_str();
        tout.fill('0');
        tout.width(8);
        tout << content_char << "), \n";*/

        text id(this->get_id());
        text msg = get_message_desc(err);
        if (!msg.empty()) {
            // output is 2 line
            tout << std::endl << msg.data() << ", handle=" << std::hex << content << std::dec << std::endl;
            tout << m_info->GetModel() << " (" << id.data() << ")" << std::endl;
        }
    }
}

void CameraDevice::getThumbnail(SDK::CrContentHandle content)
{
    CrInt32u bufSize = 0x28000; // @@@@ temp

    auto* image_data = new SDK::CrImageDataBlock();
    if (!image_data)
    {
        tout << "getThumbnail FAILED (new CrImageDataBlock class)\n";
        return;
    }
    CrInt8u* image_buff = new CrInt8u[bufSize];
    if (!image_buff)
    {
        delete image_data;
        tout << "getThumbnail FAILED (new Image buffer)\n";
        return;
    }
    image_data->SetSize(bufSize);
    image_data->SetData(image_buff);

    SDK::CrFileType fileType = SDK::CrFileType_None;
    SDK::CrError err = SDK::GetContentsThumbnailImage(m_device_handle, content, image_data, &fileType);
    if (CR_FAILED(err))
    {
        //tout << "[Error] err=0x";
        //std::ostringstream err_hex;
        //err_hex << std::hex << std::uppercase << err;
        //tout << std::dec;
        //std::string err_str = err_hex.str();
        //err_str.erase(std::remove(err_str.begin(), err_str.end(), ','), err_str.end());
        //const char* err_char = err_str.c_str();
        //tout.fill('0');
        //tout.width(4);
        //tout << err_char;
        //tout << ", handle(0x";
        //std::ostringstream content_hex;
        //content_hex << std::hex << std::uppercase << content;
        //tout << std::dec;
        //std::string content_str = content_hex.str();
        //content_str.erase(std::remove(content_str.begin(), content_str.end(), ','), content_str.end());
        //const char* content_char = content_str.c_str();
        //tout.fill('0');
        //tout.width(8);
        //tout << content_char << "), \n";

        text id(this->get_id());
        text msg = get_message_desc(err);
        if (!msg.empty()) {
            // output is 2 line
            tout << std::endl << msg.data() << ", handle=" << std::hex << content << std::dec << std::endl;
            tout << m_info->GetModel() << " (" << id.data() << ")" << std::endl;
        }
    }
    else
    {
        if (0 < image_data->GetSize() && fileType != SDK::CrFileType_None)
        {
            text filename(TEXT("Thumbnail.JPG"));
            if (fileType == SDK::CrFileType_Heif) {
                filename= (TEXT("Thumbnail.HIF"));
            }

#if defined(__APPLE__)
            char path[MAC_MAX_PATH]; /*MAX_PATH*/
            memset(path, 0, sizeof(path));
            if(NULL == getcwd(path, sizeof(path) - 1)){
                // FAILED
                delete[] image_buff; // Release
                delete image_data; // Release
                tout << "Folder path is too long.\n";
                return;
            };
            char* delimit = "/";
            if(strlen(path) + strlen(delimit) + filename.length() > MAC_MAX_PATH){
                // FAILED
                delete[] image_buff; // Release
                delete image_data; // Release
                tout << "Failed to create save path\n";
                return;
            }
            strncat(path, delimit, strlen(delimit));
            strncat(path, (CrChar*)filename.c_str(), filename.length());
#else
            auto path = fs::current_path();
            path.append(filename);
#endif
            tout << path << '\n';

            std::ofstream file(path, std::ios::out | std::ios::binary);
            if (!file.bad())
            {
                std::uint32_t len = image_data->GetImageSize();
                file.write((char*)image_data->GetImageData(), len);
                file.close();
            }
        }
    }
    delete[] image_buff; // Release
    delete image_data; // Release
}

text CameraDevice::format_display_string_type(SDK::CrDisplayStringType type) {
    text_stringstream ts;
    switch (type)
    {
    case SCRSDK::CrDisplayStringType_AllList:
        ts << "All Display List";
        break;
    case SCRSDK::CrDisplayStringType_BaseLook_AELevelOffset_ExposureValue:
        ts << "[BaseLook] AELevelOffset ExposureValue";
        break;
    case SCRSDK::CrDisplayStringType_BaseLook_Input_Display:
        ts << "[BaseLook] Input Display";
        break;
    case SCRSDK::CrDisplayStringType_BaseLook_Name_Display:
        ts << "[BaseLook] Name Display";
        break;
    case SCRSDK::CrDisplayStringType_BaseLook_Output_Display:
        ts << "[BaseLook] Output Display";
        break;
    case SCRSDK::CrDisplayStringType_SceneFile_Name_Display:
        ts << "[SceneFile] Name Display";
        break;
    case SCRSDK::CrDisplayStringType_ShootingMode_Cinema_ColorGamut_Display:
        ts << "[ShootingMode] Cinema ColorGamut Display";
        break;
    case SCRSDK::CrDisplayStringType_ShootingMode_TargetDisplay_Display:
        ts << "[ShootingMode] TargetDisplay Display";
        break;
    case SCRSDK::CrDisplayStringType_Camera_Gain_BaseISO_Display:
        ts << "[Camera Gain BaseISO] Display";
        break;
    case SCRSDK::CrDisplayStringType_Video_EIGain_Display:
        ts << "[Video EIGain] Display";
        break;
    case SCRSDK::CrDisplayStringType_Button_Assign_Display:
        ts << "[Button Assign] Display";
        break;
    case SCRSDK::CrDisplayStringType_Button_Assign_ShortDisplay:
        ts << "[Button Assign] ShortDisplay";
        break;
    case SCRSDK::CrDisplayStringType_Reserved1:
        ts << "reserved";
        break;
    case SCRSDK::CrDisplayStringType_Reserved2:
        ts << "reserved";
        break;
    case SCRSDK::CrDisplayStringType_Reserved3:
        ts << "reserved";
        break;
    case SCRSDK::CrDisplayStringType_Reserved4:
        ts << "reserved";
        break;
    case SCRSDK::CrDisplayStringType_Reserved5:
        ts << "reserved";
        break;
    case SCRSDK::CrDisplayStringType_Reserved6:
        ts << "reserved";
        break;
    case SCRSDK::CrDisplayStringType_Reserved7:
        ts << "reserved";
        break;
    case SCRSDK::CrDisplayStringType_CreativeLook_Name_Display:
        ts << "[Creative Look] Name Display";
        break;
    default:
        ts << "end";
        break;
    }
    return ts.str();
}

void CameraDevice::execute_request_displaystringlist() {
    SDK::CrDisplayStringType type;

    tout << "Select Display String List Type. \n";
    tout << "[-1] Cancel input\n";
    
    std::vector<int> list_type;
    int i = SDK::CrDisplayStringType_AllList; // top of enum
    int num = 0;
    while(1)
    {
        text listName = format_display_string_type((SDK::CrDisplayStringType)i);
        if (0 == listName.compare(TEXT("end"))) break;
        if (0 != listName.compare(TEXT("reserved")))
        {
            tout << "[" << num << "] " << listName << "\n";
            list_type.push_back(i);
            num++;
        }
        i++;
    }
    tout << "[-1] Cancel input\n";
    tout << "input> ";
    text input;
    std::getline(tin, input);
    text_stringstream ss(input);
    int selected_index = 0;
    ss >> selected_index;
    if (selected_index < 0 || num <= selected_index || selected_index == 0 && input != TEXT("0")) {
        tout << "Input cancelled.\n";
        return;
    }
    type = (SDK::CrDisplayStringType)list_type[selected_index];
    SDK::RequestDisplayStringList(m_device_handle, type);
}

void CameraDevice::execute_get_displaystringtypes() {
    CrInt32u num;
    SDK::CrDisplayStringType* Types;
    SDK::CrError err = GetDisplayStringTypes(m_device_handle, &Types, &num);
    if (CR_SUCCEEDED(err)) {
        tout << "Successfully retrieved DisplayStringTypes.\n\n";
        tout << "----- ListType to be retrieved -----\n";
        m_dispStrTypeList.clear();
        for (int i = 0; i < num; i++)
        {
            text listName = format_display_string_type(Types[i]);
            if ((0 != listName.compare(TEXT("end"))) && (0 != listName.compare(TEXT("reserved"))))
            {
                m_dispStrTypeList.push_back(Types[i]);
                tout << listName << "\n";
            }
        }
        tout << "----- ListType to be retrieved -----\n";
        ReleaseDisplayStringTypes(m_device_handle, Types);
    }
    else {
        tout << "Failed to get DisplayStringTypes.\n";
    }
}

void CameraDevice::execute_get_displaystringlist() {
    if (m_dispStrTypeList.size() > 0) {
        SDK::CrDisplayStringListInfo* pProperty = nullptr;
        SDK::CrDisplayStringType type;
        CrInt32u numOfList = 0;
        tout << "Select Display String List Type. \n";
        tout << "[-1] Cancel input\n";
        tout << "[0] "<< format_display_string_type(SDK::CrDisplayStringType_AllList) << "\n";
        for (int i = 0; i < m_dispStrTypeList.size(); i++)
        {
                tout << "[" << i + 1 << "] " << format_display_string_type(m_dispStrTypeList[i]) << "\n";
        }
        tout << "[-1] Cancel input\n";
        tout << "input> ";
        text input;
        std::getline(tin, input);
        text_stringstream ss(input);
        int selected_index = 0;
        ss >> selected_index;
        if (selected_index < 0 || m_dispStrTypeList.size() < selected_index || selected_index == 0 && input != TEXT("0"))
        {
            tout << "Input cancelled.\n";
            return;
        }
        if (selected_index == 0) type = SDK::CrDisplayStringType_AllList;
        else type = m_dispStrTypeList[selected_index-1];
        SDK::CrError err = SDK::GetDisplayStringList(m_device_handle, type, &pProperty, &numOfList);

        if (err == SDK::CrError_Api_NoApplicableInformation) {
            tout << "\nFailed to get DisplayStringList\n";
        }
        if (CR_SUCCEEDED(err)) {
            tout << "\n";
            SDK::CrDisplayStringType titleType = SDK::CrDisplayStringType_AllList; // I never receive ALLList.
            for (int i = 0; i < numOfList; i++)
            {
                text listName = format_display_string_type((SDK::CrDisplayStringType)pProperty[i].listType);
                if ((0 == listName.compare(TEXT("end"))) || (0 == listName.compare(TEXT("reserved")))) continue;
                // Output the name of the new type when the type changes
                if(titleType != pProperty[i].listType)
                {
                    titleType = pProperty[i].listType;
                    tout << "----- " << listName << " -----\n"; // List name (title row)
                }
                tout << pProperty[i].value << ": ";
                tout << format_dispstrlist(pProperty[i]) << "\n";
            }
            ReleaseDisplayStringList(m_device_handle, pProperty);
        }
    }
    else {
        tout << "\nFailed to get DisplayStringList.\n";
    }
}

text CameraDevice::format_dispstrlist(SDK::CrDisplayStringListInfo list) {
    text_stringstream ts;

    CrInt32 length = (list.displayStringSize + 1) * sizeof(CrChar);
    CrChar* listStr = new CrChar[length];
    memset(listStr, 0, length);
    for (int i = 0; i < list.displayStringSize; i++)
    {
        listStr[i] = list.displayString[i];
    }
    ts << listStr;
    return ts.str();
}

void CameraDevice::get_mediaprofile()
{
    if (m_mediaprofileList.size() > 0) m_mediaprofileList.clear();

    get_media_slot_status();
    if (((SDK::CrSlotStatus::CrSlotStatus_OK != m_prop.media_slot1_status.current) && (SDK::CrSlotStatus::CrSlotStatus_OK != m_prop.media_slot2_status.current))
        || ((SDK::CrSlotStatus::CrSlotStatus_OK != m_prop.media_slot1_status.current) && (-1 == m_prop.media_slot2_status.writable))) {
        tout << "\nMedia SLOT1/2 Status is Disable. Can not get media profile.\n";
        return;
    }

    tout << "\nSelect Get Media profile SLOT. \n";
    tout << "[1] SLOT1\n";
    tout << "[2] SLOT2\n";
    tout << "[-1] Cancel input\n";
    tout << "input> ";
    text input;
    std::getline(tin, input);
    text_stringstream ss(input);
    int selected_index = 0;
    ss >> selected_index;
    SCRSDK::CrMediaProfile slot;

    bool slotsts = false;
    load_properties();
    switch (selected_index)
    {
    case 1:
        slot = SCRSDK::CrMediaProfile_Slot1;
        if (SDK::CrSlotStatus::CrSlotStatus_OK != m_prop.media_slot1_status.current) 
            slotsts = true;
        break;
    case 2:
        slot = SCRSDK::CrMediaProfile_Slot2;
        if (-1 == m_prop.media_slot2_status.writable) {
            tout << "Media SLOT2 is not supported.\n";
            selected_index = -1;
        }
        if (SDK::CrSlotStatus::CrSlotStatus_OK != m_prop.media_slot2_status.current)
            slotsts = true;
        break;
    default:
        selected_index = -1;
        break;
    }

    if (selected_index == -1) {
        tout << "Input cancelled.\n";
        return;
    }
    if (slotsts) {
        tout << "Slot Status is not OK.\n";
        return;
    }
    CrInt32u nums = 0;
    SCRSDK::CrMediaProfileInfo* mediaProfileList = nullptr;
    SCRSDK::CrError ret = SCRSDK::GetMediaProfile(m_device_handle, slot, &mediaProfileList, &nums);
    if (CR_SUCCEEDED(ret) && 0 < nums)
    {
        for (int i = 0; i < nums; i++)
        {
            auto pItem = new SCRSDK::CrMediaProfileInfo();
            CrInt32u len = strlen((char*)mediaProfileList[i].contentUrl);
            len = (len + 1) * sizeof(CrChar);
            pItem->contentUrl = new CrInt8u[len];
            MemCpyEx(pItem->contentUrl, mediaProfileList[i].contentUrl, len);
            if (mediaProfileList[i].proxyUrl != NULL)
            {
                len = strlen((char*)mediaProfileList[i].proxyUrl);
                len = (len + 1) * sizeof(CrChar);
                pItem->proxyUrl = new CrInt8u[len];
                MemCpyEx(pItem->proxyUrl, mediaProfileList[i].proxyUrl, len);
            }
            len = strlen((char*)mediaProfileList[i].thumbnailUrl);
            len = (len + 1) * sizeof(CrChar);
            pItem->thumbnailUrl = new CrInt8u[len];
            MemCpyEx(pItem->thumbnailUrl, mediaProfileList[i].thumbnailUrl, len);

            len = strlen((char*)mediaProfileList[i].contentName);
            len = (len + 1) * sizeof(CrChar);
            pItem->contentName = new CrInt8u[len];
            MemCpyEx(pItem->contentName, mediaProfileList[i].contentName, len);
            m_mediaprofileList.push_back(pItem);
        }
        SCRSDK::ReleaseMediaProfile(m_device_handle, mediaProfileList);

        if (m_mediaprofileList.size() > 0)
        {
            for (int i = 0; i < m_mediaprofileList.size(); ++i)
            {

                tout << "\n";
                tout.fill(' ');
                tout.width(3);
                tout <<  i + 1 << ": " << (char*) m_mediaprofileList[i]->contentName << "\n";
                tout << "      Clip URL      : " << (char*) m_mediaprofileList[i]->contentUrl << "\n";
                tout << "      Thumbnail URL : " << (char*) m_mediaprofileList[i]->thumbnailUrl << "\n";
                tout << "      Proxy URL     : ";

                if (m_mediaprofileList[i]->proxyUrl != NULL) {
                    tout << (char*) m_mediaprofileList[i]->proxyUrl << "\n";
                }
                else {
                    tout << "-\n";
                }
            }
        }
    }
    else
    {
        tout << "Media Profile data is empty or GetMediaProfile is not supported.\n";
    }
}

} // namespace cli
