#include <pebble.h>
#include <ctype.h>
#include <limits.h>
#include <stdlib.h>
#include "message_keys.auto.h"

static Window *s_window;
static TextLayer *s_date_layer;
static TextLayer *s_time_layer;
static Layer *s_line_layer;
static Layer *s_corner_line_layer;
static Layer *s_matrix_layer;
static Layer *s_battery_layer;
static BitmapLayer *s_weather_icon_layer;
static TextLayer *s_weather_temp_layer;
static GBitmap *s_weather_icon_bitmap;
static GFont s_date_font;
static GFont s_time_font;
static BatteryChargeState s_battery_state;
static GColor s_foreground_color;
static GColor s_background_color;
static int s_theme;
static bool s_bt_connected = false;
static bool s_weather_enabled = true;
static bool s_weather_show_temp = true;
static uint8_t s_weather_unit = 0;
static int16_t s_weather_temp = INT16_MAX;
static uint8_t s_weather_code = 255;
static time_t s_last_weather = 0;
static uint8_t s_weather_retry_count = 0;

enum {
  THEME_LIGHT = 0,
  THEME_DARK = 1,
  THEME_COLOR = 2,
};

static const int DEFAULT_THEME = THEME_LIGHT;

enum {
  PERSIST_KEY_THEME = 1,
  PERSIST_KEY_WEATHER_ENABLED = 2,
  PERSIST_KEY_WEATHER_SHOW_TEMP = 3,
  PERSIST_KEY_WEATHER_UNIT = 4,
  PERSIST_KEY_WEATHER_TEMP = 5,
  PERSIST_KEY_WEATHER_CODE = 6,
};

static void apply_theme(void);
static void update_weather_visibility(void);
static void update_weather_icon(void);
static void update_weather_temp_text(void);
static void update_weather_layout(void);
static void request_weather(void);
static bool tuple_value_to_bool(const Tuple *tuple, bool fallback);
static void send_settings_to_phone(void);
static void connection_handler(bool connected);
static const uint8_t s_matrix[32][31] = {
  {0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0},
  {0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0},
  {0, 0, 0, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 0, 0, 0},
  {0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0},
  {0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0},
  {0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0},
  {0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0},
  {0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0},
  {0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0},
  {0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0},
  {0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0},
  {0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0},
  {0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0},
  {0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0},
  {0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0},
  {0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0},
  {0, 0, 0, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 0, 0, 0},
  {0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0},
  {0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0},
  {0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0},
  {0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0},
  {0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 0, 0, 0},
  {0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0},
  {0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0},
  {0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0},
  {0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0},
  {0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0},
  {0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0},
  {0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0},
  {0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0},
  {0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0},
};

