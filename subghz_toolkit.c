// RocketGod's SubGHz Toolkit
// Use Flipper Zero to Reverse Engineer itself (helpful for closed source firmware)
// Decrypt Keeloq Manufacturer Codes
// Analyze Protocols
// Save all data to files on sdcard

// https://betaskynet.com
// https://discord.gg/thepirates

#include <furi.h>
#include <furi_hal.h>
#include <gui/gui.h>
#include <gui/view_dispatcher.h>
#include <gui/modules/submenu.h>
#include <gui/modules/popup.h>
#include <gui/modules/text_box.h>
#include <gui/modules/loading.h>
#include <storage/storage.h>
#include <lib/toolbox/stream/file_stream.h>
#include <notification/notification_messages.h>

#include <lib/subghz/subghz_keystore.h>
#include <lib/subghz/receiver.h>
#include <lib/subghz/transmitter.h>
#include <lib/subghz/subghz_file_encoder_worker.h>
#include <lib/subghz/protocols/base.h>
#include <lib/subghz/environment.h>
#include <lib/subghz/subghz_setting.h>
#include <lib/subghz/registry.h>

#define TAG "SubGhzToolkit"
#define SUBGHZ_TOOLKIT_VERSION "1.0"
#define TEXT_BUFFER_SIZE 32768
#define SUBGHZ_ANALYSIS_DIR EXT_PATH("subghz/analysis")

extern const SubGhzProtocolRegistry subghz_protocol_registry;

typedef struct
{
    ViewDispatcher *view_dispatcher;
    Gui *gui;
    NotificationApp *notifications;
    Submenu *submenu;
    Popup *popup;
    TextBox *text_box;
    Loading *loading;
    FuriString *text_buffer;
    SubGhzEnvironment *environment;
    SubGhzReceiver *receiver;
    SubGhzSetting *setting;
    const SubGhzProtocolRegistry *protocol_registry;
} SubGhzToolkitApp;

typedef enum
{
    SubGhzToolkitViewSubmenu,
    SubGhzToolkitViewPopup,
    SubGhzToolkitViewTextBox,
    SubGhzToolkitViewLoading,
} SubGhzToolkitView;

typedef enum
{
    SubGhzToolkitSubmenuIndexDecryptKeeloq,
    SubGhzToolkitSubmenuIndexListProtocols,
    SubGhzToolkitSubmenuIndexExportProtocolInfo,
    SubGhzToolkitSubmenuIndexAdvancedAnalysis,
    SubGhzToolkitSubmenuIndexProtocolDetails = 100,
    SubGhzToolkitSubmenuIndexFunctionDisassembly,
    SubGhzToolkitSubmenuIndexProtocolStateAnalysis,
    SubGhzToolkitSubmenuIndexSignalCapture,
    SubGhzToolkitSubmenuIndexTimingAnalysis,
    SubGhzToolkitSubmenuIndexCHeaderGeneration,
    SubGhzToolkitSubmenuIndexAbout,
} SubGhzToolkitSubmenuIndex;

static bool subghz_toolkit_export_keeloq_keys(SubGhzToolkitApp *app);
static void subghz_toolkit_show_protocols_list(SubGhzToolkitApp *app);
static void subghz_toolkit_extract_protocol_details(SubGhzToolkitApp *app, const char *protocol_name);
static void subghz_toolkit_export_all_protocol_info(SubGhzToolkitApp *app);
static void subghz_toolkit_advanced_analysis(SubGhzToolkitApp *app);
static void subghz_toolkit_show_about(SubGhzToolkitApp *app);

static void subghz_toolkit_deep_protocol_analysis(Stream *stream, const SubGhzProtocol *protocol, SubGhzEnvironment *env);

// Enhanced analysis functions
static void subghz_toolkit_function_disassembly(SubGhzToolkitApp *app);
static void subghz_toolkit_protocol_state_analysis(SubGhzToolkitApp *app);
static void subghz_toolkit_signal_capture_analysis(SubGhzToolkitApp *app);
static void subghz_toolkit_timing_analysis(SubGhzToolkitApp *app);
static void subghz_toolkit_generate_c_headers(SubGhzToolkitApp *app);
static void subghz_toolkit_analyze_function_bytes(Stream *stream, const char *func_name, void *func_ptr, size_t max_bytes);
static void subghz_toolkit_analyze_protocol_state(Stream *stream, const SubGhzProtocol *protocol, SubGhzEnvironment *env);
static void subghz_toolkit_capture_signal_samples(Stream *stream, SubGhzReceiver *receiver);
static void subghz_toolkit_analyze_timing_patterns(Stream *stream, const SubGhzProtocol *protocol);
static void subghz_toolkit_generate_protocol_c_header(Stream *stream, const SubGhzProtocol *protocol);

static uint32_t subghz_toolkit_exit_callback(void *context)
{
    UNUSED(context);
    return VIEW_NONE;
}

static uint32_t subghz_toolkit_exit_to_submenu_callback(void *context)
{
    UNUSED(context);
    return SubGhzToolkitViewSubmenu;
}

static void subghz_toolkit_submenu_callback(void *context, uint32_t index)
{
    SubGhzToolkitApp *app = context;

    if (index == SubGhzToolkitSubmenuIndexDecryptKeeloq)
    {
        view_dispatcher_switch_to_view(app->view_dispatcher, SubGhzToolkitViewLoading);
        if (subghz_toolkit_export_keeloq_keys(app))
        {
            popup_set_header(app->popup, "Success!", 64, 10, AlignCenter, AlignTop);
            popup_set_text(app->popup, "Keeloq keys exported to:\n/ext/subghz/keeloq_keys.txt", 64, 20, AlignCenter, AlignTop);
        }
        else
        {
            popup_set_header(app->popup, "Error!", 64, 10, AlignCenter, AlignTop);
            popup_set_text(app->popup, "Failed to export Keeloq keys", 64, 20, AlignCenter, AlignTop);
        }
        popup_set_callback(app->popup, subghz_toolkit_popup_callback);
        popup_set_context(app->popup, app);
        popup_set_timeout(app->popup, 3000);
        popup_enable_timeout(app->popup);
        view_dispatcher_switch_to_view(app->view_dispatcher, SubGhzToolkitViewPopup);
    }
    else if (index == SubGhzToolkitSubmenuIndexListProtocols)
    {
        subghz_toolkit_show_protocols_list(app);
    }
    else if (index == SubGhzToolkitSubmenuIndexExportProtocolInfo)
    {
        view_dispatcher_switch_to_view(app->view_dispatcher, SubGhzToolkitViewLoading);
        subghz_toolkit_export_all_protocol_info(app);
        popup_set_header(app->popup, "Success!", 64, 10, AlignCenter, AlignTop);
        popup_set_text(app->popup, "Protocol info exported to:\n/ext/subghz/protocols.txt", 64, 20, AlignCenter, AlignTop);
        popup_set_callback(app->popup, subghz_toolkit_popup_callback);
        popup_set_context(app->popup, app);
        popup_set_timeout(app->popup, 3000);
        popup_enable_timeout(app->popup);
        view_dispatcher_switch_to_view(app->view_dispatcher, SubGhzToolkitViewPopup);
    }
    else if (index == SubGhzToolkitSubmenuIndexAdvancedAnalysis)
    {
        subghz_toolkit_advanced_analysis(app);
    }
    else if (index == SubGhzToolkitSubmenuIndexFunctionDisassembly)
    {
        subghz_toolkit_function_disassembly(app);
    }
    else if (index == SubGhzToolkitSubmenuIndexProtocolStateAnalysis)
    {
        subghz_toolkit_protocol_state_analysis(app);
    }
    else if (index == SubGhzToolkitSubmenuIndexSignalCapture)
    {
        subghz_toolkit_signal_capture_analysis(app);
    }
    else if (index == SubGhzToolkitSubmenuIndexTimingAnalysis)
    {
        subghz_toolkit_timing_analysis(app);
    }
    else if (index == SubGhzToolkitSubmenuIndexCHeaderGeneration)
    {
        subghz_toolkit_generate_c_headers(app);
    }
    else if (index == SubGhzToolkitSubmenuIndexAbout)
    {
        subghz_toolkit_show_about(app);
    }
    else if (index >= SubGhzToolkitSubmenuIndexProtocolDetails)
    {
        // Handle protocol-specific details
        size_t protocol_index = index - SubGhzToolkitSubmenuIndexProtocolDetails;
        const SubGhzProtocol *protocol = subghz_protocol_registry_get_by_index(app->protocol_registry, protocol_index);
        if (protocol && protocol->name)
        {
            subghz_toolkit_extract_protocol_details(app, protocol->name);
        }
    }
}

