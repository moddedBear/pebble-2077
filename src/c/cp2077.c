#include <pebble.h>
#include <ctype.h>

#define SETTINGS_KEY 1

const int MARGIN_SIZE = 4;
const int TEXT_HEIGHT = 14;

static Window *s_main_window;

static GFont s_time_font;
static GFont s_date_font;
static GFont s_text_font;

static TextLayer *s_time_layer;
static TextLayer *s_date_layer;
static TextLayer *s_day_layer;
static TextLayer *s_custom_layer;
static TextLayer *s_bt_layer;
static TextLayer *s_step_layer;
static TextLayer *s_temperature_layer;
static TextLayer *s_condition_layer;

static BitmapLayer *s_hud_layer;
static GBitmap *s_hud_bitmap;
static BitmapLayer *s_charge_layer;
static GBitmap *s_charge_bitmap;

static GColor color_fg;
static GColor color_bg;

static int s_battery_level;
static Layer *s_battery_layer;

typedef struct ClaySettings {
  bool show_steps, show_weather, weather_use_metric, hour_vibe, disconnect_alert;
  int temperature;
  char custom_text[32], condition[32];
} ClaySettings;

static ClaySettings settings;

static void str_to_upper(char *str) {
  char *s = str;
  while (*s) {
    *s = toupper((unsigned char) *s);
    s++;
  }
}

static void update_time() {
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);

  static char s_time_buffer[8];
  strftime(s_time_buffer, sizeof(s_time_buffer), clock_is_24h_style() ? "%H:%M" : "%I:%M", tick_time);

  static char s_date_buffer[8];
  strftime(s_date_buffer, sizeof(s_date_buffer), "%d", tick_time);

  static char s_day_buffer[32];
  strftime(s_day_buffer, sizeof(s_day_buffer), "%A", tick_time);
  str_to_upper(s_day_buffer);

  static char s_os_buffer[32];
  strftime(s_os_buffer, sizeof(s_os_buffer), settings.custom_text, tick_time);
  str_to_upper(s_os_buffer);

  text_layer_set_text(s_time_layer, s_time_buffer);
  text_layer_set_text(s_date_layer, s_date_buffer);
  text_layer_set_text(s_day_layer, s_day_buffer);
  text_layer_set_text(s_custom_layer, s_os_buffer);
}

static void update_steps() {
  #if defined(PBL_HEALTH)
  if (settings.show_steps) {
    HealthMetric metric = HealthMetricStepCount;
    time_t start = time_start_of_today();
    time_t end = time(NULL);

    HealthServiceAccessibilityMask mask = health_service_metric_accessible(metric, start, end);
    int step_count;

    if (mask & HealthServiceAccessibilityMaskAvailable) {
      step_count = (int)health_service_sum_today(metric);
    }
    else {
      step_count = 0;
    }
    static char s_step_buffer[16];
    snprintf(s_step_buffer, sizeof(s_step_buffer), "STEPS: %d", step_count);
    text_layer_set_text(s_step_layer, s_step_buffer);
    layer_set_hidden(text_layer_get_layer(s_step_layer), false);
  }
  else {
    layer_set_hidden(text_layer_get_layer(s_step_layer), true);
  }
  #endif
}