static const uint8_t s_color_matrix[32][25] = {
  {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
  {1, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 1},
  {1, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 1},
  {1, 6, 6, 6, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 6, 6, 6, 1},
  {1, 6, 6, 4, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 8, 6, 6, 1},
  {1, 6, 6, 4, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 8, 6, 6, 1},
  {1, 6, 6, 4, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 8, 6, 6, 1},
  {1, 6, 6, 4, 7, 7, 7, 7, 2, 7, 7, 7, 2, 7, 7, 7, 2, 7, 7, 7, 7, 8, 6, 6, 1},
  {1, 6, 6, 4, 7, 7, 7, 7, 2, 7, 7, 7, 2, 7, 7, 7, 2, 7, 7, 7, 7, 8, 6, 6, 1},
  {1, 6, 6, 4, 7, 7, 7, 7, 7, 7, 7, 7, 2, 7, 7, 7, 7, 7, 7, 7, 7, 8, 6, 6, 1},
  {1, 6, 6, 4, 7, 7, 7, 7, 7, 7, 7, 7, 2, 7, 7, 7, 7, 7, 7, 7, 7, 8, 6, 6, 1},
  {1, 6, 6, 4, 7, 7, 7, 7, 7, 7, 7, 2, 2, 7, 7, 7, 7, 7, 7, 7, 7, 8, 6, 6, 1},
  {1, 6, 6, 4, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 8, 6, 6, 1},
  {1, 6, 6, 4, 7, 7, 7, 7, 7, 2, 7, 7, 7, 7, 2, 7, 7, 7, 7, 7, 7, 8, 6, 6, 1},
  {1, 6, 6, 4, 7, 7, 7, 7, 7, 7, 2, 2, 2, 2, 7, 7, 7, 7, 7, 7, 7, 8, 6, 6, 1},
  {1, 6, 6, 4, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 8, 6, 6, 1},
  {1, 6, 6, 4, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 8, 6, 6, 1},
  {1, 6, 6, 6, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 6, 6, 6, 1},
  {1, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 1},
  {1, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 1},
  {1, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 1},
  {1, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 1},
  {1, 6, 6, 5, 5, 6, 6, 6, 6, 6, 6, 6, 6, 2, 2, 2, 2, 2, 2, 2, 2, 6, 6, 6, 1},
  {1, 6, 6, 3, 3, 6, 6, 6, 6, 6, 6, 6, 6, 8, 8, 8, 8, 8, 8, 8, 8, 6, 6, 6, 1},
  {1, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 1},
  {1, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 1},
  {1, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 1},
  {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0},
  {0, 1, 6, 6, 6, 6, 6, 4, 4, 4, 4, 4, 4, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0},
  {0, 1, 6, 6, 6, 6, 6, 6, 6, 6, 4, 4, 4, 4, 4, 4, 1, 1, 1, 1, 1, 1, 1, 1, 0},
  {0, 1, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 4, 4, 4, 4, 4, 4, 1, 1, 1, 1, 1, 0},
  {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0},
};

#define WEATHER_INTERVAL (30 * 60)
#define WEATHER_ICON_SIZE 17

static const uint32_t s_weather_icon_light_resources[9] = {
  RESOURCE_ID_WEATHER_LIGHT_1,
  RESOURCE_ID_WEATHER_LIGHT_2,
  RESOURCE_ID_WEATHER_LIGHT_3,
  RESOURCE_ID_WEATHER_LIGHT_4,
  RESOURCE_ID_WEATHER_LIGHT_5,
  RESOURCE_ID_WEATHER_LIGHT_6,
  RESOURCE_ID_WEATHER_LIGHT_7,
  RESOURCE_ID_WEATHER_LIGHT_8,
  RESOURCE_ID_WEATHER_LIGHT_9
};

static const uint32_t s_weather_icon_dark_resources[9] = {
  RESOURCE_ID_WEATHER_DARK_1,
  RESOURCE_ID_WEATHER_DARK_2,
  RESOURCE_ID_WEATHER_DARK_3,
  RESOURCE_ID_WEATHER_DARK_4,
  RESOURCE_ID_WEATHER_DARK_5,
  RESOURCE_ID_WEATHER_DARK_6,
  RESOURCE_ID_WEATHER_DARK_7,
  RESOURCE_ID_WEATHER_DARK_8,
  RESOURCE_ID_WEATHER_DARK_9
};

static int weather_icon_index_from_code(uint8_t code) {
  switch (code) {
    case 0:
    case 1:
      return 1;
    case 2:
      return 2;
    case 3:
      return 3;
    case 45:
    case 48:
      return 4;
    case 51:
    case 53:
    case 55:
    case 56:
    case 57:
    case 61:
    case 80:
      return 5;
    case 63:
    case 81:
      return 6;
    case 65:
    case 66:
    case 67:
    case 82:
      return 7;
    case 71:
    case 73:
    case 75:
    case 77:
    case 85:
    case 86:
      return 8;
    case 95:
    case 96:
    case 99:
      return 9;
    default:
      return 1;
  }
}

static void update_time() {
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);

  static char s_buffer[16];
  static char s_time_buffer[8];
#ifdef PBL_ROUND
  strftime(s_buffer, sizeof(s_buffer), "%b %d", tick_time);
#else
  strftime(s_buffer, sizeof(s_buffer), "%b %d %a", tick_time);
#endif
  strftime(s_time_buffer, sizeof(s_time_buffer), "%H:%M", tick_time);

  for (char *p = s_buffer; *p; ++p) {
    *p = toupper((unsigned char)*p);
  }

  text_layer_set_text(s_date_layer, s_buffer);
  text_layer_set_text(s_time_layer, s_time_buffer);
}

