/*
    This is not a final layer. (Some) Additional Requirements:

    COMPLETE
    - Basic loading from FMOD Studio
    - Integration with the FMOD Studio API
    - Play sounds through API calls on Events

    TODO
    - Audio tracks (Music)
    - A call for a looping sound that's terminated on condition

*/


#include "Intrinsics.h"
#include "Shiver_Globals.h"

#if FMOD
#include "../data/deps/FMOD/fmod.h"
#include "../data/deps/FMOD/fmod_common.h"

// FMOD STUDIO
#include "../data/deps/FMOD/fmod_studio.h"
#include "../data/deps/FMOD/fmod_studio_common.h"

internal void
sh_InitializeFMODStudioSubsystem(fmod_sound_subsystem_data *FMODSubsystemData)
{
    FMOD_RESULT Result;
    
    // NOTE(Sleepster): Core System
    Result = FMOD_Studio_System_Create(&FMODSubsystemData->StudioSystem, FMOD_VERSION);
    if(Result != FMOD_OK)
    {
        print_m("Error: %d\n", Result);
        Assert(false, "Failed to create the FMOD studio system!\n");
    }
    // NOTE(Sleepster): We only need to gather the core system if we want to assign changes to it
    Result = FMOD_Studio_System_GetCoreSystem(FMODSubsystemData->StudioSystem, &FMODSubsystemData->CoreSystem);
    if(Result != FMOD_OK)
    {
        print_m("Error: %d\n", Result);
        Assert(false, "Failed to get FMOD Studio's core system!\n");
    }
    Result = FMOD_System_SetSoftwareFormat(FMODSubsystemData->CoreSystem, 48000, FMOD_SPEAKERMODE_STEREO, 0);
    if(Result != FMOD_OK)
    {
        print_m("Error: %d\n", Result);
        Assert(false, "Failed to Set the FMOD System software format!\n");
    }
    
    // NOTE(Sleepster): Studio System
    Result = FMOD_Studio_System_Initialize(FMODSubsystemData->StudioSystem, 512, FMOD_STUDIO_INIT_NORMAL, FMOD_INIT_NORMAL, 0);
    if(Result != FMOD_OK)
    {
        print_m("Error: %d\n", Result);
        Assert(false, "Failed to Intialize the FMOD studio system!\n");
    }
}

internal void
sh_LoadFMODStudioBankData(fmod_sound_subsystem_data *FMODSubsystemData,
                          const char *MainBankFilepath,
                          const char *StringsBankFilepath)
{
    FMOD_RESULT Result;
    
    FMODSubsystemData->MasterBankFilepath = MainBankFilepath;
    FMODSubsystemData->StringsBankFilepath = StringsBankFilepath;
    
    Result = FMOD_Studio_System_LoadBankFile(FMODSubsystemData->StudioSystem, FMODSubsystemData->MasterBankFilepath,
                                             FMOD_STUDIO_LOAD_BANK_NORMAL, &FMODSubsystemData->MasterBank);
    if(Result != FMOD_OK)
    {
        print_m("Error: %d\n", Result);
        Assert(false, "Failed to Load the main bank file!\n");
    }
    
    Result = FMOD_Studio_System_LoadBankFile(FMODSubsystemData->StudioSystem, FMODSubsystemData->StringsBankFilepath,
                                             FMOD_STUDIO_LOAD_BANK_NORMAL, &FMODSubsystemData->MasterStringsBank);
    if(Result != FMOD_OK)
    {
        print_m("Error: %d\n", Result);
        Assert(false, "Failed to Load the strings bank file!\n");
    }
    
    FMODSubsystemData->MasterBankLastWriteTime = Win32GetLastWriteTime(FMODSubsystemData->MasterBankFilepath);
    FMODSubsystemData->StringsBankLastWriteTime = Win32GetLastWriteTime(FMODSubsystemData->StringsBankFilepath);
}

// NOTE(Sleepster): This will load all of the sounds from the banks into the engine
internal inline void
sh_FMODStudioLoadSFXData(fmod_sound_subsystem_data *FMODSubsystemData,
                         fmod_sound_event *SoundEffects)
{
    FMOD_Studio_System_GetEvent(FMODSubsystemData->StudioSystem, "event:/Test", &SoundEffects[TEST_SFX].EventDesc);
    FMOD_Studio_System_GetEvent(FMODSubsystemData->StudioSystem, "event:/Boop", &SoundEffects[TEST_BOOP].EventDesc);
    FMOD_Studio_System_GetEvent(FMODSubsystemData->StudioSystem, "event:/Music", &SoundEffects[TEST_MUSIC].EventDesc);
}

