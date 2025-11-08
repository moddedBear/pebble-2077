#include <pebble.h>
#include <ctype.h>

const int MARGIN_SIZE = 4;
const int TEXT_HEIGHT = 14;

static Window *s_main_window;

static GFont s_time_font;
static GFont s_date_font;
static GFont s_text_font;

static TextLayer *s_time_layer;
static TextLayer *s_date_layer;
static TextLayer *s_day_layer;
static TextLayer *s_os_layer;
static TextLayer *s_bt_layer;
static TextLayer *s_step_layer;
static TextLayer *s_temperature_layer;
static TextLayer *s_condition_layer;

static BitmapLayer *s_hud_layer;
static GBitmap *s_hud_bitmap;

static GColor color_fg;
static GColor color_bg;

static int s_battery_level;
static Layer *s_battery_layer;

static void update_time() {
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);

  static char s_time_buffer[8];
  strftime(s_time_buffer, sizeof(s_time_buffer), clock_is_24h_style() ? "%H:%M" : "%I:%M", tick_time);

  static char s_date_buffer[8];
  strftime(s_date_buffer, sizeof(s_date_buffer), "%d", tick_time);

  static char s_day_buffer[32];
  strftime(s_day_buffer, sizeof(s_day_buffer), "%A", tick_time);
  char *s = s_day_buffer;
  while (*s) {
    *s = toupper((unsigned char) *s);
    s++;
  }

  static char s_os_buffer[16];
  strftime(s_os_buffer, sizeof(s_os_buffer), "PBBL_%m%U%j", tick_time);
  // strftime(s_os_buffer, sizeof(s_os_buffer), "PBBL_9999999", tick_time);

  text_layer_set_text(s_time_layer, s_time_buffer);
  text_layer_set_text(s_date_layer, s_date_buffer);
  text_layer_set_text(s_day_layer, s_day_buffer);
  text_layer_set_text(s_os_layer, s_os_buffer);
}

