#include <pebble.h>
#include <stdbool.h>
#include "text.c"
#include "data.c"

static Window *window;
static TextLayer *tl_current_time;
static TextLayer *tl_current_date;
static TextLayer *tl_next_class_subject;
static TextLayer *tl_next_class_time;

static TextLayer* text_layer_create_default(GRect rect) {
  TextLayer *tl = text_layer_create(rect);
  text_layer_set_text_alignment(tl, GTextAlignmentCenter);
  text_layer_set_background_color(tl, GColorBlack);
  text_layer_set_text_color(tl, GColorWhite);
  /* text_layer_set_font(tl, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_SIGNIKA_18))); */
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

static void set_class_text(char* subject, char* time) {
  text_layer_set_text(tl_next_class_subject, subject);
  text_layer_set_text(tl_next_class_time, time);
}

static void update_next_class_time(struct tm *tick_time) {
  uint16_t current_minutes = tick_time->tm_hour * 60 + tick_time->tm_min;
  ClassEvent event = data_next_class_event(current_minutes);
  if (tick_time->tm_wday != schedule_weekday) {
    set_class_text("Connect phone", "& wait to update.");
  } else if (event.is_nothing) {
    set_class_text("No more classes.", "See you tomorrow.");
  } else {
    int16_t next_class_minutes_left = event.minutes - current_minutes;
    static char *subject = "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"; // fucking cstrings
    fucking_copy_string(subject, event.subject, SUBJECT_LENGTH); // i can't even
    set_class_text(subject, format_next_class_time(next_class_minutes_left, event.verb));
  }
}

static void handle_minute_tick(struct tm *tick_time, TimeUnits units_changed) {
  text_layer_set_text(tl_current_time, format_time(tick_time));
  text_layer_set_text(tl_current_date, format_date(tick_time));
  if (tick_time->tm_wday != schedule_weekday) data_request_from_phone();
  update_next_class_time(tick_time);
}

static void handle_message_receive(DictionaryIterator *iter, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Received message from phone");
  struct tm *cur_time = current_time();
  data_set_from_dict(iter, cur_time);
  update_next_class_time(cur_time);
}

static void handle_message_send_failed(DictionaryIterator *failed, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Sending message to phone failed with reason code %d", reason);
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
  app_message_open(/* inbound_size: */ 2048, /* outbound_size: */ 32);
  data_read_persisted();
  window_stack_push(window, /* animated: */ true);
  handle_minute_tick(current_time(), MINUTE_UNIT);
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