static void update_weather_layers() {
  if (settings.show_weather && settings.temperature && settings.temperature) {
    static char temperature_buffer[8];
    static char conditions_buffer[32];

    if (settings.weather_use_metric) {
      snprintf(temperature_buffer, sizeof(temperature_buffer), "%dC", settings.temperature);
    }
    else {
      int temp_f = settings.temperature * 1.8 + 32;
      snprintf(temperature_buffer, sizeof(temperature_buffer), "%dF", temp_f);
    }
    snprintf(conditions_buffer, sizeof(conditions_buffer), "%s", settings.condition);

    text_layer_set_text(s_condition_layer, conditions_buffer);
    text_layer_set_text(s_temperature_layer, temperature_buffer);

    // update position based on step visibility/availability
    GRect step_frame = layer_get_frame(text_layer_get_layer(s_step_layer));
    int x = step_frame.origin.x;
    if (!layer_get_hidden(text_layer_get_layer(s_step_layer))) {
      int condition_y = step_frame.origin.y - TEXT_HEIGHT;
      int temperature_y = condition_y - TEXT_HEIGHT;
      layer_set_frame(text_layer_get_layer(s_condition_layer),
        GRect(x, condition_y, 136, TEXT_HEIGHT)
      );
      layer_set_frame(text_layer_get_layer(s_temperature_layer),
        GRect(x, temperature_y, 136, TEXT_HEIGHT)
      );
    }
    else {
      int temperature_y = step_frame.origin.y - TEXT_HEIGHT;
      layer_set_frame(text_layer_get_layer(s_condition_layer), step_frame);
      layer_set_frame(text_layer_get_layer(s_temperature_layer),
        GRect(x, temperature_y, 136, TEXT_HEIGHT)
      );
    }

    layer_set_hidden(text_layer_get_layer(s_condition_layer), false);
    layer_set_hidden(text_layer_get_layer(s_temperature_layer), false);
  }
  else {
    layer_set_hidden(text_layer_get_layer(s_condition_layer), true);
    layer_set_hidden(text_layer_get_layer(s_temperature_layer), true);
  }
}

static void battery_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  int width = (s_battery_level * bounds.size.w) / 100;

  // draw the background
  graphics_context_set_fill_color(ctx, color_bg);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  // draw the bar
  graphics_context_set_fill_color(ctx, color_fg);
  graphics_fill_rect(ctx, GRect(0, 0, width, bounds.size.h), 0, GCornerNone);
}

static void load_fonts() {
  s_time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_RAJDHANI_58));
  s_date_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_RAJDHANI_24));
  s_text_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ORBITRON_12));
}

static void load_battery_layer(int x, int y) {
  s_battery_layer = layer_create(GRect(x+43, y+13, 96, 8));
  layer_set_update_proc(s_battery_layer, battery_update_proc);
}

static void load_hud_layer(int x, int y) {
  s_hud_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_HUD);
  s_hud_layer = bitmap_layer_create(
    GRect(x, y, 144, 40)
  );
  bitmap_layer_set_bitmap(s_hud_layer, s_hud_bitmap);
  bitmap_layer_set_alignment(s_hud_layer, GAlignCenter);
  bitmap_layer_set_compositing_mode(s_hud_layer, GCompOpSet);
}

static void load_charge_layer(int x, int y) {
  s_charge_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_CHARGE);
  s_charge_layer = bitmap_layer_create(
    GRect(x+43, y+13, 5, 8)
  );
  bitmap_layer_set_bitmap(s_charge_layer, s_charge_bitmap);
  bitmap_layer_set_alignment(s_charge_layer, GAlignTopLeft);
  bitmap_layer_set_compositing_mode(s_charge_layer, GCompOpSet);
}

static void load_time_layer(int x, int y) {
  s_time_layer = text_layer_create(
    GRect(x, y, 136, 58)
  );
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, color_fg);
  text_layer_set_font(s_time_layer, s_time_font);
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentLeft);
}

static void load_date_layer(int x, int y) {
  s_date_layer = text_layer_create(
    GRect(x+4, y+4, 36, 36)
  );
  text_layer_set_background_color(s_date_layer, GColorClear);
  text_layer_set_text_color(s_date_layer, color_fg);
  text_layer_set_font(s_date_layer, s_date_font);
  text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter);
}

static void load_day_layer(int x, int y) {
  s_day_layer = text_layer_create(
    GRect(x+42, y+24, 97, TEXT_HEIGHT)
  );
  text_layer_set_background_color(s_day_layer, GColorClear);
  text_layer_set_text_color(s_day_layer, color_fg);
  text_layer_set_font(s_day_layer, s_text_font);
  text_layer_set_text_alignment(s_day_layer, GTextAlignmentLeft);
}

