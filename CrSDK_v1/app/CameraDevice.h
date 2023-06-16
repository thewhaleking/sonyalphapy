#ifndef CAMERADEVICE_H
#define CAMERADEVICE_H

#if defined(__linux__)
// Use errno_t if available, define it otherwise
#ifndef __STDC_LIB_EXT1__
typedef int errno_t;
#else
#define  __STDC_WANT_LIB_EXT1__ 1
#endif

// End Linux definitions
#endif

#include <atomic>
#include <cstdint>
#include "CRSDK/CameraRemote_SDK.h"
#include "CRSDK/IDeviceCallback.h"
#include "ConnectionInfo.h"
#include "PropertyValueTable.h"
#include "Text.h"
#include "MessageDefine.h"

namespace cli
{

class CRFolderInfos
{
public:
    CRFolderInfos(SCRSDK::CrMtpFolderInfo* info, int32_t nums)
        : pFolder(info)
        , numOfContents(nums)
    {};
    ~CRFolderInfos()
    {
        delete[] pFolder->folderName;
        pFolder->folderName = NULL;
        delete pFolder;
        pFolder = NULL;
    };
public:
    SCRSDK::CrMtpFolderInfo* pFolder;
    int32_t numOfContents;
};

typedef std::vector<CRFolderInfos*> MtpFolderList;
typedef std::vector<SCRSDK::CrMtpContentsInfo*> MtpContentsList;
typedef std::vector<SCRSDK::CrMediaProfileInfo*> MediaProfileList;

class CameraDevice : public SCRSDK::IDeviceCallback
{
public:
    CameraDevice() = delete;
    CameraDevice(std::int32_t no, SCRSDK::ICrCameraObjectInfo const* camera_info);
    ~CameraDevice();

    // Get fingerprint
    bool getfingerprint();

    // Try to connect to the device
    bool connect(SCRSDK::CrSdkControlMode openMode, SCRSDK::CrReconnectingSet reconnect);

    // Disconnect from the device
    bool disconnect();

    // Release from the device
    bool release();

    /*** Shooting operations ***/

    void capture_image() const;
    void s1_shooting() const;
    void af_shutter() const;
    void continuous_shooting();

    /*** Property operations ***/
    // Should be const functions, but requires load property, which is not

    void get_aperture();
    void get_iso();
    void get_shutter_speed();
    void get_position_key_setting();
    void get_exposure_program_mode();
    void get_still_capture_mode();
    void get_focus_mode();
    void get_focus_area();
    void get_live_view();
    void get_live_view_image_quality();
    void get_af_area_position();
    void get_select_media_format();
    void get_white_balance();
    bool get_custom_wb();
    void get_zoom_operation();
    void get_remocon_zoom_speed_type();
    bool get_aps_c_or_full_switching_setting();
    bool get_camera_setting_saveread_state();
    bool get_playback_media();
    bool get_gain_base_sensitivity();
    bool get_gain_base_iso_sensitivity();
    bool get_monitor_lut_setting();
    bool get_exposure_index();
    bool get_baselook_value();
    bool get_iris_mode_setting();
    bool get_shutter_mode_setting();
    void get_iso_current_sensitivity();
    bool get_gain_control_setting();
    bool get_exposure_control_type();
    bool get_recording_setting();
    bool get_gain_db_value();
    bool get_shutter_speed_value();
    bool get_white_balance_tint();
    void get_media_slot_status();
    bool get_movie_rec_button_toggle_enable_status();
    bool get_focus_bracket_shot_num();
    bool get_focus_bracket_focus_range();
    bool get_image_stabilization_steady_shot();
    bool get_movie_image_stabilization_steady_shot();
    bool get_silent_mode();
    bool get_silent_mode_aperture_drive_in_af();
    bool get_silent_mode_shutter_when_power_off();
    bool get_silent_mode_auto_pixel_mapping();
    bool get_shutter_type();

    void set_aperture();
    void set_iso();
    bool set_save_info() const;
    void set_shutter_speed();
    void set_position_key_setting();
    void set_exposure_program_mode();
    void set_still_capture_mode();
    void set_focus_mode();
    void set_focus_area();
    void set_live_view_image_quality();
    void set_af_area_position();
    void set_white_balance();
    void set_custom_wb();
    void set_zoom_operation();
    void set_remocon_zoom_speed_type();
    bool set_drive_mode(CrInt64u value);
    void execute_camera_setting_reset();
    void set_playback_media();

