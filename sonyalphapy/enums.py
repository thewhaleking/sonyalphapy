"""Python IntEnum mirrors of Sony CrSDK C++ enums."""

from enum import IntEnum, IntFlag


# ---------------------------------------------------------------------------
# CrCommandData.h
# ---------------------------------------------------------------------------


class CommandId(IntEnum):
    Release = 0
    MovieRecord = 1
    CancelShooting = 2
    MediaFormat = 4
    MediaQuickFormat = 5
    CancelMediaFormat = 6
    S1andRelease = 7
    CancelContentsTransfer = 8
    CameraSettingsReset = 9
    APS_C_or_Full_Switching = 10
    MovieRecButtonToggle = 11
    CancelRemoteTouchOperation = 12
    PixelMapping = 13
    TimeCodePresetReset = 14
    UserBitPresetReset = 15
    SensorCleaning = 16
    PictureProfileReset = 17
    CreativeLookReset = 18
    PowerOff = 19


class CommandParam(IntEnum):
    Up = 0x0000
    Down = 0x0001


# ---------------------------------------------------------------------------
# CrDefines.h
# ---------------------------------------------------------------------------


class DataType(IntEnum):
    Undefined = 0x0000
    UInt8 = 0x0001
    UInt16 = 0x0002
    UInt32 = 0x0003
    UInt64 = 0x0004
    UInt128 = 0x0005
    Int8 = 0x1001
    Int16 = 0x1002
    Int32 = 0x1003
    Int64 = 0x1004
    Int128 = 0x1005
    UInt8Array = 0x2001
    UInt16Array = 0x2002
    UInt32Array = 0x2003
    UInt64Array = 0x2004
    UInt128Array = 0x2005
    Int8Array = 0x3001
    Int16Array = 0x3002
    Int32Array = 0x3003
    Int64Array = 0x3004
    Int128Array = 0x3005
    UInt8Range = 0x4001
    UInt16Range = 0x4002
    UInt32Range = 0x4003
    UInt64Range = 0x4004
    UInt128Range = 0x4005
    Int8Range = 0x5001
    Int16Range = 0x5002
    Int32Range = 0x5003
    Int64Range = 0x5004
    Int128Range = 0x5005
    STR = 0xFFFF


class SdkControlMode(IntEnum):
    Remote = 0x00000000
    ContentsTransfer = 0x00000001


class ReconnectingSet(IntEnum):
    Off = 0x00000000
    On = 0x00000001


class MediaProfile(IntEnum):
    Slot1 = 0x00000001
    Slot2 = 0x00000002


class SSHSupport(IntEnum):
    Off = 0x00000000
    On = 0x00000001


class CameraDeviceModel(IntEnum):
    ILCE_7RM4 = 0
    ILCE_9M2 = 1
    ILCE_7C = 2
    ILCE_7SM3 = 3
    ILCE_1 = 4
    ILCE_7RM4A = 5
    DSC_RX0M2 = 6
    ILCE_7M4 = 7
    ILME_FX3 = 8
    ILME_FX30 = 9
    ILME_FX6 = 10
    ILCE_7RM5 = 11
    ZV_E1 = 12


# ---------------------------------------------------------------------------
# CrDeviceProperty.h — property codes
# ---------------------------------------------------------------------------