// Used for small text layers like weather and steps
static void load_info_layer(TextLayer **layer, int y) {
  *layer = text_layer_create(
    GRect(MARGIN_SIZE, y, 136, TEXT_HEIGHT)
  );
  text_layer_set_background_color(*layer, GColorClear);
  text_layer_set_text_color(*layer, color_fg);
  text_layer_set_font(*layer, s_text_font);
}

static void load_hud(int x, int y) {
  load_battery_layer(x, y);
  load_hud_layer(x, y);
  load_charge_layer(x, y);
  load_date_layer(x, y);
  load_day_layer(x, y);
}

static void main_window_load(Window *window) {
  // get info about the window
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  load_fonts();

  // set colors
  color_fg = GColorWhite;
  color_bg = GColorBlack;

  window_set_background_color(window, color_bg);

  int hud_y = bounds.size.h - 44;
  int time_y = hud_y - 60;
  int custom_y = 2;
  int bt_y = custom_y;
  int step_y = time_y + 2;
  int condition_y = step_y - TEXT_HEIGHT;
  int temperature_y = condition_y - TEXT_HEIGHT;

  load_hud(0, hud_y);
  load_time_layer(MARGIN_SIZE, time_y);

  // custom text
  load_info_layer(&s_custom_layer, custom_y);

  // BT
  load_info_layer(&s_bt_layer, bt_y);
  text_layer_set_text(s_bt_layer, "NO_CONNECTION");

  // steps
  load_info_layer(&s_step_layer, step_y);
  // hide step layer initially so it doesn't show on unsupported devices
  layer_set_hidden(text_layer_get_layer(s_step_layer), true);

  // weather
  load_info_layer(&s_condition_layer, condition_y);
  load_info_layer(&s_temperature_layer, temperature_y);

  // add children
  layer_add_child(window_layer, s_battery_layer);
  layer_add_child(window_layer, bitmap_layer_get_layer(s_hud_layer));
  layer_add_child(window_layer, bitmap_layer_get_layer(s_charge_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_date_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_day_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_custom_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_bt_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_step_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_condition_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_temperature_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));
}

static void main_window_unload(Window *window) {
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_date_layer);
  text_layer_destroy(s_day_layer);
  text_layer_destroy(s_custom_layer);
  text_layer_destroy(s_bt_layer);
  text_layer_destroy(s_step_layer);
  text_layer_destroy(s_temperature_layer);
  text_layer_destroy(s_condition_layer);
  gbitmap_destroy(s_hud_bitmap);
  bitmap_layer_destroy(s_hud_layer);
  gbitmap_destroy(s_charge_bitmap);
  bitmap_layer_destroy(s_charge_layer);
  fonts_unload_custom_font(s_time_font);
  fonts_unload_custom_font(s_date_font);
  fonts_unload_custom_font(s_text_font);
  layer_destroy(s_battery_layer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();

  // request new weather data
  if (tick_time->tm_min % 30 == 0 && settings.show_weather) {
    DictionaryIterator *it;
    app_message_outbox_begin(&it);
    dict_write_uint8(it, 0, 0);
    app_message_outbox_send();
  }

  if (tick_time->tm_min == 0 && settings.hour_vibe) {
    vibes_double_pulse();
  }
}

static void battery_callback(BatteryChargeState state) {
  s_battery_level = state.charge_percent;
  layer_mark_dirty(s_battery_layer);
  layer_set_hidden(bitmap_layer_get_layer(s_charge_layer), !state.is_plugged);
}

static void bt_callback(bool connected) {
  // Replace OS layer with BT layer when disconnected
  layer_set_hidden(text_layer_get_layer(s_bt_layer), connected);
  layer_set_hidden(text_layer_get_layer(s_custom_layer), !connected);

  if (!connected) {
    vibes_short_pulse();
  }
}