static void subghz_toolkit_popup_callback(void *context)
{
    SubGhzToolkitApp *app = context;
    view_dispatcher_switch_to_view(app->view_dispatcher, SubGhzToolkitViewSubmenu);
}

static bool subghz_toolkit_export_keeloq_keys(SubGhzToolkitApp *app)
{
    UNUSED(app);
    bool success = false;
    SubGhzKeystore *keystore = subghz_keystore_alloc();
    Storage *storage = furi_record_open(RECORD_STORAGE);
    Stream *stream = file_stream_alloc(storage);

    do
    {
        if (!subghz_keystore_load(keystore, EXT_PATH("subghz/assets/keeloq_mfcodes")))
        {
            FURI_LOG_E(TAG, "Failed to load keystore");
            break;
        }

        storage_simply_mkdir(storage, EXT_PATH("subghz"));
        storage_simply_mkdir(storage, SUBGHZ_ANALYSIS_DIR);

        if (!file_stream_open(stream, SUBGHZ_ANALYSIS_DIR "/keeloq_keys.txt",
                              FSAM_WRITE, FSOM_CREATE_ALWAYS))
        {
            FURI_LOG_E(TAG, "Failed to open output file");
            break;
        }

        stream_write_format(stream,
                            "====================================\n"
                            "  Flipper SubGhz KeeLoq Mfcodes\n"
                            "  Decrypted by SubGhz Toolkit\n"
                            "  RocketGod | betaskynet.com\n"
                            "====================================\n\n");

        SubGhzKeyArray_t *keys = subghz_keystore_get_data(keystore);
        size_t key_count = SubGhzKeyArray_size(*keys);

        stream_write_format(stream, "Total Keys: %zu\n\n", key_count);

        size_t exported = 0;
        for (size_t i = 0; i < key_count; i++)
        {
            const SubGhzKey *key = SubGhzKeyArray_get(*keys, i);

            stream_write_format(stream,
                                "Manufacturer: %s\n"
                                "Key (Hex):    %016llX\n"
                                "Key (Dec):    %llu\n"
                                "Type:         %hu\n"
                                "------------------------------------\n\n",
                                furi_string_get_cstr(key->name),
                                key->key,
                                key->key,
                                key->type);

            exported++;
        }

        success = (exported == key_count);

    } while (0);

    stream_free(stream);
    furi_record_close(RECORD_STORAGE);
    subghz_keystore_free(keystore);

    return success;
}

static void subghz_toolkit_extract_protocol_details(SubGhzToolkitApp *app, const char *protocol_name)
{
    furi_string_reset(app->text_buffer);
    furi_string_cat_printf(app->text_buffer, "=== %s Protocol Analysis ===\n\n", protocol_name);

    const SubGhzProtocol *protocol = NULL;
    size_t protocol_count = subghz_protocol_registry_count(app->protocol_registry);

    for (size_t i = 0; i < protocol_count; i++)
    {
        const SubGhzProtocol *p = subghz_protocol_registry_get_by_index(app->protocol_registry, i);
        if (p && p->name && strcmp(p->name, protocol_name) == 0)
        {
            protocol = p;
            break;
        }
    }

    if (!protocol)
    {
        furi_string_cat_str(app->text_buffer, "Protocol not found!\n");
        text_box_set_text(app->text_box, furi_string_get_cstr(app->text_buffer));
        view_dispatcher_switch_to_view(app->view_dispatcher, SubGhzToolkitViewTextBox);
        return;
    }

    furi_string_cat_printf(app->text_buffer, "Protocol Name: %s\n", protocol->name);
    furi_string_cat_printf(app->text_buffer, "Type: ");
    switch (protocol->type)
    {
    case SubGhzProtocolTypeStatic:
        furi_string_cat_str(app->text_buffer, "Static\n");
        break;
    case SubGhzProtocolTypeDynamic:
        furi_string_cat_str(app->text_buffer, "Dynamic\n");
        break;
    case SubGhzProtocolTypeRAW:
        furi_string_cat_str(app->text_buffer, "RAW\n");
        break;
    default:
        furi_string_cat_str(app->text_buffer, "Unknown\n");
        break;
    }

    furi_string_cat_printf(app->text_buffer, "Flag: 0x%08lX\n", (uint32_t)protocol->flag);

    if (protocol->decoder)
    {
        furi_string_cat_printf(app->text_buffer, "\nDecoder Functions:\n");
        furi_string_cat_printf(app->text_buffer, "- Alloc: %p\n", protocol->decoder->alloc);
        furi_string_cat_printf(app->text_buffer, "- Free: %p\n", protocol->decoder->free);
        furi_string_cat_printf(app->text_buffer, "- Feed: %p\n", protocol->decoder->feed);
        furi_string_cat_printf(app->text_buffer, "- Reset: %p\n", protocol->decoder->reset);
        furi_string_cat_printf(app->text_buffer, "- Get String: %p\n", protocol->decoder->get_string);
        furi_string_cat_printf(app->text_buffer, "- Serialize: %p\n", protocol->decoder->serialize);
        furi_string_cat_printf(app->text_buffer, "- Deserialize: %p\n", protocol->decoder->deserialize);
        furi_string_cat_printf(app->text_buffer, "- Get Hash: %p\n", protocol->decoder->get_hash_data);
    }

    if (protocol->encoder)
    {
        furi_string_cat_printf(app->text_buffer, "\nEncoder Functions:\n");
        furi_string_cat_printf(app->text_buffer, "- Alloc: %p\n", protocol->encoder->alloc);
        furi_string_cat_printf(app->text_buffer, "- Free: %p\n", protocol->encoder->free);
        furi_string_cat_printf(app->text_buffer, "- Deserialize: %p\n", protocol->encoder->deserialize);
        furi_string_cat_printf(app->text_buffer, "- Stop: %p\n", protocol->encoder->stop);
        furi_string_cat_printf(app->text_buffer, "- Yield: %p\n", protocol->encoder->yield);
    }

    if (app->setting)
    {
        furi_string_cat_printf(app->text_buffer, "\nSupported Frequencies:\n");
        for (size_t i = 0; i < subghz_setting_get_frequency_count(app->setting); i++)
        {
            uint32_t freq = subghz_setting_get_frequency(app->setting, i);
            furi_string_cat_printf(app->text_buffer, "- %lu Hz\n", freq);
        }
    }

    text_box_set_text(app->text_box, furi_string_get_cstr(app->text_buffer));
    text_box_set_focus(app->text_box, TextBoxFocusStart);
    view_dispatcher_switch_to_view(app->view_dispatcher, SubGhzToolkitViewTextBox);
}