internal void
sh_ReloadFMODStudioBankData(fmod_sound_subsystem_data *FMODSubsystemData,
                            fmod_sound_event *SoundEffects)
{
    // RELOAD THE MASTER BANK DATA
    FMOD_Studio_Bank_Unload(FMODSubsystemData->MasterBank);
    FMOD_Studio_System_LoadBankFile(FMODSubsystemData->StudioSystem, FMODSubsystemData->MasterBankFilepath,
                                    FMOD_STUDIO_LOAD_BANK_NORMAL, &FMODSubsystemData->MasterBank);
    // RELOAD THE STRINGS BANK DATA
    FMOD_Studio_Bank_Unload(FMODSubsystemData->MasterStringsBank);
    FMOD_Studio_System_LoadBankFile(FMODSubsystemData->StudioSystem, FMODSubsystemData->StringsBankFilepath,
                                    FMOD_STUDIO_LOAD_BANK_NORMAL, &FMODSubsystemData->MasterStringsBank);
    
    FMODSubsystemData->MasterBankLastWriteTime = Win32GetLastWriteTime(FMODSubsystemData->MasterBankFilepath);
    FMODSubsystemData->StringsBankLastWriteTime = Win32GetLastWriteTime(FMODSubsystemData->StringsBankFilepath);
    w
        sh_FMODStudioLoadSFXData(FMODSubsystemData, SoundEffects);
}
#endif

// GAME HEADERS
#include "Win32_Shiver.h"
#include "Shiver_AudioEngine.h"

#if 0
// NOTE(Sleepster): Low Level API Stuff
internal void
sh_AudioEngineCallback(ma_device *Device, void *Output, const void *Input, uint32 FrameCount)
{
    ma_decoder *Decoder = (ma_decoder*)Device->pUserData;
    ma_resource_manager *ResourceManager = (ma_resource_manager *)Device->pUserData;
    //Assert(!Decoder, "Decoder not Found");
    Assert(ResourceManager != 0, "ResourceManager not Found");
    
    ma_data_source_read_pcm_frames(ResourceManager, Output, FrameCount, 0);
    
    (void)Input;
}

internal bool32
sh_InitAudioEngine(shiver_audio_engine *sh_AudioEngine)
{
    // NOTE(Sleepster): Playback
    ma_device_info *PlaybackInfo;
    uint32 PlaybackCount;
    
    // NOTE(Sleepster): Capture
    ma_device_info *CaptureInfo;
    uint32 CaptureCount;
    ma_result Result;
    
    Result = ma_context_init(NULL, 0, NULL, &sh_AudioEngine->sh_AudioContext);
    if(Result != MA_SUCCESS)
    {
        print_m("Failed to Initialize the sh_AudioEngine's Context!, Code: %d\n", Result)
            Assert(false, "Failed to Initialize the sh_AudioEngine's Context!\n");
    }
    
    if(ma_context_get_devices(&sh_AudioEngine->sh_AudioContext,
                              &PlaybackInfo, &PlaybackCount,
                              &CaptureInfo, &CaptureCount) != MA_SUCCESS)
    {
        print_m("Failed to Gather the Input/Output Devices!, Code: %d\n", Result);
        Assert(false, "Failed to Gather the Input/Output Devices!\n");
    }
    
    for(uint32 DeviceIndex = 0;
        DeviceIndex < PlaybackCount;
        ++DeviceIndex)
    {
        print_m("%d - %s\n", DeviceIndex, PlaybackInfo[DeviceIndex].name);
    }
    
    sh_AudioEngine->sh_AudioDeviceConfig = ma_device_config_init(ma_device_type_playback);
    sh_AudioEngine->sh_AudioDeviceConfig.playback.format = ma_format_f32;
    sh_AudioEngine->sh_AudioDeviceConfig.playback.channels = CHANNEL_COUNT;
    sh_AudioEngine->sh_AudioDeviceConfig.sampleRate = SAMPLE_COUNT;
    sh_AudioEngine->sh_AudioDeviceConfig.dataCallback = sh_AudioEngineCallback;
    sh_AudioEngine->sh_AudioDeviceConfig.pUserData = &sh_AudioEngine->sh_AudioLoadedSounds[1];
    
    Result = ma_device_init(NULL,
                            &sh_AudioEngine->sh_AudioDeviceConfig,
                            &sh_AudioEngine->sh_AudioOutputDevice);
    if(Result != MA_SUCCESS)
    {
        print_m("Failure to initialize the device!, Code: %d\n", Result);
        Assert(false, "Failure\n");
    }
    
    sh_AudioEngine->sh_AudioManagerConfig = ma_resource_manager_config_init();
    sh_AudioEngine->sh_AudioManagerConfig.decodedFormat = sh_AudioEngine->sh_AudioOutputDevice.playback.format;
    sh_AudioEngine->sh_AudioManagerConfig.decodedChannels = sh_AudioEngine->sh_AudioOutputDevice.playback.channels;
    sh_AudioEngine->sh_AudioManagerConfig.decodedSampleRate = sh_AudioEngine->sh_AudioOutputDevice.sampleRate;
    
    Result = ma_resource_manager_init(&sh_AudioEngine->sh_AudioManagerConfig, &sh_AudioEngine->sh_AudioManager);
    if(Result != MA_SUCCESS)
    {
        print_m("Failure to initialize the Resource Manager!, Code: %d\n", Result);
        Assert(false, "Failure\n");
    }
    
    for(int32 SoundIndex = 0;
        SoundIndex < 1;
        ++SoundIndex)
    {
        Result = ma_resource_manager_data_source_init(&sh_AudioEngine->sh_AudioManager,
                                                      "res/sounds/Test.mp3",
                                                      MA_RESOURCE_MANAGER_DATA_SOURCE_FLAG_DECODE|
                                                      MA_RESOURCE_MANAGER_DATA_SOURCE_FLAG_ASYNC |
                                                      MA_RESOURCE_MANAGER_DATA_SOURCE_FLAG_STREAM,
                                                      0,
                                                      &sh_AudioEngine->sh_AudioLoadedSounds[1]);
        if(Result != MA_SUCCESS)
        {
            print_m("Failure to initialize the Sound!, Index: %d, Code: %d\n", sh_AudioEngine->CurrentSoundIndex, Result);
            Assert(false, "Failure\n");
        }
        // TODO(Sleepster): Add a struct that will define exactly what it is that a soundfx has. Such as whether it is looping.
        ma_data_source_set_looping(&sh_AudioEngine->sh_AudioLoadedSounds[1], MA_TRUE);
    }
    
    ma_device_set_master_volume(&sh_AudioEngine->sh_AudioOutputDevice, 0.01f);
    
    Result = ma_device_start(&sh_AudioEngine->sh_AudioOutputDevice);
    if(Result != MA_SUCCESS)
    {
        print_m("Failure to start the device!, Code: %d\n", Result);
        Assert(false, "Failure\n");
    }
    return(1);
}
#endif

