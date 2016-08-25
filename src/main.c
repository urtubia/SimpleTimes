#include <pebble.h>

Window *s_main_window;
static TextLayer *s_time_layer;
static TextLayer *s_utc_time_layer;
static TextLayer *s_date_layer;
static TextLayer *s_steps_layer;

static Layer *s_battery_gauge_layer;

static int charge_percent = 5;
static bool is_charging = false;

static void health_handler(HealthEventType event, void *context) {
  static char s_steps_text[] = "1000000 steps";
  // Which type of event occurred?
  switch(event) {
    
    case HealthEventSignificantUpdate:
      //APP_LOG(APP_LOG_LEVEL_INFO, 
      //        "New HealthService HealthEventSignificantUpdate event");
      //break;
    case HealthEventMovementUpdate:
      
      HealthMetric metric = HealthMetricStepCount;
      time_t start = time_start_of_today();
      time_t end = time(NULL);

      // Check the metric has data available for today
      HealthServiceAccessibilityMask mask = health_service_metric_accessible(metric, start, end);

      if(mask & HealthServiceAccessibilityMaskAvailable) {
        // Data is available!
        int steps_today = (int)health_service_sum_today(metric);
        snprintf(s_steps_text, sizeof(s_steps_text), "%d steps", steps_today);
        text_layer_set_text(s_steps_layer, s_steps_text);
      } else {
        // No data recorded yet today
        APP_LOG(APP_LOG_LEVEL_ERROR, "Data unavailable!");
      }
      break;
    case HealthEventSleepUpdate:
      APP_LOG(APP_LOG_LEVEL_INFO, 
              "New HealthService HealthEventSleepUpdate event");
      break;
  }
}

static void handle_battery(BatteryChargeState charge_state) {
  charge_percent = charge_state.charge_percent;
  is_charging = charge_state.is_charging;
  layer_mark_dirty(s_battery_gauge_layer);
}

static void battery_gauge_update(struct Layer *layer, GContext *ctx)
{
  if(is_charging){
    graphics_context_set_fill_color(ctx, GColorMintGreen);
  }else{
    graphics_context_set_fill_color(ctx, GColorVividCerulean);
  }
  
  graphics_fill_rect(ctx, layer_get_bounds(layer), 0, GCornerNone);
  if(is_charging){
    graphics_context_set_fill_color(ctx, GColorJaegerGreen);
  }else{
    graphics_context_set_fill_color(ctx, GColorDukeBlue);
  }
  
  int gauge_width = (int) ((charge_percent / 100.0) * layer_get_bounds(layer).size.w);
  
  graphics_fill_rect(ctx, GRect(0, 0, gauge_width, layer_get_bounds(layer).size.h), 0, GCornerNone);
   
}

static void handle_second_tick(struct tm* tick_time, TimeUnits units_changed) {
  static char s_time_text[] = "00:00:00";
  static char s_utc_time_text[] = "00:00:00 UTC";
  static char s_date_text[] = "XXXX XX XXX XXXXX";

  strftime(s_time_text, sizeof(s_time_text), clock_is_24h_style() ?
                                          "%H:%M" : "%I:%M", tick_time);
  text_layer_set_text(s_time_layer, s_time_text);
  
  
  time_t epoch = mktime(tick_time);
  struct tm* gm_time = gmtime(&epoch);
  strftime(s_utc_time_text, sizeof(s_utc_time_text), "%R UTC", gm_time);
  text_layer_set_text(s_utc_time_layer, s_utc_time_text);
  
  time_t now = time(NULL);
  struct tm *local_time = localtime(&now);
  strftime(s_date_text, sizeof(s_date_text), "%a %d %b", local_time);
  text_layer_set_text(s_date_layer, s_date_text);
}

static void main_window_load(Window *window) {
  
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_frame(window_layer);
  
  s_time_layer = text_layer_create(GRect(0, 20, bounds.size.w, 54));
  text_layer_set_text_color(s_time_layer, GColorBlack);
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  
  s_utc_time_layer = text_layer_create(GRect(0,70,bounds.size.w, 30));
  text_layer_set_text_color(s_utc_time_layer, GColorBlack);
  text_layer_set_background_color(s_utc_time_layer, GColorClear);
  text_layer_set_font(s_utc_time_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_text_alignment(s_utc_time_layer, GTextAlignmentCenter);
  
  s_date_layer = text_layer_create(GRect(0, 110, bounds.size.w, 30));
  text_layer_set_text_color(s_date_layer, GColorBlack);
  text_layer_set_background_color(s_date_layer, GColorClear);
  text_layer_set_font(s_date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter);
  
  s_steps_layer = text_layer_create(GRect(0, 140, bounds.size.w, 30));
  text_layer_set_text_color(s_steps_layer, GColorBlack);
  text_layer_set_background_color(s_steps_layer, GColorClear);
  text_layer_set_font(s_steps_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_alignment(s_steps_layer, GTextAlignmentCenter);
  
  
  s_battery_gauge_layer = layer_create(GRect(0 + 20,100, bounds.size.w - 40, 2));
  layer_set_update_proc(s_battery_gauge_layer, battery_gauge_update);
  
  
  time_t now = time(NULL);
  struct tm *current_time = localtime(&now);
  handle_second_tick(current_time, SECOND_UNIT);

  tick_timer_service_subscribe(SECOND_UNIT, handle_second_tick);
  
  handle_battery(battery_state_service_peek());
  battery_state_service_subscribe(handle_battery);
  
#if defined(PBL_HEALTH)
  // Attempt to subscribe 
  if(!health_service_events_subscribe(health_handler, NULL)) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Health not available!");
  }
#endif

  
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_utc_time_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_date_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_steps_layer));
  layer_add_child(window_layer, s_battery_gauge_layer);
}

static void main_window_unload(Window *window) {
  
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_utc_time_layer);
  text_layer_destroy(s_date_layer);
  text_layer_destroy(s_steps_layer);
  layer_destroy(s_battery_gauge_layer);
}

static void init() {
  s_main_window = window_create();
  window_set_background_color(s_main_window, GColorWhite);
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload,
  });
  window_stack_push(s_main_window, true);
}

void handle_deinit(void) {
  tick_timer_service_unsubscribe();
  battery_state_service_unsubscribe();
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  handle_deinit();
}