static GColor color_from_index(uint8_t index) {
  switch (index) {
    case 1:
      return GColorBlack;
    case 2:
      return GColorOxfordBlue;
    case 3:
      return GColorRed;
    case 4:
      return GColorDarkGray;
    case 5:
      return GColorIslamicGreen;
    case 6:
      return GColorLightGray;
    case 7:
      return GColorBabyBlueEyes;
    case 8:
      return GColorWhite;
    default:
      return GColorBlack;
  }
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
  request_weather();
}

static void line_layer_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  graphics_context_set_fill_color(ctx, s_foreground_color);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);
}

static void matrix_layer_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  const int pixel_size = bounds.size.w < 190 ? 2 : 3;
  const int matrix_cols = (s_theme == THEME_COLOR) ? 25 : 31;
  const int matrix_width = matrix_cols * pixel_size;
  const int matrix_height = 32 * pixel_size;
  const int origin_x = (bounds.size.w - matrix_width) / 2;
  const int origin_y = (bounds.size.h * 2 / 5) - (matrix_height / 2);

  for (int row = 0; row < 32; ++row) {
    for (int col = 0; col < matrix_cols; ++col) {
      if (s_theme == THEME_COLOR) {
        const uint8_t color_index = s_color_matrix[row][col];
        if (color_index == 0) {
          continue;
        }
        graphics_context_set_fill_color(ctx, color_from_index(color_index));
        graphics_fill_rect(ctx,
                           GRect(origin_x + col * pixel_size, origin_y + row * pixel_size,
                                 pixel_size, pixel_size),
                           0, GCornerNone);
      } else if (s_matrix[row][col] == 1) {
        graphics_context_set_fill_color(ctx, s_foreground_color);
        graphics_fill_rect(ctx,
                           GRect(origin_x + col * pixel_size, origin_y + row * pixel_size,
                                 pixel_size, pixel_size),
                           0, GCornerNone);
      }
    }
  }
}

static void battery_layer_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  GRect body = GRect(0, 0, bounds.size.w - 3, bounds.size.h);
  GRect nub = GRect(bounds.size.w - 3, (bounds.size.h - 4) / 2, 3, 4);

  graphics_context_set_fill_color(ctx, s_foreground_color);
  graphics_fill_rect(ctx, body, 0, GCornerNone);
  graphics_fill_rect(ctx, nub, 0, GCornerNone);

  const int segment_count = 4;
  const int segment_gap = 1;
  const int inner_width = body.size.w - 4;
  const int inner_height = body.size.h - 4;
  const int segment_width = (inner_width - (segment_gap * (segment_count - 1))) / segment_count;
  const int filled_segments = (s_battery_state.charge_percent * segment_count + 99) / 100;

  graphics_context_set_fill_color(ctx, s_background_color);
  for (int i = 0; i < segment_count; ++i) {
    if (i >= filled_segments) {
      break;
    }
    const int x = body.origin.x + 2 + i * (segment_width + segment_gap);
    const int y = body.origin.y + 2;
    graphics_fill_rect(ctx, GRect(x, y, segment_width, inner_height), 0, GCornerNone);
  }
}

static void battery_handler(BatteryChargeState state) {
  s_battery_state = state;
  if (s_battery_layer) {
    layer_mark_dirty(s_battery_layer);
  }
}