static void subghz_toolkit_export_all_protocol_info(SubGhzToolkitApp *app)
{
    view_dispatcher_switch_to_view(app->view_dispatcher, SubGhzToolkitViewLoading);

    bool success = false;
    Storage *storage = furi_record_open(RECORD_STORAGE);
    Stream *stream = file_stream_alloc(storage);

    do
    {
        storage_simply_mkdir(storage, EXT_PATH("subghz"));
        storage_simply_mkdir(storage, SUBGHZ_ANALYSIS_DIR);

        if (!file_stream_open(stream, SUBGHZ_ANALYSIS_DIR "/protocol_analysis.txt",
                              FSAM_WRITE, FSOM_CREATE_ALWAYS))
        {
            break;
        }

        stream_write_format(stream,
                            "==============================================\n"
                            "     SubGhz Protocol Implementation Analysis\n"
                            "           Generated by SubGhz Toolkit\n"
                            "           RocketGod | betaskynet.com\n"
                            "==============================================\n\n");

        const Version *ver = furi_hal_version_get_firmware_version();
        stream_write_format(stream,
                            "Firmware Info:\n"
                            "Version: %s\n"
                            "Build Date: %s\n"
                            "Git Hash: %s\n"
                            "Target: %d\n\n",
                            version_get_version(ver),
                            version_get_builddate(ver),
                            version_get_githash(ver),
                            version_get_target(ver));

        size_t protocol_count = subghz_protocol_registry_count(app->protocol_registry);
        stream_write_format(stream, "Total Protocols: %zu\n\n", protocol_count);

        for (size_t i = 0; i < protocol_count; i++)
        {
            const SubGhzProtocol *protocol = subghz_protocol_registry_get_by_index(app->protocol_registry, i);
            if (!protocol || !protocol->name)
                continue;

            stream_write_format(stream, "\n========== %s ==========\n", protocol->name);

            stream_write_format(stream,
                                "Type: %s\n"
                                "Flag: 0x%08lX\n",
                                protocol->type == SubGhzProtocolTypeStatic ? "Static" : protocol->type == SubGhzProtocolTypeDynamic ? "Dynamic"
                                                                                                                                    : "RAW",
                                (uint32_t)protocol->flag);

            if (protocol->decoder)
            {
                stream_write_format(stream,
                                    "\nDecoder Functions:\n"
                                    "  Alloc:       %p\n"
                                    "  Free:        %p\n"
                                    "  Reset:       %p\n"
                                    "  Feed:        %p\n"
                                    "  Get String:  %p\n"
                                    "  Serialize:   %p\n"
                                    "  Deserialize: %p\n"
                                    "  Get Hash:    %p\n",
                                    protocol->decoder->alloc,
                                    protocol->decoder->free,
                                    protocol->decoder->reset,
                                    protocol->decoder->feed,
                                    protocol->decoder->get_string,
                                    protocol->decoder->serialize,
                                    protocol->decoder->deserialize,
                                    protocol->decoder->get_hash_data);
            }

            if (protocol->encoder)
            {
                stream_write_format(stream,
                                    "\nEncoder Functions:\n"
                                    "  Alloc:       %p\n"
                                    "  Free:        %p\n"
                                    "  Deserialize: %p\n"
                                    "  Stop:        %p\n"
                                    "  Yield:       %p\n",
                                    protocol->encoder->alloc,
                                    protocol->encoder->free,
                                    protocol->encoder->deserialize,
                                    protocol->encoder->stop,
                                    protocol->encoder->yield);
            }
        }

        success = true;
    } while (0);

    stream_free(stream);
    furi_record_close(RECORD_STORAGE);

    if (success)
    {
        popup_set_header(app->popup, "Success!", 64, 10, AlignCenter, AlignTop);
        popup_set_text(app->popup, "Protocol info exported to:\n/ext/subghz/analysis/protocol_analysis.txt",
                       64, 20, AlignCenter, AlignTop);
    }
    else
    {
        popup_set_header(app->popup, "Error!", 64, 10, AlignCenter, AlignTop);
        popup_set_text(app->popup, "Failed to export protocol info", 64, 20, AlignCenter, AlignTop);
    }

    popup_set_callback(app->popup, subghz_toolkit_popup_callback);
    popup_set_context(app->popup, app);
    popup_set_timeout(app->popup, 3000);
    popup_enable_timeout(app->popup);
    view_dispatcher_switch_to_view(app->view_dispatcher, SubGhzToolkitViewPopup);
}

static void subghz_toolkit_deep_protocol_analysis(Stream *stream, const SubGhzProtocol *protocol, SubGhzEnvironment *env)
{
    stream_write_format(stream, "\n  === DEEP ANALYSIS ===\n");

    stream_write_format(stream, "  Protocol Structure:\n");
    stream_write_format(stream, "    Protocol Ptr: %p\n", protocol);
    stream_write_format(stream, "    Name Ptr: %p -> \"%s\"\n", protocol->name, protocol->name);
    stream_write_format(stream, "    Type Value: 0x%02X\n", protocol->type);
    stream_write_format(stream, "    Flag Value: 0x%08lX\n", (uint32_t)protocol->flag);

    if (protocol->decoder)
    {
        stream_write_format(stream, "\n  Decoder Structure Analysis:\n");
        stream_write_format(stream, "    Decoder Ptr: %p\n", protocol->decoder);
        stream_write_format(stream, "    Size: %zu bytes\n", sizeof(*protocol->decoder));

        stream_write_format(stream, "\n    Function Entry Points:\n");
        if (protocol->decoder->alloc)
        {
            stream_write_format(stream, "      Alloc @ %p", protocol->decoder->alloc);
            uint8_t *func_bytes = (uint8_t *)protocol->decoder->alloc;
            stream_write_format(stream, " [");
            for (int i = 0; i < 8; i++)
            {
                stream_write_format(stream, "%02X ", func_bytes[i]);
            }
            stream_write_format(stream, "...]\n");
        }

        if (protocol->decoder->alloc && env)
        {
            SubGhzProtocolDecoderBase *decoder = protocol->decoder->alloc(env);
            if (decoder)
            {
                stream_write_format(stream, "\n    Decoder Instance Analysis:\n");
                stream_write_format(stream, "      Instance Ptr: %p\n", decoder);
                stream_write_format(stream, "      Protocol Ref: %p\n", decoder->protocol);
                stream_write_format(stream, "      Callback: %p\n", decoder->callback);

                if (decoder->protocol)
                {
                    stream_write_format(stream, "      Protocol Name: %s\n",
                                        decoder->protocol->name ? decoder->protocol->name : "NULL");
                }

                if (protocol->decoder->free)
                {
                    protocol->decoder->free(decoder);
                }
            }
        }
    }

    if (protocol->encoder)
    {
        stream_write_format(stream, "\n  Encoder Structure Analysis:\n");
        stream_write_format(stream, "    Encoder Ptr: %p\n", protocol->encoder);
        stream_write_format(stream, "    Size: %zu bytes\n", sizeof(*protocol->encoder));

        stream_write_format(stream, "\n    Function Entry Points:\n");
        if (protocol->encoder->alloc)
        {
            stream_write_format(stream, "      Alloc @ %p", protocol->encoder->alloc);
            uint8_t *func_bytes = (uint8_t *)protocol->encoder->alloc;
            stream_write_format(stream, " [");
            for (int i = 0; i < 8; i++)
            {
                stream_write_format(stream, "%02X ", func_bytes[i]);
            }
            stream_write_format(stream, "...]\n");
        }
    }

    stream_write_format(stream, "\n  Memory Layout:\n");
    stream_write_format(stream, "    Protocol @ %p\n", protocol);
    stream_write_format(stream, "    +0x00: name     = %p\n", &protocol->name);
    stream_write_format(stream, "    +0x04: type     = %p\n", &protocol->type);
    stream_write_format(stream, "    +0x08: flag     = %p\n", &protocol->flag);
    stream_write_format(stream, "    +0x0C: decoder  = %p\n", &protocol->decoder);
    stream_write_format(stream, "    +0x10: encoder  = %p\n", &protocol->encoder);
}