    void set_gain_base_iso_sensitivity();
    void set_gain_base_sensitivity();
    void set_monitor_lut_setting();
    void set_exposure_index();
    void set_baselook_value();
    void set_iris_mode_setting();
    void set_shutter_mode_setting();
    void set_gain_control_setting();
    void set_exposure_control_type();
    void set_recording_setting();
    void set_dispmode();
    void set_gain_db_value();
    void set_white_balance_tint();
    void set_shutter_speed_value();
    void set_focus_bracket_focus_range();
    void set_focus_bracket_shot_num();
    void set_movie_image_stabilization_steady_shot();
    void set_image_stabilization_steady_shot();
    void set_silent_mode();
    void set_silent_mode_aperture_drive_in_af();
    void set_silent_mode_shutter_when_power_off();
    void set_silent_mode_auto_pixel_mapping();
    void set_shutter_type();

    void execute_lock_property(CrInt16u code);
    void set_select_media_format();
    void execute_movie_rec();
    void execute_downup_property(CrInt16u code);
    void execute_pos_xy(CrInt16u code);
    void change_live_view_enable();
    bool is_live_view_enable() { return m_lvEnbSet; };
    void execute_preset_focus();
    void execute_APS_C_or_Full();
    void execute_movie_rec_toggle();
    void do_download_camera_setting_file();
    void do_upload_camera_setting_file();

    void execute_request_displaystringlist();
    void execute_get_displaystringtypes();
    void execute_get_displaystringlist();
    void execute_focus_bracket();

    void get_mediaprofile();

    bool get_movie_shooting_mode();
    void set_movie_shooting_mode();

    std::int32_t get_number() { return m_number; }
    text get_model() { return text(m_info->GetModel()); }
    text get_id();

    // Check if this device is connected
    bool is_connected() const;
    std::uint32_t ip_address() const;
    text ip_address_fmt() const;
    text mac_address() const;
    std::int16_t pid() const;

    void getContentsList();
    void getFileNames(std::vector<text> &file_names);
    void pullContents(SCRSDK::CrContentHandle content);
    void getScreennail(SCRSDK::CrContentHandle content);
    void getThumbnail(SCRSDK::CrContentHandle content);

    SCRSDK::CrSdkControlMode get_sdkmode();

    CrInt32u get_sshsupport();
    bool is_getfingerprint() { return !m_fingerprint.empty(); };
    bool is_setpassword() { return !m_userPassword.empty(); };

public:
    // Inherited via IDeviceCallback
    virtual void OnConnected(SCRSDK::DeviceConnectionVersioin version) override;
    virtual void OnDisconnected(CrInt32u error) override;
    virtual void OnPropertyChanged() override;
    virtual void OnLvPropertyChanged() override;
    virtual void OnCompleteDownload(CrChar* filename, CrInt32u type) override;
    virtual void OnWarning(CrInt32u warning) override;
    virtual void OnError(CrInt32u error) override;
    virtual void OnPropertyChangedCodes(CrInt32u num, CrInt32u* codes) override;
    virtual void OnLvPropertyChangedCodes(CrInt32u num, CrInt32u* codes) override;
    virtual void OnNotifyContentsTransfer(CrInt32u notify, SCRSDK::CrContentHandle contentHandle, CrChar* filename) override;

private:
    void load_properties(CrInt32u num = 0, CrInt32u* codes = nullptr);
    void get_property(SCRSDK::CrDeviceProperty& prop) const;
    bool set_property(SCRSDK::CrDeviceProperty& prop) const;
    text format_dispstrlist(SCRSDK::CrDisplayStringListInfo list);
    text format_display_string_type(SCRSDK::CrDisplayStringType type);

private:
    std::int32_t m_number;
    SCRSDK::ICrCameraObjectInfo* m_info;
    std::int64_t m_device_handle;
    std::atomic<bool> m_connected;
    ConnectionType m_conn_type;
    NetworkInfo m_net_info;
    UsbInfo m_usb_info;
    PropertyValueTable m_prop;
    bool m_lvEnbSet;
    SCRSDK::CrSdkControlMode m_modeSDK;
    MtpFolderList   m_foldList;
    MtpContentsList m_contentList;
    bool m_spontaneous_disconnection;
    // DispStrList
    std::vector<SCRSDK::CrDisplayStringType> m_dispStrTypeList; // Information returned as a result of GetDisplayStringTypes
    MediaProfileList m_mediaprofileList;
    std::string m_fingerprint;
    std::string m_userPassword;
};
} // namespace cli


inline errno_t MemCpyEx(void* result, const void* source, size_t type_size)
{
#if (defined(_WIN32) || defined(_WIN64))
    return memcpy_s(result, type_size, source, type_size);
#else
    std::memcpy(result, source, type_size);
    return 0;
#endif
}

#define MAC_MAX_PATH 255

#endif // !CAMERADEVICE_H