static void connection_handler(bool connected) {
  s_bt_connected = connected;
  if (connected) {
    APP_LOG(APP_LOG_LEVEL_INFO, "bluetooth connected, request weather");
    s_last_weather = 0;
    s_weather_retry_count = 0;
    request_weather();
  } else {
    APP_LOG(APP_LOG_LEVEL_INFO, "bluetooth disconnected");
  }
}

static void apply_theme(void) {
  switch (s_theme) {
    case THEME_DARK:
      s_background_color = GColorBlack;
      s_foreground_color = GColorWhite;
      break;
    case THEME_COLOR:
      s_background_color = GColorWhite;
      s_foreground_color = GColorBlack;
      break;
    case THEME_LIGHT:
    default:
      s_background_color = GColorWhite;
      s_foreground_color = GColorBlack;
      break;
  }

  window_set_background_color(s_window, s_background_color);
  if (s_date_layer) {
    text_layer_set_text_color(s_date_layer, s_foreground_color);
  }
  if (s_time_layer) {
    text_layer_set_text_color(s_time_layer, s_foreground_color);
  }
  if (s_line_layer) {
    layer_mark_dirty(s_line_layer);
  }
  if (s_corner_line_layer) {
    layer_mark_dirty(s_corner_line_layer);
  }
  if (s_matrix_layer) {
    layer_mark_dirty(s_matrix_layer);
  }
  if (s_battery_layer) {
    layer_mark_dirty(s_battery_layer);
  }
  if (s_weather_temp_layer) {
    text_layer_set_text_color(s_weather_temp_layer, s_foreground_color);
  }
  update_weather_icon();
}

static void send_settings_to_phone(void) {
  DictionaryIterator *iter = NULL;
  if (app_message_outbox_begin(&iter) != APP_MSG_OK || !iter) {
    return;
  }
  dict_write_int(iter, MESSAGE_KEY_theme, &s_theme, sizeof(s_theme), true);
  dict_write_uint8(iter, MESSAGE_KEY_WEATHER_ENABLED, s_weather_enabled ? 1 : 0);
  dict_write_uint8(iter, MESSAGE_KEY_WEATHER_SHOW_TEMP, s_weather_show_temp ? 1 : 0);
  dict_write_uint8(iter, MESSAGE_KEY_WEATHER_TEMP_UNIT, s_weather_unit);
  app_message_outbox_send();
}

static void update_weather_visibility(void) {
  if (s_corner_line_layer) {
#ifdef PBL_ROUND
    layer_set_hidden(s_corner_line_layer, true);
#else
    layer_set_hidden(s_corner_line_layer, s_weather_enabled);
#endif
  }
  if (s_weather_icon_layer) {
    layer_set_hidden(bitmap_layer_get_layer(s_weather_icon_layer), !s_weather_enabled);
  }
  if (s_weather_temp_layer) {
    layer_set_hidden(text_layer_get_layer(s_weather_temp_layer),
                     !s_weather_enabled || !s_weather_show_temp);
  }
  update_weather_layout();
}

static void update_weather_layout(void) {
  if (!s_weather_icon_layer || !s_weather_temp_layer) {
    return;
  }

#ifdef PBL_ROUND
  Layer *root_layer = window_get_root_layer(s_window);
  GRect bounds = layer_get_bounds(root_layer);
  const int icon_gap = 2;
  const int temp_width = 50;
  const int temp_height = 16;
  const int icon_y = bounds.size.h - WEATHER_ICON_SIZE - 10;
  const int temp_y = icon_y + ((WEATHER_ICON_SIZE - temp_height) / 2);
  const int center_x = bounds.size.w / 2;

  if (s_weather_enabled && s_weather_show_temp) {
    const int icon_x = center_x - (icon_gap / 2) - WEATHER_ICON_SIZE;
    const int temp_x = center_x + (icon_gap / 2);
    layer_set_frame(bitmap_layer_get_layer(s_weather_icon_layer),
                    GRect(icon_x, icon_y, WEATHER_ICON_SIZE, WEATHER_ICON_SIZE));
    layer_set_frame(text_layer_get_layer(s_weather_temp_layer),
                    GRect(temp_x, temp_y, temp_width, temp_height));
  } else {
    const int icon_x = (bounds.size.w - WEATHER_ICON_SIZE) / 2;
    layer_set_frame(bitmap_layer_get_layer(s_weather_icon_layer),
                    GRect(icon_x, icon_y, WEATHER_ICON_SIZE, WEATHER_ICON_SIZE));
  }
#endif
}