static void subghz_toolkit_advanced_analysis(SubGhzToolkitApp *app)
{
    view_dispatcher_switch_to_view(app->view_dispatcher, SubGhzToolkitViewLoading);

    bool success = false;
    Storage *storage = furi_record_open(RECORD_STORAGE);
    Stream *stream = file_stream_alloc(storage);

    do
    {
        storage_simply_mkdir(storage, EXT_PATH("subghz"));
        storage_simply_mkdir(storage, SUBGHZ_ANALYSIS_DIR);

        if (!file_stream_open(stream, SUBGHZ_ANALYSIS_DIR "/advanced_analysis.txt",
                              FSAM_WRITE, FSOM_CREATE_ALWAYS))
        {
            break;
        }

        stream_write_format(stream,
                            "==============================================================\n"
                            "        SubGhz Protocol ADVANCED Implementation Analysis\n"
                            "                  Generated by SubGhz Toolkit\n"
                            "                 RocketGod | betaskynet.com\n"
                            "==============================================================\n\n");

        const Version *ver = furi_hal_version_get_firmware_version();
        stream_write_format(stream,
                            "System Information:\n"
                            "  Firmware Version: %s\n"
                            "  Build Date: %s\n"
                            "  Git Hash: %s\n"
                            "  Target: %d\n"
                            "  HW Version: %d\n"
                            "  HW Target: %d\n"
                            "  HW Body: %d\n"
                            "  HW Connect: %d\n"
                            "  HW Region: %d\n"
                            "  HW Display: %d\n\n",
                            version_get_version(ver),
                            version_get_builddate(ver),
                            version_get_githash(ver),
                            version_get_target(ver),
                            furi_hal_version_get_hw_version(),
                            furi_hal_version_get_hw_target(),
                            furi_hal_version_get_hw_body(),
                            furi_hal_version_get_hw_connect(),
                            furi_hal_version_get_hw_region(),
                            furi_hal_version_get_hw_display());

        stream_write_format(stream, "Protocol Registry Analysis:\n");
        stream_write_format(stream, "  Registry Ptr: %p\n", app->protocol_registry);
        stream_write_format(stream, "  Protocol Count: %zu\n", subghz_protocol_registry_count(app->protocol_registry));
        stream_write_format(stream, "  Registry Symbol: subghz_protocol_registry @ %p\n\n", &subghz_protocol_registry);

        stream_write_format(stream, "SubGhz Environment Analysis:\n");
        stream_write_format(stream, "  Environment Ptr: %p\n", app->environment);
        stream_write_format(stream, "  Receiver Ptr: %p\n", app->receiver);
        stream_write_format(stream, "  Setting Ptr: %p\n\n", app->setting);

        size_t protocol_count = subghz_protocol_registry_count(app->protocol_registry);

        for (size_t i = 0; i < protocol_count; i++)
        {
            const SubGhzProtocol *protocol = subghz_protocol_registry_get_by_index(app->protocol_registry, i);
            if (!protocol || !protocol->name)
                continue;

            stream_write_format(stream, "\n████████████████████████████████████████████████████████████\n");
            stream_write_format(stream, "Protocol #%zu: %s\n", i, protocol->name);
            stream_write_format(stream, "████████████████████████████████████████████████████████████\n");

            stream_write_format(stream, "\nBasic Information:\n");
            stream_write_format(stream, "  Name: %s\n", protocol->name);
            stream_write_format(stream, "  Type: 0x%02X (%s)\n",
                                protocol->type,
                                protocol->type == SubGhzProtocolTypeStatic ? "Static" : protocol->type == SubGhzProtocolTypeDynamic ? "Dynamic"
                                                                                                                                    : "RAW");
            stream_write_format(stream, "  Flag: 0x%08lX\n", (uint32_t)protocol->flag);

            stream_write_format(stream, "\n  Flag Breakdown:\n");
            stream_write_format(stream, "    Decodable:       %s\n", (protocol->flag & SubGhzProtocolFlag_Decodable) ? "YES" : "NO");
            stream_write_format(stream, "    Save:            %s\n", (protocol->flag & SubGhzProtocolFlag_Save) ? "YES" : "NO");
            stream_write_format(stream, "    Load:            %s\n", (protocol->flag & SubGhzProtocolFlag_Load) ? "YES" : "NO");
            stream_write_format(stream, "    Send:            %s\n", (protocol->flag & SubGhzProtocolFlag_Send) ? "YES" : "NO");
            stream_write_format(stream, "    BinRAW:          %s\n", (protocol->flag & SubGhzProtocolFlag_BinRAW) ? "YES" : "NO");

            if (protocol->decoder)
            {
                stream_write_format(stream, "\nDecoder Implementation:\n");
                stream_write_format(stream, "  Structure Address: %p\n", protocol->decoder);
                stream_write_format(stream, "\n  Function Pointers:\n");
                stream_write_format(stream, "    alloc:          %p\n", protocol->decoder->alloc);
                stream_write_format(stream, "    free:           %p\n", protocol->decoder->free);
                stream_write_format(stream, "    reset:          %p\n", protocol->decoder->reset);
                stream_write_format(stream, "    feed:           %p\n", protocol->decoder->feed);
                stream_write_format(stream, "    get_string:     %p\n", protocol->decoder->get_string);
                stream_write_format(stream, "    serialize:      %p\n", protocol->decoder->serialize);
                stream_write_format(stream, "    deserialize:    %p\n", protocol->decoder->deserialize);
                stream_write_format(stream, "    get_hash_data:  %p\n", protocol->decoder->get_hash_data);
            }

            if (protocol->encoder)
            {
                stream_write_format(stream, "\nEncoder Implementation:\n");
                stream_write_format(stream, "  Structure Address: %p\n", protocol->encoder);
                stream_write_format(stream, "\n  Function Pointers:\n");
                stream_write_format(stream, "    alloc:          %p\n", protocol->encoder->alloc);
                stream_write_format(stream, "    free:           %p\n", protocol->encoder->free);
                stream_write_format(stream, "    deserialize:    %p\n", protocol->encoder->deserialize);
                stream_write_format(stream, "    stop:           %p\n", protocol->encoder->stop);
                stream_write_format(stream, "    yield:          %p\n", protocol->encoder->yield);
            }

            subghz_toolkit_deep_protocol_analysis(stream, protocol, app->environment);

            stream_write_format(stream, "\n");
        }

        stream_write_format(stream, "\n████████████████████████████████████████████████████████████\n");
        stream_write_format(stream, "Memory Map Summary\n");
        stream_write_format(stream, "████████████████████████████████████████████████████████████\n\n");

        void *min_addr = (void *)0xFFFFFFFF;
        void *max_addr = (void *)0x00000000;

        for (size_t i = 0; i < protocol_count; i++)
        {
            const SubGhzProtocol *protocol = subghz_protocol_registry_get_by_index(app->protocol_registry, i);
            if (!protocol)
                continue;

            if ((void *)protocol < min_addr)
                min_addr = (void *)protocol;
            if ((void *)protocol > max_addr)
                max_addr = (void *)protocol;

            if (protocol->decoder)
            {
                if ((void *)protocol->decoder->alloc < min_addr)
                    min_addr = (void *)protocol->decoder->alloc;
                if ((void *)protocol->decoder->get_hash_data > max_addr)
                    max_addr = (void *)protocol->decoder->get_hash_data;
            }

            if (protocol->encoder)
            {
                if ((void *)protocol->encoder->alloc < min_addr)
                    min_addr = (void *)protocol->encoder->alloc;
                if ((void *)protocol->encoder->yield > max_addr)
                    max_addr = (void *)protocol->encoder->yield;
            }
        }

        stream_write_format(stream, "Address Range: %p - %p\n", min_addr, max_addr);
        stream_write_format(stream, "Total Range: %lu bytes\n\n", (uint32_t)max_addr - (uint32_t)min_addr);

        success = true;
    } while (0);

    stream_free(stream);
    furi_record_close(RECORD_STORAGE);

    if (success)
    {
        popup_set_header(app->popup, "Success!", 64, 10, AlignCenter, AlignTop);
        popup_set_text(app->popup, "Advanced analysis exported to:\n/ext/subghz/analysis/advanced_analysis.txt",
                       64, 20, AlignCenter, AlignTop);
    }
    else
    {
        popup_set_header(app->popup, "Error!", 64, 10, AlignCenter, AlignTop);
        popup_set_text(app->popup, "Failed to export analysis", 64, 20, AlignCenter, AlignTop);
    }

    popup_set_callback(app->popup, subghz_toolkit_popup_callback);
    popup_set_context(app->popup, app);
    popup_set_timeout(app->popup, 3000);
    popup_enable_timeout(app->popup);
    view_dispatcher_switch_to_view(app->view_dispatcher, SubGhzToolkitViewPopup);
}