class PropertyCode(IntEnum):
    Undefined = 0

    S1 = 1
    AEL = 2
    FEL = 3
    AFL = 4
    AWBL = 5

    FNumber = 0x0100
    ExposureBiasCompensation = 0x0101
    FlashCompensation = 0x0102
    ShutterSpeed = 0x0103
    IsoSensitivity = 0x0104
    ExposureProgramMode = 0x0105
    FileType = 0x0106
    JpegQuality = 0x0107
    WhiteBalance = 0x0108
    FocusMode = 0x0109
    MeteringMode = 0x010A
    FlashMode = 0x010B
    WirelessFlash = 0x010C
    RedEyeReduction = 0x010D
    DriveMode = 0x010E
    DRO = 0x010F
    ImageSize = 0x0110
    AspectRatio = 0x0111
    PictureEffect = 0x0112
    FocusArea = 0x0113
    Colortemp = 0x0115
    ColorTuningAB = 0x0116
    ColorTuningGM = 0x0117
    LiveViewDisplayEffect = 0x0118
    StillImageStoreDestination = 0x0119
    PriorityKeySettings = 0x011A
    Focus_Magnifier_Setting = 0x011E
    DateTime_Settings = 0x011F
    NearFar = 0x0120
    AF_Area_Position = 0x0122
    Zoom_Scale = 0x0125
    Zoom_Setting = 0x0126
    Zoom_Operation = 0x0127
    Movie_File_Format = 0x0128
    Movie_Recording_Setting = 0x0129
    Movie_Recording_FrameRateSetting = 0x012A
    CompressionFileFormatStill = 0x012B
    MediaSLOT1_FileType = 0x012C
    MediaSLOT2_FileType = 0x012D
    MediaSLOT1_JpegQuality = 0x012E
    MediaSLOT2_JpegQuality = 0x012F
    MediaSLOT1_ImageSize = 0x0130
    MediaSLOT2_ImageSize = 0x0131
    RAW_FileCompressionType = 0x0132
    MediaSLOT1_RAW_FileCompressionType = 0x0133
    MediaSLOT2_RAW_FileCompressionType = 0x0134
    ZoomAndFocusPosition_Save = 0x0135
    ZoomAndFocusPosition_Load = 0x0136
    IrisModeSetting = 0x0137
    ShutterModeSetting = 0x0138
    GainControlSetting = 0x0139
    GainBaseIsoSensitivity = 0x013A
    GainBaseSensitivity = 0x013B
    ExposureIndex = 0x013C
    BaseLookValue = 0x013D
    PlaybackMedia = 0x013E
    DispModeSetting = 0x013F
    DispMode = 0x0140
    TouchOperation = 0x0141
    SelectFinder = 0x0142
    AutoPowerOffTemperature = 0x0143
    BodyKeyLock = 0x0144
    ImageID_Num_Setting = 0x0145
    ImageID_Num = 0x0146
    ImageID_String = 0x0147
    ExposureCtrlType = 0x0148
    MonitorLUTSetting = 0x0149
    FocalDistanceInMeter = 0x014A
    FocalDistanceInFeet = 0x014B
    FocalDistanceUnitSetting = 0x014C
    DigitalZoomScale = 0x014D
    ZoomDistance = 0x014E
    ZoomDistanceUnitSetting = 0x014F
    ShutterModeStatus = 0x0150
    ShutterSlow = 0x0151
    ShutterSlowFrames = 0x0152
    Movie_Recording_ResolutionForMain = 0x0153
    Movie_Recording_ResolutionForProxy = 0x0154
    Movie_Recording_FrameRateProxySetting = 0x0155
    BatteryRemainDisplayUnit = 0x0156
    PowerSource = 0x0157
    MovieShootingMode = 0x0158
    MovieShootingModeColorGamut = 0x0159
    MovieShootingModeTargetDisplay = 0x015A
    DepthOfFieldAdjustmentMode = 0x015B
    ColortempStep = 0x015C
    WhiteBalanceModeSetting = 0x015D
    WhiteBalanceTint = 0x015E
    WhiteBalanceTintStep = 0x015F
    Focus_Operation = 0x0160
    ShutterECSSetting = 0x0161
    ShutterECSNumber = 0x0162
    ShutterECSNumberStep = 0x0163
    ShutterECSFrequency = 0x0164
    RecorderControlProxySetting = 0x0165
    ButtonAssignmentAssignable1 = 0x0166
    ButtonAssignmentAssignable2 = 0x0167
    ButtonAssignmentAssignable3 = 0x0168
    ButtonAssignmentAssignable4 = 0x0169
    ButtonAssignmentAssignable5 = 0x016A
    ButtonAssignmentAssignable6 = 0x016B
    ButtonAssignmentAssignable7 = 0x016C
    ButtonAssignmentAssignable8 = 0x016D
    ButtonAssignmentAssignable9 = 0x016E
    ButtonAssignmentLensAssignable1 = 0x016F
    AssignableButton1 = 0x0170
    AssignableButton2 = 0x0171
    AssignableButton3 = 0x0172
    AssignableButton4 = 0x0173
    AssignableButton5 = 0x0174
    AssignableButton6 = 0x0175
    AssignableButton7 = 0x0176
    AssignableButton8 = 0x0177
    AssignableButton9 = 0x0178
    LensAssignableButton1 = 0x0179
    FocusModeSetting = 0x017A
    ShutterAngle = 0x017B
    ShutterSetting = 0x017C
    ShutterMode = 0x017D
    ShutterSpeedValue = 0x017E
    NDFilter = 0x017F
    NDFilterModeSetting = 0x0180
    NDFilterValue = 0x0181
    GainUnitSetting = 0x0182
    GaindBValue = 0x0183
    AWB = 0x0184
    SceneFileIndex = 0x0185
    MoviePlayButton = 0x0186
    MoviePlayPauseButton = 0x0187
    MoviePlayStopButton = 0x0188
    MovieForwardButton = 0x0189
    MovieRewindButton = 0x018A
    MovieNextButton = 0x018B
    MoviePrevButton = 0x018C
    MovieRecReviewButton = 0x018D
    FaceEyeDetectionAF = 0x018E
    AFTransitionSpeed = 0x018F
    AFSubjShiftSens = 0x0190
    AFAssist = 0x0191
    NDPresetOrVariableSwitchingSetting = 0x0192
    FunctionOfRemoteTouchOperation = 0x0193
    RemoteTouchOperation = 0x0194
    FollowFocusPositionSetting = 0x0195
    FocusBracketShotNumber = 0x0196
    FocusBracketFocusRange = 0x0197
    ExtendedInterfaceMode = 0x0198
    SQRecordingFrameRateSetting = 0x0199
    SQFrameRate = 0x019A
    SQRecordingSetting = 0x019B
    AudioRecording = 0x019C
    AudioInputMasterLevel = 0x019D
    TimeCodePreset = 0x019E
    TimeCodeFormat = 0x019F
    TimeCodeRun = 0x01A0
    TimeCodeMake = 0x01A1
    UserBitPreset = 0x01A2
    UserBitTimeRec = 0x01A3
    ImageStabilizationSteadyShot = 0x01A4
    Movie_ImageStabilizationSteadyShot = 0x01A5
    SilentMode = 0x01A6
    SilentModeApertureDriveInAF = 0x01A7
    SilentModeShutterWhenPowerOff = 0x01A8
    SilentModeAutoPixelMapping = 0x01A9
    ShutterType = 0x01AA
    PictureProfile = 0x01AB
    CreativeLook = 0x01C0
    Movie_ProxyFileFormat = 0x01CA
    ProxyRecordingSetting = 0x01CB
    FunctionOfTouchOperation = 0x01CC

    S2 = 0x0500
    Interval_Rec_Mode = 0x0505
    Still_Image_Trans_Size = 0x0506
    RAW_J_PC_Save_Image = 0x0507
    LiveView_Image_Quality = 0x0508
    CustomWB_Capture_Standby = 0x0509
    CustomWB_Capture_Standby_Cancel = 0x050A
    CustomWB_Capture = 0x050B
    Remocon_Zoom_Speed_Type = 0x050C

    # Read-only properties
    GetOnly = 0x0700
    SnapshotInfo = 0x0701
    BatteryRemain = 0x0702
    BatteryLevel = 0x0703
    EstimatePictureSize = 0x0704
    RecordingState = 0x0705
    LiveViewStatus = 0x0706
    FocusIndication = 0x0707
    MediaSLOT1_Status = 0x0708
    MediaSLOT1_RemainingNumber = 0x0709
    MediaSLOT1_RemainingTime = 0x070A
    MediaSLOT1_FormatEnableStatus = 0x070B
    MediaSLOT2_Status = 0x070D
    MediaSLOT2_FormatEnableStatus = 0x070E
    MediaSLOT2_RemainingNumber = 0x070F
    MediaSLOT2_RemainingTime = 0x0710
    Media_FormatProgressRate = 0x0712
    LiveView_Area = 0x0716
    Interval_Rec_Status = 0x0719
    CustomWB_Execution_State = 0x071A
    CustomWB_Capturable_Area = 0x071B
    CustomWB_Capture_Frame_Size = 0x071C
    CustomWB_Capture_Operation = 0x071D
    Zoom_Operation_Status = 0x071F
    Zoom_Bar_Information = 0x0720
    Zoom_Type_Status = 0x0721
    MediaSLOT1_QuickFormatEnableStatus = 0x0722
    MediaSLOT2_QuickFormatEnableStatus = 0x0723
    Cancel_Media_FormatEnableStatus = 0x0724
    Zoom_Speed_Range = 0x0725
    SdkControlMode = 0x0726
    ContentsTransferStatus = 0x0727
    ContentsTransferCancelEnableStatus = 0x0728
    ContentsTransferProgress = 0x0729
    IsoCurrentSensitivity = 0x072A
    CameraSetting_SaveOperationEnableStatus = 0x072B
    CameraSetting_ReadOperationEnableStatus = 0x072C
    CameraSetting_SaveRead_State = 0x072D
    CameraSettingsResetEnableStatus = 0x072E
    APS_C_or_Full_SwitchingSetting = 0x072F
    APS_C_or_Full_SwitchingEnableStatus = 0x0730
    DispModeCandidate = 0x0731
    ShutterSpeedCurrentValue = 0x0732
    Focus_Speed_Range = 0x0733
    NDFilterMode = 0x0734
    MoviePlayingSpeed = 0x0735
    BatteryRemainingInMinutes = 0x073B
    BatteryRemainingInVoltage = 0x073C
    DCVoltage = 0x073D
    MoviePlayingState = 0x073E
    DeviceOverheatingState = 0x0758