// NOTE(Sleepster): High Level Engine
internal void
sh_InitializeAudioEngine(shiver_audio_engine *AudioEngine)
{
    ma_result Result;
    
    Result = ma_context_init(0, 0, 0, &AudioEngine->AudioContext);
    if(Result != MA_SUCCESS)
    {
        print_m("Failure to initialize the context!, Code: %d\n", Result);
        Assert(false, "Failure\n");
    }
    
    AudioEngine->OutputDeviceConfig = ma_device_config_init(ma_device_type_playback);
    AudioEngine->OutputDeviceConfig.playback.format = ma_format_f32;
    AudioEngine->OutputDeviceConfig.playback.channels = CHANNEL_COUNT;
    AudioEngine->OutputDeviceConfig.sampleRate = SAMPLE_RATE;
    AudioEngine->OutputDeviceConfig.pUserData = &AudioEngine->Engine;
    
    Result = ma_device_init(&AudioEngine->AudioContext, &AudioEngine->OutputDeviceConfig, &AudioEngine->OutputDevice);
    if(Result != MA_SUCCESS)
    {
        print_m("Failure to initialize the device!, Code: %d\n", Result);
        Assert(false, "Failure\n");
    }
    
    AudioEngine->EngineConfig = ma_engine_config_init();
    AudioEngine->EngineConfig.channels = CHANNEL_COUNT;
    AudioEngine->EngineConfig.sampleRate = SAMPLE_RATE;
    AudioEngine->EngineConfig.noAutoStart = MA_FALSE;
    
    Result = ma_engine_init(&AudioEngine->EngineConfig, &AudioEngine->Engine);
    if(Result != MA_SUCCESS)
    {
        print_m("Failure to initialize the engine!, Code: %d\n", Result);
        Assert(false, "Failure\n");
    }
    
    ma_context_uninit(&AudioEngine->AudioContext);
    ma_device_start(&AudioEngine->OutputDevice);
    
    ma_engine_set_volume(&AudioEngine->Engine, 0.05f);
}

internal void
sh_ResetAudioEngine(shiver_audio_engine *AudioEngine)
{
    for(uint32 AudioIdx = 0;
        AudioIdx < AudioEngine->CurrentTrackIdx;
        ++AudioIdx)
    {
        ma_sound_stop(&AudioEngine->BackgroundTracks[AudioIdx]);
        ma_sound_uninit(&AudioEngine->BackgroundTracks[AudioIdx]);
    }
    ma_engine_uninit(&AudioEngine->Engine);
    ma_device_uninit(&AudioEngine->OutputDevice);
}

internal void
sh_RestartAudioEngine(shiver_audio_engine *AudioEngine)
{
    AudioEngine->Initialized = false;
    
    while(!AudioEngine->Initialized)
    {
        sh_ResetAudioEngine(AudioEngine);
        sh_InitializeAudioEngine(AudioEngine);
        AudioEngine->Initialized = true;
    }
}