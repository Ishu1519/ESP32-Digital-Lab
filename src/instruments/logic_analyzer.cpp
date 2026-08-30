#include "instruments/logic_analyzer.h"
#include <Arduino.h>
#include "esp_log.h"
#include "driver/uart.h"
#include "system_config.h"

static const char *TAG = "INST_LA";

// Helper for Base64 encoding captured DMA samples
static const char b64_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static String base64_encode_bytes(const uint8_t *data, size_t len) {
    String out = "";
    out.reserve(((len + 2) / 3) * 4);
    for (size_t i = 0; i < len; i += 3) {
        uint32_t b = (data[i] << 16) | ((i + 1 < len ? data[i + 1] : 0) << 8) | (i + 2 < len ? data[i + 2] : 0);
        out += b64_table[(b >> 18) & 0x3F];
        out += b64_table[(b >> 12) & 0x3F];
        out += (i + 1 < len) ? b64_table[(b >> 6) & 0x3F] : '=';
        out += (i + 2 < len) ? b64_table[b & 0x3F] : '=';
    }
    return out;
}

LogicAnalyzerInstrument::LogicAnalyzerInstrument()
    : m_running(false),
      m_sample_rate_hz(1000000), // Default 1 MHz
      m_sample_depth(2048),      // Default 2048 samples
      m_num_channels(4),
      m_trigger_channel(0),
      m_trigger_mode(LA_TRIGGER_NONE),
      m_last_capture_buf(NULL),
      m_last_capture_len(0),
      m_capture_seq(0),
      m_continuous_mode(true),
      m_gen_freq_hz(10000),
      m_gen_duty_pct(50),
      m_gen_enabled(true) {
    
    // Default channel pin map (CH0 on GPIO 18 connects to existing test loopback from GPIO 19)
    m_channel_pins[0] = 18; // CH0
    m_channel_pins[1] = 12; // CH1
    m_channel_pins[2] = 13; // CH2
    m_channel_pins[3] = 14; // CH3
    m_channel_pins[4] = 25;
    m_channel_pins[5] = 26;
    m_channel_pins[6] = 27;
    m_channel_pins[7] = 32;
}

LogicAnalyzerInstrument::~LogicAnalyzerInstrument() {
    deinit();
}

esp_err_t LogicAnalyzerInstrument::init() {
    la_config_t config = {
        .sample_rate_hz = m_sample_rate_hz,
        .sample_depth = m_sample_depth,
        .num_channels = m_num_channels,
        .trigger_channel = m_trigger_channel,
        .trigger_mode = m_trigger_mode,
        .channel_pins = m_channel_pins
    };

    esp_err_t err = hal_i2s_la_init(&config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "hal_i2s_la_init failed: %s", esp_err_to_name(err));
        return err;
    }

    // Initialize test signal generator on GPIO 19
    hal_ledc_gen_init(PIN_REF_GEN_OUTPUT);
    hal_ledc_gen_set_frequency(m_gen_freq_hz, m_gen_duty_pct);
    if (m_gen_enabled) {
        hal_ledc_gen_start();
    }

    if (m_last_capture_buf) free(m_last_capture_buf);
    m_last_capture_buf = (uint8_t *)malloc(m_sample_depth);
    m_last_capture_len = 0;

    return ESP_OK;
}

esp_err_t LogicAnalyzerInstrument::start() {
    m_running = true;
    return ESP_OK;
}

esp_err_t LogicAnalyzerInstrument::stop() {
    hal_i2s_la_stop();
    m_running = false;
    return ESP_OK;
}

esp_err_t LogicAnalyzerInstrument::deinit() {
    stop();
    hal_i2s_la_deinit();
    hal_ledc_gen_deinit();
    if (m_last_capture_buf) {
        free(m_last_capture_buf);
        m_last_capture_buf = NULL;
    }
    return ESP_OK;
}

uint32_t LogicAnalyzerInstrument::getRequiredPeripherals() const {
    return PERIPH_I2S0 | PERIPH_LEDC0;
}

uint64_t LogicAnalyzerInstrument::getRequiredPinsMask() const {
    uint64_t mask = (1ULL << PIN_REF_GEN_OUTPUT);
    for (int i = 0; i < m_num_channels; i++) {
        mask |= (1ULL << m_channel_pins[i]);
    }
    return mask;
}

esp_err_t LogicAnalyzerInstrument::captureTriggered() {
    la_capture_result_t result = {0};
    esp_err_t err = hal_i2s_la_capture_sync(500, &result);
    if (err == ESP_OK && result.buffer && result.buffer_len > 0) {
        if (m_last_capture_buf && result.buffer_len <= m_sample_depth) {
            memcpy(m_last_capture_buf, result.buffer, result.buffer_len);
            m_last_capture_len = result.buffer_len;
            m_capture_seq++;
        }
    } else {
        ESP_LOGW(TAG, "hal_i2s_la_capture_sync returned %s", esp_err_to_name(err));
    }
    return err;
}