# ---------------------------------------------------------------------------
# CrDeviceProperty.h — value enums for common properties
# ---------------------------------------------------------------------------


class FocusMode(IntEnum):
    MF = 0x0001
    AF_S = 0x0002
    AF_C = 0x0003
    AF_A = 0x0004
    AF_D = 0x0005


class ExposureProgram(IntEnum):
    M_Manual = 0x00000001
    P_Auto = 0x00000002
    A_AperturePriority = 0x00000003
    S_ShutterSpeedPriority = 0x00000004
    Auto = 0x00008000
    Auto_Plus = 0x00008001
    Movie_P = 0x00008050
    Movie_A = 0x00008051
    Movie_S = 0x00008052
    Movie_M = 0x00008053
    Movie_Auto = 0x00008054
    Movie_F = 0x00008055


class FileType(IntEnum):
    None_ = 0x0000
    Jpeg = 0x0001
    Raw = 0x0002
    RawJpeg = 0x0003
    RawHeif = 0x0004
    Heif = 0x0005


class JpegQuality(IntEnum):
    Unknown = 0x0000
    Light = 0x0001
    Standard = 0x0002
    Fine = 0x0003
    ExFine = 0x0004


class WhiteBalance(IntEnum):
    AWB = 0x0000
    Underwater_Auto = 0x0001
    Daylight = 0x0011
    Shadow = 0x0012
    Cloudy = 0x0013
    Tungsten = 0x0014
    Fluorescent = 0x0020
    Fluorescent_WarmWhite = 0x0021
    Fluorescent_CoolWhite = 0x0022
    Fluorescent_DayWhite = 0x0023
    Fluorescent_Daylight = 0x0024
    Flash = 0x0030
    ColorTemp = 0x0100
    Custom_1 = 0x0101
    Custom_2 = 0x0102
    Custom_3 = 0x0103
    Custom = 0x0104