static void subghz_toolkit_show_protocols_list(SubGhzToolkitApp *app)
{
    furi_string_reset(app->text_buffer);
    furi_string_cat_printf(app->text_buffer, "SubGhz Protocols Found: %zu\n\n",
                           subghz_protocol_registry_count(app->protocol_registry));

    submenu_reset(app->submenu);

    submenu_add_item(
        app->submenu,
        "Decrypt Keeloq mfcodes",
        SubGhzToolkitSubmenuIndexDecryptKeeloq,
        subghz_toolkit_submenu_callback,
        app);

    submenu_add_item(
        app->submenu,
        "List SubGhz Protocols",
        SubGhzToolkitSubmenuIndexListProtocols,
        subghz_toolkit_submenu_callback,
        app);

    submenu_add_item(
        app->submenu,
        "Export All Protocol Info",
        SubGhzToolkitSubmenuIndexExportProtocolInfo,
        subghz_toolkit_submenu_callback,
        app);

    submenu_add_item(
        app->submenu,
        "Advanced Analysis",
        SubGhzToolkitSubmenuIndexAdvancedAnalysis,
        subghz_toolkit_submenu_callback,
        app);

    submenu_add_item(
        app->submenu,
        "Function Disassembly",
        SubGhzToolkitSubmenuIndexFunctionDisassembly,
        subghz_toolkit_submenu_callback,
        app);

    submenu_add_item(
        app->submenu,
        "Protocol State Analysis",
        SubGhzToolkitSubmenuIndexProtocolStateAnalysis,
        subghz_toolkit_submenu_callback,
        app);

    submenu_add_item(
        app->submenu,
        "Signal Capture Analysis",
        SubGhzToolkitSubmenuIndexSignalCapture,
        subghz_toolkit_submenu_callback,
        app);

    submenu_add_item(
        app->submenu,
        "Timing Analysis",
        SubGhzToolkitSubmenuIndexTimingAnalysis,
        subghz_toolkit_submenu_callback,
        app);

    submenu_add_item(
        app->submenu,
        "Generate C Headers",
        SubGhzToolkitSubmenuIndexCHeaderGeneration,
        subghz_toolkit_submenu_callback,
        app);

    submenu_add_item(
        app->submenu,
        "About",
        SubGhzToolkitSubmenuIndexAbout,
        subghz_toolkit_submenu_callback,
        app);

    size_t protocol_count = subghz_protocol_registry_count(app->protocol_registry);
    for (size_t i = 0; i < protocol_count; i++)
    {
        const SubGhzProtocol *protocol = subghz_protocol_registry_get_by_index(app->protocol_registry, i);
        if (protocol && protocol->name)
        {
            furi_string_cat_printf(app->text_buffer, "%zu. %s", i + 1, protocol->name);
            if (protocol->type == SubGhzProtocolTypeStatic)
            {
                furi_string_cat_str(app->text_buffer, " [Static]");
            }
            else if (protocol->type == SubGhzProtocolTypeDynamic)
            {
                furi_string_cat_str(app->text_buffer, " [Dynamic]");
            }
            else if (protocol->type == SubGhzProtocolTypeRAW)
            {
                furi_string_cat_str(app->text_buffer, " [RAW]");
            }
            furi_string_cat_str(app->text_buffer, "\n");

            submenu_add_item(
                app->submenu,
                protocol->name,
                SubGhzToolkitSubmenuIndexProtocolDetails + i,
                subghz_toolkit_submenu_callback,
                app);
        }
    }

    text_box_set_text(app->text_box, furi_string_get_cstr(app->text_buffer));
    text_box_set_focus(app->text_box, TextBoxFocusStart);
    view_dispatcher_switch_to_view(app->view_dispatcher, SubGhzToolkitViewTextBox);
}

static void subghz_toolkit_show_about(SubGhzToolkitApp *app)
{
    furi_string_reset(app->text_buffer);
    furi_string_cat_str(app->text_buffer,
                        "=== SubGhz Toolkit ===\n"
                        "Version: " SUBGHZ_TOOLKIT_VERSION "\n\n"
                        "RocketGod was here\n"
                        "https://betaskynet.com\n\n"
                        "Features:\n"
                        "- Decrypt KeeLoq keys\n"
                        "- List all protocols\n"
                        "- Export protocol info\n"
                        "- ADVANCED Analysis\n"
                        "- Analyze implementations\n\n"
                        "Files saved to:\n"
                        "/ext/subghz/analysis/\n\n"
                        "Use this tool to compare\n"
                        "different firmware versions!");

    text_box_set_text(app->text_box, furi_string_get_cstr(app->text_buffer));
    text_box_set_focus(app->text_box, TextBoxFocusStart);
    view_dispatcher_switch_to_view(app->view_dispatcher, SubGhzToolkitViewTextBox);
}

static void subghz_toolkit_show_intro_popup(SubGhzToolkitApp *app)
{
    popup_set_header(app->popup, "SubGhz Toolkit", 64, 10, AlignCenter, AlignTop);
    popup_set_text(app->popup, "RocketGod was here\nhttps://betaskynet.com", 64, 20, AlignCenter, AlignTop);
    popup_set_callback(app->popup, subghz_toolkit_popup_callback);
    popup_set_context(app->popup, app);
    popup_set_timeout(app->popup, 3000);
    popup_enable_timeout(app->popup);
    view_dispatcher_switch_to_view(app->view_dispatcher, SubGhzToolkitViewPopup);
}

// Enhanced analysis functions for comprehensive protocol implementation data

static void subghz_toolkit_analyze_function_bytes(Stream *stream, const char *func_name, void *func_ptr, size_t max_bytes)
{
    if (!func_ptr) return;
    
    stream_write_format(stream, "\n  Function: %s @ %p\n", func_name, func_ptr);
    stream_write_format(stream, "  Raw Bytes (first %zu bytes):\n", max_bytes);
    
    uint8_t *bytes = (uint8_t *)func_ptr;
    stream_write_format(stream, "    ");
    
    for (size_t i = 0; i < max_bytes && i < 64; i++) // Limit to 64 bytes for readability
    {
        stream_write_format(stream, "%02X ", bytes[i]);
        if ((i + 1) % 16 == 0)
        {
            stream_write_format(stream, "\n    ");
        }
    }
    
    // Basic ARM instruction pattern analysis
    stream_write_format(stream, "\n  ARM Instruction Analysis:\n");
    for (size_t i = 0; i < max_bytes - 3; i += 4)
    {
        uint32_t instruction = *(uint32_t *)(bytes + i);
        
        // Common ARM patterns
        if ((instruction & 0xFF000000) == 0xE9000000) // STMDB
        {
            stream_write_format(stream, "    +%02zu: STMDB (stack push)\n", i);
        }
        else if ((instruction & 0xFF000000) == 0xE8B00000) // LDMIA
        {
            stream_write_format(stream, "    +%02zu: LDMIA (stack pop)\n", i);
        }
        else if ((instruction & 0xFF000000) == 0xE1A00000) // MOV
        {
            stream_write_format(stream, "    +%02zu: MOV (register move)\n", i);
        }
        else if ((instruction & 0xFF000000) == 0xE3A00000) // MOV immediate
        {
            stream_write_format(stream, "    +%02zu: MOV immediate\n", i);
        }
        else if ((instruction & 0xFF000000) == 0xE5900000) // LDR
        {
            stream_write_format(stream, "    +%02zu: LDR (load register)\n", i);
        }
        else if ((instruction & 0xFF000000) == 0xE5800000) // STR
        {
            stream_write_format(stream, "    +%02zu: STR (store register)\n", i);
        }
        else if ((instruction & 0xFF000000) == 0xEB000000) // BL
        {
            stream_write_format(stream, "    +%02zu: BL (branch and link)\n", i);
        }
        else if ((instruction & 0xFF000000) == 0xEA000000) // B
        {
            stream_write_format(stream, "    +%02zu: B (branch)\n", i);
        }
        else if ((instruction & 0xFF000000) == 0xE12FFF10) // BX
        {
            stream_write_format(stream, "    +%02zu: BX (branch exchange)\n", i);
        }
        else if ((instruction & 0xFF000000) == 0xE1A0F000) // MOV PC, LR
        {
            stream_write_format(stream, "    +%02zu: MOV PC, LR (return)\n", i);
        }
    }
}

static void subghz_toolkit_analyze_protocol_state(Stream *stream, const SubGhzProtocol *protocol, SubGhzEnvironment *env)
{
    if (!protocol->decoder || !env) return;
    
    stream_write_format(stream, "\n  Protocol State Analysis:\n");
    
    SubGhzProtocolDecoderBase *decoder = protocol->decoder->alloc(env);
    if (decoder)
    {
        stream_write_format(stream, "    Decoder Instance: %p\n", decoder);
        stream_write_format(stream, "    Decoder Size: %zu bytes\n", sizeof(*decoder));
        
        // Analyze decoder structure
        stream_write_format(stream, "    Decoder Structure Dump:\n");
        uint8_t *decoder_bytes = (uint8_t *)decoder;
        for (size_t i = 0; i < sizeof(*decoder); i += 4)
        {
            if (i + 3 < sizeof(*decoder))
            {
                uint32_t value = *(uint32_t *)(decoder_bytes + i);
                stream_write_format(stream, "      +%02zu: 0x%08lX\n", i, (uint32_t)value);
            }
        }
        
        if (protocol->decoder->free)
        {
            protocol->decoder->free(decoder);
        }
    }
}