void LogicAnalyzerInstrument::getTelemetryJson(JsonObject &root) {
    if (m_running && m_continuous_mode) {
        captureTriggered();
    }

    root["sample_rate_hz"] = m_sample_rate_hz;
    root["sample_depth"] = m_sample_depth;
    root["num_channels"] = m_num_channels;
    root["capture_seq"] = m_capture_seq;
    root["running"] = m_running;

    JsonArray pins = root.createNestedArray("channels");
    for (int i = 0; i < m_num_channels; i++) {
        pins.add(m_channel_pins[i]);
    }

    // Telemetry payload includes Base64 encoded raw samples for zero-latency rendering
    if (m_last_capture_len > 0 && m_last_capture_buf) {
        root["sample_count"] = m_last_capture_len;
        root["data_b64"] = base64_encode_bytes(m_last_capture_buf, m_last_capture_len);
    } else {
        root["sample_count"] = 0;
        root["data_b64"] = "";
    }

    // Generator status
    ledc_gen_status_t gen = hal_ledc_gen_get_status();
    JsonObject genObj = root.createNestedObject("ref_gen");
    genObj["enabled"] = gen.enabled;
    genObj["freq_hz"] = gen.frequency_hz;
    genObj["duty_pct"] = gen.duty_cycle_pct;
    genObj["gpio"] = gen.output_gpio;
}

esp_err_t LogicAnalyzerInstrument::handleCommand(const JsonObject &cmd) {
    const char *action = cmd["action"] | cmd["cmd"];
    if (!action) return ESP_ERR_INVALID_ARG;

    if (strcmp(action, "capture") == 0 || strcmp(action, "single") == 0) {
        return captureTriggered();
    } else if (strcmp(action, "set_config") == 0 || strcmp(action, "configure") == 0) {
        if (cmd.containsKey("sample_rate_hz")) m_sample_rate_hz = cmd["sample_rate_hz"];
        if (cmd.containsKey("sample_depth")) m_sample_depth = cmd["sample_depth"];
        if (cmd.containsKey("num_channels")) m_num_channels = cmd["num_channels"];
        if (cmd.containsKey("continuous")) m_continuous_mode = cmd["continuous"];
        if (cmd.containsKey("trigger_channel")) m_trigger_channel = cmd["trigger_channel"];
        if (cmd.containsKey("trigger_mode")) m_trigger_mode = (la_trigger_mode_t)(int)cmd["trigger_mode"];

        la_config_t config = {
            .sample_rate_hz = m_sample_rate_hz,
            .sample_depth = m_sample_depth,
            .num_channels = m_num_channels,
            .trigger_channel = m_trigger_channel,
            .trigger_mode = m_trigger_mode,
            .channel_pins = m_channel_pins
        };
        esp_err_t err = hal_i2s_la_init(&config);
        if (m_last_capture_buf) free(m_last_capture_buf);
        m_last_capture_buf = (uint8_t *)malloc(m_sample_depth);
        m_last_capture_len = 0;
        return err;
    } else if (strcmp(action, "set_ref_gen") == 0) {
        uint32_t freq = cmd["freq_hz"] | 10000;
        uint8_t duty = cmd["duty_pct"] | 50;
        bool enabled = cmd.containsKey("enabled") ? (bool)cmd["enabled"] : true;

        m_gen_freq_hz = freq;
        m_gen_duty_pct = duty;
        m_gen_enabled = enabled;

        hal_ledc_gen_set_frequency(freq, duty);
        if (enabled) {
            hal_ledc_gen_start();
        } else {
            hal_ledc_gen_stop();
        }
        return ESP_OK;
    } else if (strcmp(action, "set_pins") == 0) {
        if (cmd.containsKey("pins")) {
            JsonArray arr = cmd["pins"].as<JsonArray>();
            int idx = 0;
            for (JsonVariant v : arr) {
                if (idx < 8) {
                    m_channel_pins[idx++] = v.as<int>();
                }
            }
            m_num_channels = idx;
            la_config_t config = {
                .sample_rate_hz = m_sample_rate_hz,
                .sample_depth = m_sample_depth,
                .num_channels = m_num_channels,
                .trigger_channel = m_trigger_channel,
                .trigger_mode = m_trigger_mode,
                .channel_pins = m_channel_pins
            };
            return hal_i2s_la_init(&config);
        }
        return ESP_ERR_INVALID_ARG;
    } else if (strcmp(action, "start_quad_gen") == 0) {
        uint32_t f0 = cmd["f0"] | 10000;
        uint32_t f1 = cmd["f1"] | 20000;
        uint32_t f2 = cmd["f2"] | 50000;
        uint32_t f3 = cmd["f3"] | 100000;
        return hal_ledc_gen_start_quad(f0, f1, f2, f3);
    } else if (strcmp(action, "stop_quad_gen") == 0) {
        return hal_ledc_gen_stop_quad();
    } else if (strcmp(action, "send_uart_test") == 0) {
        int baud = cmd["baud_rate"] | 115200;
        const char *text = cmd["text"] | "Hello ESP32 UART!\r\n";

        // Stop LEDC reference generator to free GPIO 19
        hal_ledc_gen_stop();

        uart_config_t uart_config = {
            .baud_rate = baud,
            .data_bits = UART_DATA_8_BITS,
            .parity    = UART_PARITY_DISABLE,
            .stop_bits = UART_STOP_BITS_1,
            .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
            .source_clk = UART_SCLK_APB,
        };
        uart_param_config(UART_NUM_1, &uart_config);
        uart_set_pin(UART_NUM_1, 19, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
        uart_driver_install(UART_NUM_1, 256, 0, 0, NULL, 0);
        esp_rom_delay_us(200); // Allow TX line to settle HIGH before transmitting

        uart_tx_chars(UART_NUM_1, text, strlen(text));
        captureTriggered(); // Capture in-flight transmission
        uart_wait_tx_done(UART_NUM_1, pdMS_TO_TICKS(100));

        uart_driver_delete(UART_NUM_1);
        m_continuous_mode = false; // Hold buffer for telemetry reading
        return ESP_OK;
    }

    return ESP_ERR_NOT_SUPPORTED;
}