static void update_weather_icon(void) {
  if (!s_weather_icon_layer || !s_weather_enabled || s_weather_code == 255) {
    if (s_weather_icon_layer) {
      bitmap_layer_set_bitmap(s_weather_icon_layer, NULL);
    }
    if (s_weather_icon_bitmap) {
      gbitmap_destroy(s_weather_icon_bitmap);
      s_weather_icon_bitmap = NULL;
    }
    return;
  }

  const int icon_index = weather_icon_index_from_code(s_weather_code);
  const uint32_t *resources = (s_theme == THEME_DARK)
                                  ? s_weather_icon_dark_resources
                                  : s_weather_icon_light_resources;
  const uint32_t resource_id = resources[icon_index - 1];

  if (s_weather_icon_bitmap) {
    gbitmap_destroy(s_weather_icon_bitmap);
    s_weather_icon_bitmap = NULL;
  }
  s_weather_icon_bitmap = gbitmap_create_with_resource(resource_id);
  bitmap_layer_set_bitmap(s_weather_icon_layer, s_weather_icon_bitmap);
}

static void update_weather_temp_text(void) {
  if (!s_weather_temp_layer) {
    return;
  }
  if (!s_weather_enabled || !s_weather_show_temp || s_weather_temp == INT16_MAX) {
    text_layer_set_text(s_weather_temp_layer, "");
    return;
  }

  static char s_temp_buffer[8];
  const char unit_char = s_weather_unit == 1 ? 'F' : 'C';
  snprintf(s_temp_buffer, sizeof(s_temp_buffer), "%d%c", s_weather_temp, unit_char);
  text_layer_set_text(s_weather_temp_layer, s_temp_buffer);
}

static void request_weather(void) {
  if (!s_weather_enabled) {
    APP_LOG(APP_LOG_LEVEL_INFO, "weather request skipped: disabled");
    return;
  }
  if (!s_bt_connected) {
    APP_LOG(APP_LOG_LEVEL_INFO, "weather request skipped: disconnected");
    return;
  }

  const time_t now = time(NULL);
  if (s_last_weather != 0 && s_last_weather + WEATHER_INTERVAL > now) {
    APP_LOG(APP_LOG_LEVEL_INFO, "weather request skipped: throttled");
    return;
  }
  if (s_weather_retry_count >= 10) {
    APP_LOG(APP_LOG_LEVEL_INFO, "weather request skipped: retry limit");
    return;
  }

  DictionaryIterator *iter = NULL;
  if (app_message_outbox_begin(&iter) != APP_MSG_OK || !iter) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "weather request: outbox begin failed");
    return;
  }
  APP_LOG(APP_LOG_LEVEL_INFO, "weather request unit=%u", s_weather_unit);
  dict_write_uint8(iter, MESSAGE_KEY_WEATHER_REQUEST, s_weather_unit);
  app_message_outbox_send();
  s_weather_retry_count++;
}

static bool tuple_value_to_bool(const Tuple *tuple, bool fallback) {
  if (!tuple) {
    return fallback;
  }
  switch (tuple->type) {
    case TUPLE_CSTRING:
      return tuple->value->cstring[0] == '1' ||
             tuple->value->cstring[0] == 't' ||
             tuple->value->cstring[0] == 'T';
    case TUPLE_INT:
      return tuple->value->int32 != 0;
    case TUPLE_UINT:
      return tuple->value->uint32 != 0;
    default:
      return fallback;
  }
}