static void subghz_toolkit_capture_signal_samples(Stream *stream, SubGhzReceiver *receiver)
{
    stream_write_format(stream, "\n  Signal Capture Analysis:\n");
    stream_write_format(stream, "    Receiver: %p\n", receiver);
    
    // Capture some signal samples for analysis
    stream_write_format(stream, "    Capturing signal samples...\n");
    
    // This would need to be implemented with actual signal capture
    // For now, we'll document the approach
    stream_write_format(stream, "    Signal capture approach:\n");
    stream_write_format(stream, "    1. Start receiver\n");
    stream_write_format(stream, "    2. Capture raw signal data\n");
    stream_write_format(stream, "    3. Analyze timing patterns\n");
    stream_write_format(stream, "    4. Extract protocol parameters\n");
}

static void subghz_toolkit_analyze_timing_patterns(Stream *stream, const SubGhzProtocol *protocol)
{
    stream_write_format(stream, "\n  Timing Pattern Analysis:\n");
    stream_write_format(stream, "    Protocol: %s\n", protocol->name);
    
    // Common timing patterns for different protocols
    stream_write_format(stream, "    Common timing patterns:\n");
    stream_write_format(stream, "    - Manchester: 50/50 duty cycle\n");
    stream_write_format(stream, "    - PWM: Variable pulse width\n");
    stream_write_format(stream, "    - PPM: Pulse position modulation\n");
    stream_write_format(stream, "    - RAW: Custom timing patterns\n");
    
    // Analyze protocol type for timing hints
    switch (protocol->type)
    {
        case SubGhzProtocolTypeStatic:
            stream_write_format(stream, "    Type: Static (fixed timing)\n");
            break;
        case SubGhzProtocolTypeDynamic:
            stream_write_format(stream, "    Type: Dynamic (variable timing)\n");
            break;
        default:
            stream_write_format(stream, "    Type: RAW (custom timing)\n");
            break;
    }
}

static void subghz_toolkit_generate_protocol_c_header(Stream *stream, const SubGhzProtocol *protocol)
{
    stream_write_format(stream, "\n// Generated C Header for Protocol: %s\n", protocol->name);
    stream_write_format(stream, "#ifndef %s_PROTOCOL_H\n", protocol->name);
    stream_write_format(stream, "#define %s_PROTOCOL_H\n\n", protocol->name);
    
    stream_write_format(stream, "#include <stdint.h>\n");
    stream_write_format(stream, "#include <stddef.h>\n\n");
    
    stream_write_format(stream, "// Protocol Information\n");
    stream_write_format(stream, "#define %s_PROTOCOL_NAME \"%s\"\n", protocol->name, protocol->name);
    stream_write_format(stream, "#define %s_PROTOCOL_TYPE 0x%02X\n", protocol->name, protocol->type);
    stream_write_format(stream, "#define %s_PROTOCOL_FLAG 0x%08lX\n\n", protocol->name, (uint32_t)protocol->flag);
    
    stream_write_format(stream, "// Function Pointer Types\n");
    stream_write_format(stream, "typedef void* (*%s_alloc_func)(void* env);\n", protocol->name);
    stream_write_format(stream, "typedef void (*%s_free_func)(void* decoder);\n", protocol->name);
    stream_write_format(stream, "typedef void (*%s_reset_func)(void* decoder);\n", protocol->name);
    stream_write_format(stream, "typedef void (*%s_feed_func)(void* decoder, bool level, uint32_t duration);\n", protocol->name);
    stream_write_format(stream, "typedef void (*%s_get_string_func)(void* decoder, FuriString* output);\n", protocol->name);
    
    stream_write_format(stream, "\n// Protocol Structure\n");
    stream_write_format(stream, "typedef struct {\n");
    stream_write_format(stream, "    const char* name;\n");
    stream_write_format(stream, "    uint8_t type;\n");
    stream_write_format(stream, "    uint32_t flag;\n");
    stream_write_format(stream, "    struct {\n");
    stream_write_format(stream, "        %s_alloc_func alloc;\n", protocol->name);
    stream_write_format(stream, "        %s_free_func free;\n", protocol->name);
    stream_write_format(stream, "        %s_reset_func reset;\n", protocol->name);
    stream_write_format(stream, "        %s_feed_func feed;\n", protocol->name);
    stream_write_format(stream, "        %s_get_string_func get_string;\n", protocol->name);
    stream_write_format(stream, "    } decoder;\n");
    stream_write_format(stream, "} %s_Protocol;\n\n", protocol->name);
    
    stream_write_format(stream, "// Implementation Notes\n");
    stream_write_format(stream, "// - Function pointers can be extracted from firmware\n");
    stream_write_format(stream, "// - Timing patterns need to be analyzed from signals\n");
    stream_write_format(stream, "// - Protocol state machine needs reverse engineering\n");
    stream_write_format(stream, "// - Use signal capture to understand data encoding\n\n");
    
    stream_write_format(stream, "#endif // %s_PROTOCOL_H\n", protocol->name);
}

static void subghz_toolkit_function_disassembly(SubGhzToolkitApp *app)
{
    view_dispatcher_switch_to_view(app->view_dispatcher, SubGhzToolkitViewLoading);

    bool success = false;
    Storage *storage = furi_record_open(RECORD_STORAGE);
    Stream *stream = file_stream_alloc(storage);

    do
    {
        storage_simply_mkdir(storage, EXT_PATH("subghz"));
        storage_simply_mkdir(storage, SUBGHZ_ANALYSIS_DIR);

        if (!file_stream_open(stream, SUBGHZ_ANALYSIS_DIR "/function_disassembly.txt",
                              FSAM_WRITE, FSOM_CREATE_ALWAYS))
        {
            break;
        }

        stream_write_format(stream,
                            "==============================================================\n"
                            "        SubGhz Protocol Function Disassembly Analysis\n"
                            "                  Generated by SubGhz Toolkit\n"
                            "                 RocketGod | betaskynet.com\n"
                            "==============================================================\n\n");

        size_t protocol_count = subghz_protocol_registry_count(app->protocol_registry);

        for (size_t i = 0; i < protocol_count; i++)
        {
            const SubGhzProtocol *protocol = subghz_protocol_registry_get_by_index(app->protocol_registry, i);
            if (!protocol || !protocol->name)
                continue;

            stream_write_format(stream, "\n████████████████████████████████████████████████████████████\n");
            stream_write_format(stream, "Protocol: %s - Function Disassembly\n", protocol->name);
            stream_write_format(stream, "████████████████████████████████████████████████████████████\n");

            if (protocol->decoder)
            {
                stream_write_format(stream, "\nDECODER FUNCTIONS:\n");
                stream_write_format(stream, "==================\n");
                
                subghz_toolkit_analyze_function_bytes(stream, "decoder->alloc", protocol->decoder->alloc, 64);
                subghz_toolkit_analyze_function_bytes(stream, "decoder->free", protocol->decoder->free, 64);
                subghz_toolkit_analyze_function_bytes(stream, "decoder->reset", protocol->decoder->reset, 64);
                subghz_toolkit_analyze_function_bytes(stream, "decoder->feed", protocol->decoder->feed, 64);
                subghz_toolkit_analyze_function_bytes(stream, "decoder->get_string", protocol->decoder->get_string, 64);
                subghz_toolkit_analyze_function_bytes(stream, "decoder->serialize", protocol->decoder->serialize, 64);
                subghz_toolkit_analyze_function_bytes(stream, "decoder->deserialize", protocol->decoder->deserialize, 64);
                subghz_toolkit_analyze_function_bytes(stream, "decoder->get_hash_data", protocol->decoder->get_hash_data, 64);
            }

            if (protocol->encoder)
            {
                stream_write_format(stream, "\nENCODER FUNCTIONS:\n");
                stream_write_format(stream, "==================\n");
                
                subghz_toolkit_analyze_function_bytes(stream, "encoder->alloc", protocol->encoder->alloc, 64);
                subghz_toolkit_analyze_function_bytes(stream, "encoder->free", protocol->encoder->free, 64);
                subghz_toolkit_analyze_function_bytes(stream, "encoder->deserialize", protocol->encoder->deserialize, 64);
                subghz_toolkit_analyze_function_bytes(stream, "encoder->stop", protocol->encoder->stop, 64);
                subghz_toolkit_analyze_function_bytes(stream, "encoder->yield", protocol->encoder->yield, 64);
            }

            stream_write_format(stream, "\n");
        }

        success = true;
    } while (0);

    stream_free(stream);
    furi_record_close(RECORD_STORAGE);

    if (success)
    {
        popup_set_header(app->popup, "Success!", 64, 10, AlignCenter, AlignTop);
        popup_set_text(app->popup, "Function disassembly exported to:\n/ext/subghz/analysis/function_disassembly.txt",
                       64, 20, AlignCenter, AlignTop);
    }
    else
    {
        popup_set_header(app->popup, "Error!", 64, 10, AlignCenter, AlignTop);
        popup_set_text(app->popup, "Failed to export disassembly", 64, 20, AlignCenter, AlignTop);
    }

    popup_set_callback(app->popup, subghz_toolkit_popup_callback);
    popup_set_context(app->popup, app);
    popup_set_timeout(app->popup, 3000);
    popup_enable_timeout(app->popup);
    view_dispatcher_switch_to_view(app->view_dispatcher, SubGhzToolkitViewPopup);
}