class LockIndicator(IntEnum):
    Unknown = 0x0000
    Unlocked = 0x0001
    Locked = 0x0002


class ISOMode(IntEnum):
    Normal = 0x00
    MultiFrameNR = 0x01
    MultiFrameNR_High = 0x02
    Ext = 0x10
    AUTO = 0xFFFFFF


# ---------------------------------------------------------------------------
# CrError.h — error / warning codes
# ---------------------------------------------------------------------------


class CrError(IntEnum):
    None_ = 0x0000
    Generic = 0x8000
    File = 0x8100
    Connect = 0x8200
    Memory = 0x8300
    Api = 0x8400
    Init = 0x8500
    Polling = 0x8600
    Adaptor = 0x8700
    Device = 0x8800
    Contents = 0x8C00

    Generic_Unknown = 0x8000
    Generic_Notimpl = 0x8001
    Generic_Abort = 0x8002
    Generic_NotSupported = 0x8003
    Generic_SeriousErrorNotSupported = 0x8004
    Generic_InvalidHandle = 0x8005
    Generic_InvalidParameter = 0x8006

    File_Unknown = 0x8100
    File_IllegalOperation = 0x8101
    File_IllegalParameter = 0x8102
    File_EOF = 0x8103
    File_OutOfRange = 0x8104
    File_NotFound = 0x8105
    File_DirNotFound = 0x8106
    File_AlreadyOpened = 0x8107
    File_PermissionDenied = 0x8108
    File_StorageFull = 0x8109
    File_AlreadyExists = 0x810A
    File_TooManyOpenedFiles = 0x810B
    File_ReadOnly = 0x810C
    File_CantOpen = 0x810D
    File_CantClose = 0x810E
    File_CantDelete = 0x810F
    File_CantRead = 0x8110
    File_CantWrite = 0x8111
    File_CantCreateDir = 0x8112

    Connect_Unknown = 0x8200
    Connect_Connect = 0x8201
    Connect_Release = 0x8203
    Connect_GetProperty = 0x8204
    Connect_SendCommand = 0x8205
    Connect_HandlePlugin = 0x8206
    Connect_Disconnected = 0x8207
    Connect_TimeOut = 0x8208
    Reconnect_TimeOut = 0x8209
    Connect_FailRejected = 0x820A
    Connect_FailBusy = 0x820B
    Connect_FailUnspecified = 0x820C
    Connect_Cancel = 0x820D
    Connect_SessionAlreadyOpened = 0x8210
    Connect_ContentsTransfer_NotSupported = 0x8211
    Connect_SSH_NotSupported = 0x8212
    Connect_SSH_InvalidParameter = 0x8213
    Connect_SSH_ServerConnectFailed = 0x8214
    Connect_SSH_ServerAuthenticationFailed = 0x8215
    Connect_SSH_UserAuthenticationFailed = 0x8216
    Connect_SSH_PortForwardFailed = 0x8217
    Connect_SSH_GetFingerprintFailed = 0x8218

    Memory_Unknown = 0x8300
    Memory_OutOfMemory = 0x8301
    Memory_InvalidPointer = 0x8302
    Memory_Insufficient = 0x8303

    Api_Unknown = 0x8400
    Api_Insufficient = 0x8401
    Api_InvalidCalled = 0x8402
    Api_NoApplicableInformation = 0x8403
    Api_OutOfModelList = 0x8404
    Api_NotSupportModelOfUSB = 0x8405
    Api_NotSupportModelOfEthernet = 0x8406
    Api_InvalidSerialNumber = 0x8407
    Api_InvalidIpAddress = 0x8408
    Api_InvalidMacAddress = 0x8409

    Device_Unknown = 0x8800

    Contents_Unknown = 0x8C00
    Contents_InvalidHandle = 0x8C02
    Contents_Transfer_Unsuccess = 0x8C05
    Contents_Transfer_Cancel = 0x8C06
    Contents_RejectRequest = 0x8C07

    Warning_Unknown = 0x00020000
    Warning_Connect_Reconnected = 0x00020001
    Warning_Connect_Reconnecting = 0x00020002
    Warning_File_StorageFull = 0x00020003
    Warning_NetworkErrorOccurred = 0x00020008
    Warning_NetworkErrorRecovered = 0x00020009
    Warning_Format_Complete = 0x0002000B
    Notify_Captured_Event = 0x00020010
    Warning_DateTime_Setting_Result_OK = 0x00020012
    Warning_All_Download_Complete = 0x00020017
    Warning_Connect_Already = 0x00020018
    Warning_LensInformationChanged = 0x0002002D