static void inbox_received_handler(DictionaryIterator *iter, void *context) {
  bool weather_settings_changed = false;
  bool weather_unit_changed = false;
  Tuple *theme_tuple = dict_find(iter, MESSAGE_KEY_theme);
  if (theme_tuple) {
    int new_theme = s_theme;
    if (theme_tuple->type == TUPLE_CSTRING) {
      new_theme = atoi(theme_tuple->value->cstring);
    } else if (theme_tuple->type == TUPLE_UINT) {
      new_theme = (int)theme_tuple->value->uint32;
    } else {
      new_theme = (int)theme_tuple->value->int32;
    }

    if (new_theme >= THEME_LIGHT && new_theme <= THEME_COLOR) {
      s_theme = new_theme;
      persist_write_int(PERSIST_KEY_THEME, s_theme);
      apply_theme();
      send_settings_to_phone();
    }
  }

  Tuple *weather_enabled_tuple = dict_find(iter, MESSAGE_KEY_WEATHER_ENABLED);
  if (weather_enabled_tuple) {
    s_weather_enabled = tuple_value_to_bool(weather_enabled_tuple, s_weather_enabled);
    persist_write_bool(PERSIST_KEY_WEATHER_ENABLED, s_weather_enabled);
    APP_LOG(APP_LOG_LEVEL_INFO, "weather enabled=%d", s_weather_enabled);
    send_settings_to_phone();
    weather_settings_changed = true;
  }

  Tuple *weather_show_temp_tuple = dict_find(iter, MESSAGE_KEY_WEATHER_SHOW_TEMP);
  if (weather_show_temp_tuple) {
    s_weather_show_temp = tuple_value_to_bool(weather_show_temp_tuple, s_weather_show_temp);
    persist_write_bool(PERSIST_KEY_WEATHER_SHOW_TEMP, s_weather_show_temp);
    APP_LOG(APP_LOG_LEVEL_INFO, "weather show_temp=%d", s_weather_show_temp);
    send_settings_to_phone();
    weather_settings_changed = true;
  }

  Tuple *weather_unit_tuple = dict_find(iter, MESSAGE_KEY_WEATHER_TEMP_UNIT);
  if (weather_unit_tuple) {
    uint8_t unit = s_weather_unit;
    if (weather_unit_tuple->type == TUPLE_CSTRING) {
      unit = (weather_unit_tuple->value->cstring[0] == 'F') ? 1 : 0;
    } else if (weather_unit_tuple->type == TUPLE_UINT) {
      unit = (uint8_t)weather_unit_tuple->value->uint32;
    } else if (weather_unit_tuple->type == TUPLE_INT) {
      unit = (uint8_t)weather_unit_tuple->value->int32;
    }
    if (unit != s_weather_unit) {
      s_weather_unit = unit;
      persist_write_int(PERSIST_KEY_WEATHER_UNIT, s_weather_unit);
      APP_LOG(APP_LOG_LEVEL_INFO, "weather unit=%u", s_weather_unit);
      send_settings_to_phone();
      weather_settings_changed = true;
      weather_unit_changed = true;
    }
  }

  Tuple *weather_temp_tuple = dict_find(iter, MESSAGE_KEY_WEATHER_TEMP);
  if (weather_temp_tuple) {
    if (weather_temp_tuple->type == TUPLE_INT) {
      s_weather_temp = (int16_t)weather_temp_tuple->value->int32;
    } else if (weather_temp_tuple->type == TUPLE_UINT) {
      s_weather_temp = (int16_t)weather_temp_tuple->value->uint32;
    }
    persist_write_int(PERSIST_KEY_WEATHER_TEMP, s_weather_temp);
    s_last_weather = time(NULL);
    s_weather_retry_count = 0;
    update_weather_temp_text();
    APP_LOG(APP_LOG_LEVEL_INFO, "weather temp=%d", s_weather_temp);
  }

  Tuple *weather_code_tuple = dict_find(iter, MESSAGE_KEY_WEATHER_CODE);
  if (weather_code_tuple) {
    if (weather_code_tuple->type == TUPLE_UINT) {
      s_weather_code = (uint8_t)weather_code_tuple->value->uint32;
    } else if (weather_code_tuple->type == TUPLE_INT) {
      s_weather_code = (uint8_t)weather_code_tuple->value->int32;
    }
    persist_write_int(PERSIST_KEY_WEATHER_CODE, s_weather_code);
    s_last_weather = time(NULL);
    s_weather_retry_count = 0;
    update_weather_icon();
    APP_LOG(APP_LOG_LEVEL_INFO, "weather code=%u", s_weather_code);
  }

  if (weather_settings_changed) {
    update_weather_visibility();
    update_weather_temp_text();
    update_weather_icon();
    if (weather_unit_changed) {
      s_last_weather = 0;
      s_weather_retry_count = 0;
    }
    request_weather();
  }
}