#if defined(PBL_HEALTH)
static void health_handler(HealthEventType event, void *context) {
  if (event == HealthEventSignificantUpdate || event == HealthEventMovementUpdate) {
    update_steps();
  }
}
#endif

static void update_health_subscription() {
  #if defined(PBL_HEALTH)
  if (settings.show_steps) {
    health_service_events_subscribe(health_handler, NULL);
  }
  else {
    health_service_events_unsubscribe();
  }
  #endif
}

static void default_settings() {
  settings.show_steps = true;
  settings.show_weather = true;
  settings.weather_use_metric = true;
  settings.hour_vibe = false;
  settings.disconnect_alert = true;
  settings.temperature = (int)NULL;
  strncpy(settings.custom_text, "PBL_%m%U%j", sizeof(settings.custom_text));
  strncpy(settings.condition, "", sizeof(settings.condition));
}

static void load_settings() {
  default_settings();
  persist_read_data(SETTINGS_KEY, &settings, sizeof(settings));
}

static void save_settings() {
  persist_write_data(SETTINGS_KEY, &settings, sizeof(settings));
  update_time();
  update_steps();
  update_weather_layers();
  update_health_subscription();
}

static void inbox_received_callback(DictionaryIterator *it, void *ctx) {
  Tuple *temperature_t = dict_find(it, MESSAGE_KEY_TEMPERATURE);
  Tuple *conditions_t = dict_find(it, MESSAGE_KEY_CONDITIONS);
  if (temperature_t && conditions_t) {
    settings.temperature = temperature_t->value->int32;
    strncpy(settings.condition, conditions_t->value->cstring, sizeof(settings.condition));
  }
  
  Tuple *show_steps_t = dict_find(it, MESSAGE_KEY_PREF_SHOW_STEPS);
  if (show_steps_t) {
    settings.show_steps = show_steps_t->value->int32 == 1;
  }

  Tuple *show_weather_t = dict_find(it, MESSAGE_KEY_PREF_SHOW_WEATHER);
  if (show_weather_t) {
    settings.show_weather = show_weather_t->value->int32 == 1;
  }

  Tuple *weather_use_metric_t = dict_find(it, MESSAGE_KEY_PREF_WEATHER_METRIC);
  if (weather_use_metric_t) {
    settings.weather_use_metric = weather_use_metric_t->value->int32 == 1;
  }

  Tuple *hour_vibe_t = dict_find(it, MESSAGE_KEY_PREF_HOUR_VIBE);
  if (hour_vibe_t) {
    settings.hour_vibe = hour_vibe_t->value->int32 == 1;
  }

  Tuple *disconnect_alert_t = dict_find(it, MESSAGE_KEY_PREF_DISCONNECT_ALERT);
  if (disconnect_alert_t) {
    settings.disconnect_alert = disconnect_alert_t->value->int32 == 1;
  }

  Tuple *custom_text_t = dict_find(it, MESSAGE_KEY_PREF_CUSTOM_TEXT);
  if (custom_text_t) {
    strncpy(settings.custom_text, custom_text_t->value->cstring, sizeof(settings.custom_text));
  }

  save_settings();
}

static void init() {
  load_settings();

  // register for time updates
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  // register for battery updates
  battery_state_service_subscribe(battery_callback);
  // register for connection updates
  connection_service_subscribe((ConnectionHandlers) {
    .pebble_app_connection_handler = bt_callback
  });

  // register app message inbox and outbox
  app_message_register_inbox_received(inbox_received_callback);

  // open app message
  const int inbox_size = 256;
  const int outbox_size = 16;
  app_message_open(inbox_size, outbox_size);

  s_main_window = window_create();

  // set handlers to manage the elements inside the window
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });

  // show the window on the watch
  window_stack_push(s_main_window, true);

  update_time();
  update_steps();
  update_weather_layers();
  battery_callback(battery_state_service_peek());
  bt_callback(connection_service_peek_pebble_app_connection());
}

static void deinit() {
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}