static void subghz_toolkit_protocol_state_analysis(SubGhzToolkitApp *app)
{
    view_dispatcher_switch_to_view(app->view_dispatcher, SubGhzToolkitViewLoading);

    bool success = false;
    Storage *storage = furi_record_open(RECORD_STORAGE);
    Stream *stream = file_stream_alloc(storage);

    do
    {
        storage_simply_mkdir(storage, EXT_PATH("subghz"));
        storage_simply_mkdir(storage, SUBGHZ_ANALYSIS_DIR);

        if (!file_stream_open(stream, SUBGHZ_ANALYSIS_DIR "/protocol_state_analysis.txt",
                              FSAM_WRITE, FSOM_CREATE_ALWAYS))
        {
            break;
        }

        stream_write_format(stream,
                            "==============================================================\n"
                            "        SubGhz Protocol State Analysis\n"
                            "                  Generated by SubGhz Toolkit\n"
                            "                 RocketGod | betaskynet.com\n"
                            "==============================================================\n\n");

        size_t protocol_count = subghz_protocol_registry_count(app->protocol_registry);

        for (size_t i = 0; i < protocol_count; i++)
        {
            const SubGhzProtocol *protocol = subghz_protocol_registry_get_by_index(app->protocol_registry, i);
            if (!protocol || !protocol->name)
                continue;

            stream_write_format(stream, "\n████████████████████████████████████████████████████████████\n");
            stream_write_format(stream, "Protocol: %s - State Analysis\n", protocol->name);
            stream_write_format(stream, "████████████████████████████████████████████████████████████\n");

            subghz_toolkit_analyze_protocol_state(stream, protocol, app->environment);
            stream_write_format(stream, "\n");
        }

        success = true;
    } while (0);

    stream_free(stream);
    furi_record_close(RECORD_STORAGE);

    if (success)
    {
        popup_set_header(app->popup, "Success!", 64, 10, AlignCenter, AlignTop);
        popup_set_text(app->popup, "Protocol state analysis exported to:\n/ext/subghz/analysis/protocol_state_analysis.txt",
                       64, 20, AlignCenter, AlignTop);
    }
    else
    {
        popup_set_header(app->popup, "Error!", 64, 10, AlignCenter, AlignTop);
        popup_set_text(app->popup, "Failed to export state analysis", 64, 20, AlignCenter, AlignTop);
    }

    popup_set_callback(app->popup, subghz_toolkit_popup_callback);
    popup_set_context(app->popup, app);
    popup_set_timeout(app->popup, 3000);
    popup_enable_timeout(app->popup);
    view_dispatcher_switch_to_view(app->view_dispatcher, SubGhzToolkitViewPopup);
}

static void subghz_toolkit_signal_capture_analysis(SubGhzToolkitApp *app)
{
    view_dispatcher_switch_to_view(app->view_dispatcher, SubGhzToolkitViewLoading);

    bool success = false;
    Storage *storage = furi_record_open(RECORD_STORAGE);
    Stream *stream = file_stream_alloc(storage);

    do
    {
        storage_simply_mkdir(storage, EXT_PATH("subghz"));
        storage_simply_mkdir(storage, SUBGHZ_ANALYSIS_DIR);

        if (!file_stream_open(stream, SUBGHZ_ANALYSIS_DIR "/signal_capture_analysis.txt",
                              FSAM_WRITE, FSOM_CREATE_ALWAYS))
        {
            break;
        }

        stream_write_format(stream,
                            "==============================================================\n"
                            "        SubGhz Signal Capture Analysis\n"
                            "                  Generated by SubGhz Toolkit\n"
                            "                 RocketGod | betaskynet.com\n"
                            "==============================================================\n\n");

        subghz_toolkit_capture_signal_samples(stream, app->receiver);

        success = true;
    } while (0);

    stream_free(stream);
    furi_record_close(RECORD_STORAGE);

    if (success)
    {
        popup_set_header(app->popup, "Success!", 64, 10, AlignCenter, AlignTop);
        popup_set_text(app->popup, "Signal capture analysis exported to:\n/ext/subghz/analysis/signal_capture_analysis.txt",
                       64, 20, AlignCenter, AlignTop);
    }
    else
    {
        popup_set_header(app->popup, "Error!", 64, 10, AlignCenter, AlignTop);
        popup_set_text(app->popup, "Failed to export signal analysis", 64, 20, AlignCenter, AlignTop);
    }

    popup_set_callback(app->popup, subghz_toolkit_popup_callback);
    popup_set_context(app->popup, app);
    popup_set_timeout(app->popup, 3000);
    popup_enable_timeout(app->popup);
    view_dispatcher_switch_to_view(app->view_dispatcher, SubGhzToolkitViewPopup);
}

static void subghz_toolkit_timing_analysis(SubGhzToolkitApp *app)
{
    view_dispatcher_switch_to_view(app->view_dispatcher, SubGhzToolkitViewLoading);

    bool success = false;
    Storage *storage = furi_record_open(RECORD_STORAGE);
    Stream *stream = file_stream_alloc(storage);

    do
    {
        storage_simply_mkdir(storage, EXT_PATH("subghz"));
        storage_simply_mkdir(storage, SUBGHZ_ANALYSIS_DIR);

        if (!file_stream_open(stream, SUBGHZ_ANALYSIS_DIR "/timing_analysis.txt",
                              FSAM_WRITE, FSOM_CREATE_ALWAYS))
        {
            break;
        }

        stream_write_format(stream,
                            "==============================================================\n"
                            "        SubGhz Protocol Timing Analysis\n"
                            "                  Generated by SubGhz Toolkit\n"
                            "                 RocketGod | betaskynet.com\n"
                            "==============================================================\n\n");

        size_t protocol_count = subghz_protocol_registry_count(app->protocol_registry);

        for (size_t i = 0; i < protocol_count; i++)
        {
            const SubGhzProtocol *protocol = subghz_protocol_registry_get_by_index(app->protocol_registry, i);
            if (!protocol || !protocol->name)
                continue;

            stream_write_format(stream, "\n████████████████████████████████████████████████████████████\n");
            stream_write_format(stream, "Protocol: %s - Timing Analysis\n", protocol->name);
            stream_write_format(stream, "████████████████████████████████████████████████████████████\n");

            subghz_toolkit_analyze_timing_patterns(stream, protocol);
            stream_write_format(stream, "\n");
        }

        success = true;
    } while (0);

    stream_free(stream);
    furi_record_close(RECORD_STORAGE);

    if (success)
    {
        popup_set_header(app->popup, "Success!", 64, 10, AlignCenter, AlignTop);
        popup_set_text(app->popup, "Timing analysis exported to:\n/ext/subghz/analysis/timing_analysis.txt",
                       64, 20, AlignCenter, AlignTop);
    }
    else
    {
        popup_set_header(app->popup, "Error!", 64, 10, AlignCenter, AlignTop);
        popup_set_text(app->popup, "Failed to export timing analysis", 64, 20, AlignCenter, AlignTop);
    }

    popup_set_callback(app->popup, subghz_toolkit_popup_callback);
    popup_set_context(app->popup, app);
    popup_set_timeout(app->popup, 3000);
    popup_enable_timeout(app->popup);
    view_dispatcher_switch_to_view(app->view_dispatcher, SubGhzToolkitViewPopup);
}