static void prv_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  window_set_background_color(window, GColorWhite);

  const int date_height = 20;
  const bool is_small_screen = bounds.size.w < 190;
  const int time_height = is_small_screen ? 44 : 52;
  const int time_center_y = (bounds.size.h * 3) / 4;
  const int time_y = time_center_y - (time_height / 2);
  const int date_y =
#ifdef PBL_ROUND
      (bounds.size.h / 10) - (date_height / 2);
#else
      is_small_screen
          ? bounds.size.h - date_height - 2
          : (bounds.size.h * 9) / 10 - (date_height / 2);
#endif

  s_date_layer = text_layer_create(GRect(2, date_y, bounds.size.w - 4, date_height));
  text_layer_set_background_color(s_date_layer, GColorClear);
  text_layer_set_text_color(s_date_layer, GColorBlack);
  s_date_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_PIX_CHICAGO_20));
  text_layer_set_font(s_date_layer, s_date_font);
  text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_date_layer));

  const int line_y = is_small_screen ? 20 : 25;
  s_line_layer = layer_create(GRect(0, line_y, bounds.size.w, 2));
  layer_set_update_proc(s_line_layer, line_layer_update_proc);
  layer_add_child(window_layer, s_line_layer);
  s_corner_line_layer = layer_create(GRect(4, line_y / 2, 12, 2));
  layer_set_update_proc(s_corner_line_layer, line_layer_update_proc);
  layer_add_child(window_layer, s_corner_line_layer);
#ifdef PBL_ROUND
  layer_set_hidden(s_line_layer, true);
  layer_set_hidden(s_corner_line_layer, true);
