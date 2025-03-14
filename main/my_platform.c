// Example file - Public Domain
// Need help? https://tinyurl.com/bluepad32-help

#include <string.h>

#include <uni.h>

// Custom "instance"
typedef struct my_platform_instance_s {
    uni_gamepad_seat_t gamepad_seat;  // which "seat" is being used
} my_platform_instance_t;

// Declarations
static void trigger_event_on_gamepad(uni_hid_device_t* d);
static my_platform_instance_t* get_my_platform_instance(uni_hid_device_t* d);

//
// Platform Overrides
//
static void my_platform_init(int argc, const char** argv) {
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    logi("custom: my_platform_init()\n");

#if 0
    uni_gamepad_mappings_t mappings = GAMEPAD_DEFAULT_MAPPINGS;

    // Inverted axis with inverted Y in RY.
    mappings.axis_x = UNI_GAMEPAD_MAPPINGS_AXIS_RX;
    mappings.axis_y = UNI_GAMEPAD_MAPPINGS_AXIS_RY;
    mappings.axis_ry_inverted = true;
    mappings.axis_rx = UNI_GAMEPAD_MAPPINGS_AXIS_X;
    mappings.axis_ry = UNI_GAMEPAD_MAPPINGS_AXIS_Y;

    // Invert A & B
    mappings.button_a = UNI_GAMEPAD_MAPPINGS_BUTTON_B;
    mappings.button_b = UNI_GAMEPAD_MAPPINGS_BUTTON_A;

    uni_gamepad_set_mappings(&mappings);
#endif
    //    uni_bt_service_set_enabled(true);
}

static void my_platform_on_init_complete(void) {
    logi("custom: on_init_complete()\n");

    // Safe to call "unsafe" functions since they are called from BT thread

    // Start scanning
    uni_bt_start_scanning_and_autoconnect_unsafe();
    uni_bt_allow_incoming_connections(true);

    // Based on runtime condition, you can delete or list the stored BT keys.
    if (1)
        uni_bt_del_keys_unsafe();
    else
        uni_bt_list_keys_unsafe();
}

static uni_error_t my_platform_on_device_discovered(bd_addr_t addr, const char* name, uint16_t cod, uint8_t rssi) {
    // You can filter discovered devices here.
    // Just return any value different from UNI_ERROR_SUCCESS;
    // @param addr: the Bluetooth address
    // @param name: could be NULL, could be zero-length, or might contain the name.
    // @param cod: Class of Device. See "uni_bt_defines.h" for possible values.
    // @param rssi: Received Signal Strength Indicator (RSSI) measured in dBms. The higher (255) the better.

    // As an example, if you want to filter out keyboards, do:
    if (((cod & UNI_BT_COD_MINOR_MASK) & UNI_BT_COD_MINOR_KEYBOARD) == UNI_BT_COD_MINOR_KEYBOARD) {
        logi("Ignoring keyboard\n");
        return UNI_ERROR_IGNORE_DEVICE;
    }

    return UNI_ERROR_SUCCESS;
}

static void my_platform_on_device_connected(uni_hid_device_t* d) {
    logi("custom: device connected: %p\n", d);
}

static void my_platform_on_device_disconnected(uni_hid_device_t* d) {
    logi("custom: device disconnected: %p\n", d);
}

static uni_error_t my_platform_on_device_ready(uni_hid_device_t* d) {
    logi("custom: device ready: %p\n", d);
    my_platform_instance_t* ins = get_my_platform_instance(d);
    ins->gamepad_seat = GAMEPAD_SEAT_A;

    trigger_event_on_gamepad(d);
    return UNI_ERROR_SUCCESS;
}

void dumpGamepad(const uni_gamepad_t* gp) {
    logi(
        "dpad: 0x%02x, buttons: 0x%04x, axis L: %4d, %4d, axis R: %4d, %4d, brake: %4d, throttle: %4d, "
        "misc: 0x%02x, gyro x:%6d y:%6d z:%6d, accel x:%6d y:%6d z:%6d\n",
        gp->dpad,         // D-pad
        gp->buttons,      // bitmask of pressed buttons  
        gp->axis_x,        // (-511 - 512) left X Axis
        gp->axis_y,        // (-511 - 512) left Y axis
        gp->axis_rx,       // (-511 - 512) right X axis
        gp->axis_ry,       // (-511 - 512) right Y axis
        gp->brake,        // (0 - 1023): brake button
        gp->throttle,     // (0 - 1023): throttle (AKA gas) button
        gp->misc_buttons,  // bitmask of pressed "misc" buttons
        gp->gyro[0],        // Gyro X
        gp->gyro[1],        // Gyro Y
        gp->gyro[2],        // Gyro Z
        gp->accel[0],       // Accelerometer X
        gp->accel[1],       // Accelerometer Y
        gp->accel[2]        // Accelerometer Z
    );
}