static void subghz_toolkit_generate_c_headers(SubGhzToolkitApp *app)
{
    view_dispatcher_switch_to_view(app->view_dispatcher, SubGhzToolkitViewLoading);

    bool success = false;
    Storage *storage = furi_record_open(RECORD_STORAGE);
    Stream *stream = file_stream_alloc(storage);

    do
    {
        storage_simply_mkdir(storage, EXT_PATH("subghz"));
        storage_simply_mkdir(storage, SUBGHZ_ANALYSIS_DIR);

        if (!file_stream_open(stream, SUBGHZ_ANALYSIS_DIR "/protocol_headers.h",
                              FSAM_WRITE, FSOM_CREATE_ALWAYS))
        {
            break;
        }

        stream_write_format(stream,
                            "// ==============================================================\n"
                            "//        SubGhz Protocol C Headers for Implementation\n"
                            "//                  Generated by SubGhz Toolkit\n"
                            "//                 RocketGod | betaskynet.com\n"
                            "// ==============================================================\n\n");

        size_t protocol_count = subghz_protocol_registry_count(app->protocol_registry);

        for (size_t i = 0; i < protocol_count; i++)
        {
            const SubGhzProtocol *protocol = subghz_protocol_registry_get_by_index(app->protocol_registry, i);
            if (!protocol || !protocol->name)
                continue;

            subghz_toolkit_generate_protocol_c_header(stream, protocol);
            stream_write_format(stream, "\n");
        }

        success = true;
    } while (0);

    stream_free(stream);
    furi_record_close(RECORD_STORAGE);

    if (success)
    {
        popup_set_header(app->popup, "Success!", 64, 10, AlignCenter, AlignTop);
        popup_set_text(app->popup, "C headers generated to:\n/ext/subghz/analysis/protocol_headers.h",
                       64, 20, AlignCenter, AlignTop);
    }
    else
    {
        popup_set_header(app->popup, "Error!", 64, 10, AlignCenter, AlignTop);
        popup_set_text(app->popup, "Failed to generate C headers", 64, 20, AlignCenter, AlignTop);
    }

    popup_set_callback(app->popup, subghz_toolkit_popup_callback);
    popup_set_context(app->popup, app);
    popup_set_timeout(app->popup, 3000);
    popup_enable_timeout(app->popup);
    view_dispatcher_switch_to_view(app->view_dispatcher, SubGhzToolkitViewPopup);
}

static SubGhzToolkitApp *subghz_toolkit_app_alloc()
{
    SubGhzToolkitApp *app = malloc(sizeof(SubGhzToolkitApp));

    app->view_dispatcher = view_dispatcher_alloc();
    app->gui = furi_record_open(RECORD_GUI);
    view_dispatcher_attach_to_gui(app->view_dispatcher, app->gui, ViewDispatcherTypeFullscreen);

    app->notifications = furi_record_open(RECORD_NOTIFICATION);

    app->submenu = submenu_alloc();
    app->popup = popup_alloc();
    app->text_box = text_box_alloc();
    app->loading = loading_alloc();

    app->text_buffer = furi_string_alloc();
    furi_string_reserve(app->text_buffer, TEXT_BUFFER_SIZE);

    app->environment = subghz_environment_alloc();
    subghz_environment_load_keystore(app->environment, EXT_PATH("subghz/assets/keeloq_mfcodes"));
    subghz_environment_load_keystore(app->environment, EXT_PATH("subghz/assets/keeloq_mfcodes_user"));
    subghz_environment_set_protocol_registry(app->environment, (void *)&subghz_protocol_registry);

    app->protocol_registry = subghz_environment_get_protocol_registry(app->environment);
    app->receiver = subghz_receiver_alloc_init(app->environment);
    app->setting = subghz_setting_alloc();
    subghz_setting_load(app->setting, EXT_PATH("subghz/assets/setting_user"));

    view_set_previous_callback(
        submenu_get_view(app->submenu),
        subghz_toolkit_exit_callback);

    view_set_previous_callback(
        popup_get_view(app->popup),
        subghz_toolkit_exit_to_submenu_callback);

    view_set_previous_callback(
        text_box_get_view(app->text_box),
        subghz_toolkit_exit_to_submenu_callback);

    view_set_previous_callback(
        loading_get_view(app->loading),
        subghz_toolkit_exit_to_submenu_callback);

    view_dispatcher_add_view(app->view_dispatcher, SubGhzToolkitViewSubmenu, submenu_get_view(app->submenu));
    view_dispatcher_add_view(app->view_dispatcher, SubGhzToolkitViewPopup, popup_get_view(app->popup));
    view_dispatcher_add_view(app->view_dispatcher, SubGhzToolkitViewTextBox, text_box_get_view(app->text_box));
    view_dispatcher_add_view(app->view_dispatcher, SubGhzToolkitViewLoading, loading_get_view(app->loading));

    submenu_add_item(
        app->submenu,
        "Decrypt Keeloq mfcodes",
        SubGhzToolkitSubmenuIndexDecryptKeeloq,
        subghz_toolkit_submenu_callback,
        app);

    submenu_add_item(
        app->submenu,
        "List SubGhz Protocols",
        SubGhzToolkitSubmenuIndexListProtocols,
        subghz_toolkit_submenu_callback,
        app);

    submenu_add_item(
        app->submenu,
        "Export All Protocol Info",
        SubGhzToolkitSubmenuIndexExportProtocolInfo,
        subghz_toolkit_submenu_callback,
        app);

    submenu_add_item(
        app->submenu,
        "Advanced Analysis",
        SubGhzToolkitSubmenuIndexAdvancedAnalysis,
        subghz_toolkit_submenu_callback,
        app);

    submenu_add_item(
        app->submenu,
        "Function Disassembly",
        SubGhzToolkitSubmenuIndexFunctionDisassembly,
        subghz_toolkit_submenu_callback,
        app);

    submenu_add_item(
        app->submenu,
        "Protocol State Analysis",
        SubGhzToolkitSubmenuIndexProtocolStateAnalysis,
        subghz_toolkit_submenu_callback,
        app);

    submenu_add_item(
        app->submenu,
        "Signal Capture Analysis",
        SubGhzToolkitSubmenuIndexSignalCapture,
        subghz_toolkit_submenu_callback,
        app);

    submenu_add_item(
        app->submenu,
        "Timing Analysis",
        SubGhzToolkitSubmenuIndexTimingAnalysis,
        subghz_toolkit_submenu_callback,
        app);

    submenu_add_item(
        app->submenu,
        "Generate C Headers",
        SubGhzToolkitSubmenuIndexCHeaderGeneration,
        subghz_toolkit_submenu_callback,
        app);

    submenu_add_item(
        app->submenu,
        "About",
        SubGhzToolkitSubmenuIndexAbout,
        subghz_toolkit_submenu_callback,
        app);

    return app;
}

static void subghz_toolkit_app_free(SubGhzToolkitApp *app)
{
    view_dispatcher_remove_view(app->view_dispatcher, SubGhzToolkitViewSubmenu);
    view_dispatcher_remove_view(app->view_dispatcher, SubGhzToolkitViewPopup);
    view_dispatcher_remove_view(app->view_dispatcher, SubGhzToolkitViewTextBox);
    view_dispatcher_remove_view(app->view_dispatcher, SubGhzToolkitViewLoading);

    submenu_free(app->submenu);
    popup_free(app->popup);
    text_box_free(app->text_box);
    loading_free(app->loading);

    subghz_receiver_free(app->receiver);
    subghz_environment_free(app->environment);
    subghz_setting_free(app->setting);

    furi_string_free(app->text_buffer);

    view_dispatcher_free(app->view_dispatcher);

    furi_record_close(RECORD_GUI);
    furi_record_close(RECORD_NOTIFICATION);

    free(app);
}

int32_t subghz_toolkit_app(void *p)
{
    UNUSED(p);
    SubGhzToolkitApp *app = subghz_toolkit_app_alloc();

    subghz_toolkit_show_intro_popup(app);

    view_dispatcher_run(app->view_dispatcher);

    subghz_toolkit_app_free(app);
    return 0;
}