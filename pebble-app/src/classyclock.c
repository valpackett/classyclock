#include <pebble.h>
#include "text.c"
#include "data.c"

static Window *window;
static TextLayer *tl_current_time;
static TextLayer *tl_current_date;
static TextLayer *tl_next_class_subject;
static TextLayer *tl_next_class_time;
static uint8_t do_retry = 0;
static uint8_t is_nothing = 0;

static TextLayer* text_layer_create_default(GRect rect) {
  TextLayer *tl = text_layer_create(rect);
  text_layer_set_text_alignment(tl, GTextAlignmentCenter);
  text_layer_set_background_color(tl, GColorBlack);
  text_layer_set_text_color(tl, GColorWhite);
  text_layer_set_font(tl, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_SIGNIKA_18)));
  return tl;
}

static void handle_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  tl_current_time = text_layer_create_default((GRect) { .origin = { 0, 10 }, .size = { bounds.size.w, 50 } });
  text_layer_set_font(tl_current_time, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_SIGNIKA_SUBSET_40)));
  layer_add_child(window_layer, text_layer_get_layer(tl_current_time));

  tl_current_date = text_layer_create_default((GRect) { .origin = { 0, 60 }, .size = { bounds.size.w, 30 } });
  text_layer_set_font(tl_current_date, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_SIGNIKA_21)));
  layer_add_child(window_layer, text_layer_get_layer(tl_current_date));

  tl_next_class_subject = text_layer_create_default((GRect) { .origin = { 0, 100 }, .size = { bounds.size.w, 25 } });
  layer_add_child(window_layer, text_layer_get_layer(tl_next_class_subject));

  tl_next_class_time = text_layer_create_default((GRect) { .origin = { 0, 124 }, .size = { bounds.size.w, 25 } });
  layer_add_child(window_layer, text_layer_get_layer(tl_next_class_time));
}

static void handle_window_unload(Window *window) {
  text_layer_destroy(tl_current_time);
  text_layer_destroy(tl_current_date);
  text_layer_destroy(tl_next_class_subject);
  text_layer_destroy(tl_next_class_time);
}

static void update_next_class_time(struct tm *tick_time) {
  uint16_t current_minutes = tick_time->tm_hour * 60 + tick_time->tm_min;
  int16_t next_class_minutes_left = next_class_minutes - current_minutes;
  if (is_nothing == 1) {
    text_layer_set_text(tl_next_class_subject, "No more classes.");
    text_layer_set_text(tl_next_class_time, "See you tomorrow.");
  } else if (next_class_minutes_left <= 0) {
    text_layer_set_text(tl_next_class_subject, "Updating");
    text_layer_set_text(tl_next_class_time, "...");
  } else {
    text_layer_set_text(tl_next_class_subject, next_class_subject);
    text_layer_set_text(tl_next_class_time, format_next_class_time(next_class_minutes_left, next_class_verb));
  }
  if (next_class_minutes_left <= 0) data_request_from_phone();
}

static void handle_minute_tick(struct tm *tick_time, TimeUnits units_changed) {
  text_layer_set_text(tl_current_time, format_time(tick_time));

  static char date_text[] = "Xxxxxxxxx 00";
  strftime(date_text, sizeof(date_text), "%a %b %e", tick_time);
  text_layer_set_text(tl_current_date, date_text);

  if (do_retry == 1 || (tick_time->tm_hour == 0 && tick_time->tm_min == 1)) data_request_from_phone();
  update_next_class_time(tick_time);
}

static void handle_message_receive(DictionaryIterator *iter, void *context) {
  do_retry = 0;
  time_t now = time(NULL);
  struct tm *current_time = localtime(&now);
  is_nothing = data_set_from_dict(iter);
  if (is_nothing == 1) {
    uint16_t current_minutes = current_time->tm_hour * 60 + current_time->tm_min;
    next_class_minutes = current_minutes + 10;
  }
  update_next_class_time(current_time);
}

static void handle_message_send_failed(DictionaryIterator *failed, AppMessageResult reason, void *context) {
  do_retry = 1;
  APP_LOG(APP_LOG_LEVEL_DEBUG, "FAIL: %d", reason);
}

static void handle_init(void) {
  window = window_create();
  window_set_background_color(window, GColorBlack);
  window_set_window_handlers(window, (WindowHandlers) {
    .load = handle_window_load,
    .unload = handle_window_unload,
  });
  app_message_register_inbox_received(handle_message_receive);
  app_message_register_outbox_failed(handle_message_send_failed);
  app_message_open(/* inbound_size: */ 128, /* outbound_size: */ 128);
  window_stack_push(window, /* animated: */ true);
  time_t now = time(NULL);
  struct tm *current_time = localtime(&now);
  handle_minute_tick(current_time, MINUTE_UNIT);
  tick_timer_service_subscribe(MINUTE_UNIT, handle_minute_tick);
}

static void handle_deinit(void) {
  window_destroy(window);
  tick_timer_service_unsubscribe();
}

int main(void) {
  handle_init();
  app_event_loop();
  handle_deinit();
}