static void my_platform_on_controller_data(uni_hid_device_t* d, uni_controller_t* ctl) {
    static uint8_t leds = 0;
    static uint8_t enabled = true;
    static uni_controller_t prev = {0};
    uni_gamepad_t* gp;

    // Optimization to avoid processing the previous data so that the console
    // does not get spammed with a lot of logs, but remove it from your project.
    if (memcmp(&prev, ctl, sizeof(*ctl)) == 0) {
        return;
    }
    prev = *ctl;
    // Print device Id before dumping gamepad.
    // This could be very CPU intensive and might crash the ESP32.
    // Remove these 2 lines in production code.
    //    logi("(%p), id=%d, \n", d, uni_hid_device_get_idx_for_instance(d));
    //    uni_controller_dump(ctl);

    switch (ctl->klass) {
        case UNI_CONTROLLER_CLASS_GAMEPAD:
            gp = &ctl->gamepad;

            dumpGamepad(gp);
            
            // // Debugging
            // // Axis ry: control rumble
            // if ((gp->buttons & BUTTON_A) && d->report_parser.play_dual_rumble != NULL) {
            //     d->report_parser.play_dual_rumble(d, 0 /* delayed start ms */, 250 /* duration ms */,
            //                                       255 /* weak magnitude */, 0 /* strong magnitude */);
            // }
            // // Buttons: Control LEDs On/Off
            // if ((gp->buttons & BUTTON_B) && d->report_parser.set_player_leds != NULL) {
            //     d->report_parser.set_player_leds(d, leds++ & 0x0f);
            // }
            // // Axis: control RGB color
            // if ((gp->buttons & BUTTON_X) && d->report_parser.set_lightbar_color != NULL) {
            //     uint8_t r = (gp->axis_x * 256) / 512;
            //     uint8_t g = (gp->axis_y * 256) / 512;
            //     uint8_t b = (gp->axis_rx * 256) / 512;
            //     d->report_parser.set_lightbar_color(d, r, g, b);
            // }

            // // Toggle Bluetooth connections
            // if ((gp->buttons & BUTTON_SHOULDER_L) && enabled) {
            //     logi("*** Stop scanning\n");
            //     uni_bt_stop_scanning_safe();
            //     enabled = false;
            // }
            // if ((gp->buttons & BUTTON_SHOULDER_R) && !enabled) {
            //     logi("*** Start scanning\n");
            //     uni_bt_start_scanning_and_autoconnect_safe();
            //     enabled = true;
            // }
            break;
        default:
            break;
    }
}

static const uni_property_t* my_platform_get_property(uni_property_idx_t idx) {
    ARG_UNUSED(idx);
    return NULL;
}

static void my_platform_on_oob_event(uni_platform_oob_event_t event, void* data) {
    // switch (event) {
    //     case UNI_PLATFORM_OOB_GAMEPAD_SYSTEM_BUTTON: {
    //         uni_hid_device_t* d = data;

    //         if (d == NULL) {
    //             loge("ERROR: my_platform_on_oob_event: Invalid NULL device\n");
    //             return;
    //         }
    //         logi("custom: on_device_oob_event(): %d\n", event);

    //         my_platform_instance_t* ins = get_my_platform_instance(d);
    //         ins->gamepad_seat = ins->gamepad_seat == GAMEPAD_SEAT_A ? GAMEPAD_SEAT_B : GAMEPAD_SEAT_A;

    //         trigger_event_on_gamepad(d);
    //         break;
    //     }

    //     case UNI_PLATFORM_OOB_BLUETOOTH_ENABLED:
    //         logi("custom: Bluetooth enabled: %d\n", (bool)(data));
    //         break;

    //     default:
    //         logi("my_platform_on_oob_event: unsupported event: 0x%04x\n", event);
    //         break;
    // }
}

//
// Helpers
//
static my_platform_instance_t* get_my_platform_instance(uni_hid_device_t* d) {
    return (my_platform_instance_t*)&d->platform_data[0];
}

static void trigger_event_on_gamepad(uni_hid_device_t* d) {
    my_platform_instance_t* ins = get_my_platform_instance(d);

    if (d->report_parser.play_dual_rumble != NULL) {
        d->report_parser.play_dual_rumble(d, 0 /* delayed start ms */, 150 /* duration ms */, 128 /* weak magnitude */,
                                          40 /* strong magnitude */);
    }

    if (d->report_parser.set_player_leds != NULL) {
        d->report_parser.set_player_leds(d, ins->gamepad_seat);
    }

    if (d->report_parser.set_lightbar_color != NULL) {
        uint8_t red = (ins->gamepad_seat & 0x01) ? 0xff : 0;
        uint8_t green = (ins->gamepad_seat & 0x02) ? 0xff : 0;
        uint8_t blue = (ins->gamepad_seat & 0x04) ? 0xff : 0;
        d->report_parser.set_lightbar_color(d, red, green, blue);
    }
}

//
// Entry Point
//
struct uni_platform* get_my_platform(void) {
    static struct uni_platform plat = {
        .name = "custom",
        .init = my_platform_init,
        .on_init_complete = my_platform_on_init_complete,
        .on_device_discovered = my_platform_on_device_discovered,
        .on_device_connected = my_platform_on_device_connected,
        .on_device_disconnected = my_platform_on_device_disconnected,
        .on_device_ready = my_platform_on_device_ready,
        .on_oob_event = my_platform_on_oob_event,
        .on_controller_data = my_platform_on_controller_data,
        .get_property = my_platform_get_property,
    };

    return &plat;
}