static void update_steps() {
  #if defined(PBL_HEALTH)
  HealthMetric metric = HealthMetricStepCount;
  time_t start = time_start_of_today();
  time_t end = time(NULL);

  HealthServiceAccessibilityMask mask = health_service_metric_accessible(metric, start, end);

  if (mask & HealthServiceAccessibilityMaskAvailable) {
    static char s_step_buffer[16];
    snprintf(s_step_buffer, sizeof(s_step_buffer), "STEPS: %d", (int)health_service_sum_today(metric));
    text_layer_set_text(s_step_layer, s_step_buffer);
    layer_set_hidden(text_layer_get_layer(s_step_layer), false);
  }
  else {
    layer_set_hidden(text_layer_get_layer(s_step_layer), true);
  }
  #endif
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

// TODO: refactor these
static void load_os_layer(int x, int y) {
  s_os_layer = text_layer_create(
    GRect(x, y, 136, TEXT_HEIGHT)
  );
  text_layer_set_background_color(s_os_layer, GColorClear);
  text_layer_set_text_color(s_os_layer, color_fg);
  text_layer_set_font(s_os_layer, s_text_font);
}

static void load_bt_layer(int x, int y) {
  s_bt_layer = text_layer_create(
    GRect(x, y, 136, TEXT_HEIGHT)
  );
  text_layer_set_background_color(s_bt_layer, GColorClear);
  text_layer_set_text_color(s_bt_layer, color_fg);
  text_layer_set_font(s_bt_layer, s_text_font);
  text_layer_set_text(s_bt_layer, "NO_CONNECTION");
}

static void load_step_layer(int x, int y) {
  s_step_layer = text_layer_create(
    GRect(x, y, 136, TEXT_HEIGHT)
  );
  text_layer_set_background_color(s_step_layer, GColorClear);
  text_layer_set_text_color(s_step_layer, color_fg);
  text_layer_set_font(s_step_layer, s_text_font);
}

static void load_condition_layer(int x, int y) {
  s_condition_layer = text_layer_create(
    GRect(x, y, 136, TEXT_HEIGHT)
  );
  text_layer_set_background_color(s_condition_layer, GColorClear);
  text_layer_set_text_color(s_condition_layer, color_fg);
  text_layer_set_font(s_condition_layer, s_text_font);
}

static void load_temperature_layer(int x, int y) {
  s_temperature_layer = text_layer_create(
    GRect(x, y, 136, TEXT_HEIGHT)
  );
  text_layer_set_background_color(s_temperature_layer, GColorClear);
  text_layer_set_text_color(s_temperature_layer, color_fg);
  text_layer_set_font(s_temperature_layer, s_text_font);
}

static void load_hud(int x, int y) {
  load_battery_layer(x, y);
  load_hud_layer(x, y);
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
  // int os_y = time_y + 2;
  int os_y = 2;
  int bt_y = os_y + TEXT_HEIGHT;
  // int step_y = os_y - TEXT_HEIGHT;
  int step_y = time_y + 2;
  int condition_y = step_y - TEXT_HEIGHT;  // TODO: adjust position if steps are hidden
  int temperature_y = condition_y - TEXT_HEIGHT;

  load_hud(0, hud_y);
  load_time_layer(MARGIN_SIZE, time_y);
  load_os_layer(MARGIN_SIZE, os_y);
  // load_bt_layer(MARGIN_SIZE, MARGIN_SIZE);
  load_bt_layer(MARGIN_SIZE, bt_y);
  load_step_layer(MARGIN_SIZE, step_y);
  load_condition_layer(MARGIN_SIZE, condition_y);
  load_temperature_layer(MARGIN_SIZE, temperature_y);

  // add children
  layer_add_child(window_layer, s_battery_layer);
  layer_add_child(window_layer, bitmap_layer_get_layer(s_hud_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_date_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_day_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_os_layer));
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
  text_layer_destroy(s_os_layer);
  text_layer_destroy(s_bt_layer);
  text_layer_destroy(s_step_layer);
  text_layer_destroy(s_temperature_layer);
  text_layer_destroy(s_condition_layer);
  gbitmap_destroy(s_hud_bitmap);
  bitmap_layer_destroy(s_hud_layer);
  fonts_unload_custom_font(s_time_font);
  fonts_unload_custom_font(s_date_font);
  fonts_unload_custom_font(s_text_font);
  layer_destroy(s_battery_layer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();

  // request new weather data
  if (tick_time->tm_min % 30 == 0) {
    DictionaryIterator *it;
    app_message_outbox_begin(&it);
    dict_write_uint8(it, 0, 0);
    app_message_outbox_send();
  }
}

static void battery_callback(BatteryChargeState state) {
  s_battery_level = state.charge_percent;
  layer_mark_dirty(s_battery_layer);
  // TODO: get charging state
}

static void bt_callback(bool connected) {
  layer_set_hidden(text_layer_get_layer(s_bt_layer), connected);

  if (!connected) {
    vibes_double_pulse();
  }
}

static void health_handler(HealthEventType event, void *context) {
  #if defined(PBL_HEALTH)
  if (event == HealthEventSignificantUpdate || event == HealthEventMovementUpdate) {
    update_steps();
  }
  #endif
}

static void inbox_received_callback(DictionaryIterator *it, void *ctx) {
  static char temperature_buffer[8];
  static char conditions_buffer[32];

  Tuple *temperature_tuple = dict_find(it, MESSAGE_KEY_TEMPERATURE);
  Tuple *conditions_tuple = dict_find(it, MESSAGE_KEY_CONDITIONS);

  if (temperature_tuple && conditions_tuple) {
    snprintf(temperature_buffer, sizeof(temperature_buffer), "%dF", (int)temperature_tuple->value->int32);
    snprintf(conditions_buffer, sizeof(conditions_buffer), "%s", conditions_tuple->value->cstring);

    text_layer_set_text(s_condition_layer, conditions_buffer);
    text_layer_set_text(s_temperature_layer, temperature_buffer);
  }
}

static void inbox_dropped_callback(AppMessageResult reason, void *ctx) {

}

static void outbox_sent_callback(DictionaryIterator *it, void *ctx) {

}

static void outbox_failed_callback(DictionaryIterator *it, AppMessageResult reason, void *ctx) {

}

static void init() {
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
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_sent(outbox_sent_callback);
  app_message_register_outbox_failed(outbox_failed_callback);

  // open app message
  const int inbox_size = 128;
  const int outbox_size = 128;
  app_message_open(inbox_size, outbox_size);
  
  // register for health updates
  #if defined(PBL_HEALTH)
  health_service_events_subscribe(health_handler, NULL);
  #endif

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