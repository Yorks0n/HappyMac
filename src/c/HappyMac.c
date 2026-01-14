#include <pebble.h>
#include <ctype.h>
#include "message_keys.auto.h"

static Window *s_window;
static TextLayer *s_date_layer;
static TextLayer *s_time_layer;
static Layer *s_line_layer;
static Layer *s_corner_line_layer;
static Layer *s_matrix_layer;
static Layer *s_battery_layer;
static GFont s_date_font;
static GFont s_time_font;
static BatteryChargeState s_battery_state;
static GColor s_foreground_color;
static GColor s_background_color;
static int s_theme;

enum {
  THEME_LIGHT = 0,
  THEME_DARK = 1,
  THEME_COLOR = 2,
};

static const int DEFAULT_THEME = THEME_COLOR;

enum {
  PERSIST_KEY_THEME = 1,
};

static void apply_theme(void);
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

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
}

static void line_layer_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  graphics_context_set_fill_color(ctx, s_foreground_color);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);
}

static void matrix_layer_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  const int pixel_size = bounds.size.w < 190 ? 2 : 3;
  const int matrix_width = 31 * pixel_size;
  const int matrix_height = 32 * pixel_size;
  const int origin_x = (bounds.size.w - matrix_width) / 2;
  const int origin_y = (bounds.size.h * 2 / 5) - (matrix_height / 2);

  graphics_context_set_fill_color(ctx, s_foreground_color);
  for (int row = 0; row < 32; ++row) {
    for (int col = 0; col < 31; ++col) {
      if (s_matrix[row][col] == 1) {
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

static void apply_theme(void) {
  switch (s_theme) {
    case THEME_DARK:
      s_background_color = GColorBlack;
      s_foreground_color = GColorWhite;
      break;
    case THEME_COLOR:
#ifdef PBL_COLOR
      s_background_color = GColorPastelYellow;
      s_foreground_color = GColorBlack;
#else
      s_background_color = GColorWhite;
      s_foreground_color = GColorBlack;
#endif
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
}

static void inbox_received_handler(DictionaryIterator *iter, void *context) {
  Tuple *theme_tuple = dict_find(iter, MESSAGE_KEY_theme);
  if (theme_tuple) {
    s_theme = (int)theme_tuple->value->int32;
    persist_write_int(PERSIST_KEY_THEME, s_theme);
    apply_theme();
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
}

static void prv_init(void) {
  s_theme = DEFAULT_THEME;
  if (persist_exists(PERSIST_KEY_THEME)) {
    s_theme = persist_read_int(PERSIST_KEY_THEME);
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

  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  battery_handler(battery_state_service_peek());
  battery_state_service_subscribe(battery_handler);
}

static void prv_deinit(void) {
  battery_state_service_unsubscribe();
  window_destroy(s_window);
}

int main(void) {
  prv_init();
  app_event_loop();
  prv_deinit();
}