#endif

  int icon_x = 4;
  int icon_y = (line_y / 2) - (WEATHER_ICON_SIZE / 2);
  s_weather_icon_layer = bitmap_layer_create(GRect(icon_x, icon_y,
                                                   WEATHER_ICON_SIZE, WEATHER_ICON_SIZE));
  bitmap_layer_set_background_color(s_weather_icon_layer, GColorClear);
  layer_add_child(window_layer, bitmap_layer_get_layer(s_weather_icon_layer));

  const int temp_x = icon_x + WEATHER_ICON_SIZE + 2;
  int temp_y = icon_y;
  s_weather_temp_layer = text_layer_create(GRect(temp_x, temp_y, 50, 16));
  text_layer_set_background_color(s_weather_temp_layer, GColorClear);
  text_layer_set_text_color(s_weather_temp_layer, GColorBlack);
  text_layer_set_font(s_weather_temp_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
  text_layer_set_text_alignment(s_weather_temp_layer, GTextAlignmentLeft);
  layer_add_child(window_layer, text_layer_get_layer(s_weather_temp_layer));

  s_matrix_layer = layer_create(bounds);
  layer_set_update_proc(s_matrix_layer, matrix_layer_update_proc);
  layer_add_child(window_layer, s_matrix_layer);

  s_time_layer = text_layer_create(GRect(0, time_y, bounds.size.w, time_height));
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorBlack);
  s_time_font = fonts_load_custom_font(resource_get_handle(
      is_small_screen ? RESOURCE_ID_PIX_CHICAGO_38 : RESOURCE_ID_PIX_CHICAGO_45));
  text_layer_set_font(s_time_layer, s_time_font);
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));

  const int battery_width = 26;
  const int battery_height = 10;
  const int battery_margin = (line_y - battery_height) / 2;
  const int battery_x = bounds.size.w - battery_width - battery_margin;
  const int battery_y = battery_margin;
  s_battery_layer = layer_create(GRect(battery_x, battery_y, battery_width, battery_height));
  layer_set_update_proc(s_battery_layer, battery_layer_update_proc);
  layer_add_child(window_layer, s_battery_layer);

  apply_theme();
  update_weather_visibility();
  update_weather_layout();
  update_weather_temp_text();
  update_time();
}

static void prv_window_unload(Window *window) {
  text_layer_destroy(s_date_layer);
  text_layer_destroy(s_time_layer);
  fonts_unload_custom_font(s_date_font);
  fonts_unload_custom_font(s_time_font);
  layer_destroy(s_line_layer);
  layer_destroy(s_corner_line_layer);
  layer_destroy(s_matrix_layer);
  layer_destroy(s_battery_layer);
  if (s_weather_icon_bitmap) {
    gbitmap_destroy(s_weather_icon_bitmap);
    s_weather_icon_bitmap = NULL;
  }
  bitmap_layer_destroy(s_weather_icon_layer);
  text_layer_destroy(s_weather_temp_layer);
}

static void prv_init(void) {
  s_theme = DEFAULT_THEME;
  if (persist_exists(PERSIST_KEY_THEME)) {
    s_theme = persist_read_int(PERSIST_KEY_THEME);
  }
  if (persist_exists(PERSIST_KEY_WEATHER_ENABLED)) {
    s_weather_enabled = persist_read_bool(PERSIST_KEY_WEATHER_ENABLED);
  }
  if (persist_exists(PERSIST_KEY_WEATHER_SHOW_TEMP)) {
    s_weather_show_temp = persist_read_bool(PERSIST_KEY_WEATHER_SHOW_TEMP);
  }
  if (persist_exists(PERSIST_KEY_WEATHER_UNIT)) {
    s_weather_unit = (uint8_t)persist_read_int(PERSIST_KEY_WEATHER_UNIT);
  }
  if (persist_exists(PERSIST_KEY_WEATHER_TEMP)) {
    s_weather_temp = (int16_t)persist_read_int(PERSIST_KEY_WEATHER_TEMP);
  }
  if (persist_exists(PERSIST_KEY_WEATHER_CODE)) {
    s_weather_code = (uint8_t)persist_read_int(PERSIST_KEY_WEATHER_CODE);
  }

  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = prv_window_load,
    .unload = prv_window_unload,
  });

  const bool animated = true;
  window_stack_push(s_window, animated);

  app_message_register_inbox_received(inbox_received_handler);
  app_message_open(64, 64);
  s_bt_connected = bluetooth_connection_service_peek();
  bluetooth_connection_service_subscribe(connection_handler);
  send_settings_to_phone();
  request_weather();

  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  battery_handler(battery_state_service_peek());
  battery_state_service_subscribe(battery_handler);
}

static void prv_deinit(void) {
  battery_state_service_unsubscribe();
  bluetooth_connection_service_unsubscribe();
  window_destroy(s_window);
}

int main(void) {
  prv_init();
  app_event_loop();
  prv_deinit();
}
