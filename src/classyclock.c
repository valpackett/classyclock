#include <pebble.h>

static Window *window;
static TextLayer *tl_current_time;
static TextLayer *tl_current_date;
static TextLayer *tl_next_class_subject;
static TextLayer *tl_next_class_time;
static char next_class_subject[32];
static char next_class_time[32];
static char *next_class_verb = "xxxxxx";
static uint16_t next_class_minutes;
static uint8_t do_retry = 0;
static uint8_t is_nothing = 0;

enum {
  MSG_KEY_NOTHING = 0x0,
  MSG_KEY_TIME = 0x1,
  MSG_KEY_SUBJ = 0x2,
  MSG_KEY_END = 0x3,
  MSG_KEY_GET = 0x4
};

static void handle_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  tl_current_time = text_layer_create((GRect) { .origin = { 0, 10 }, .size = { bounds.size.w, 50 } });
  text_layer_set_font(tl_current_time, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_SIGNIKA_SUBSET_40)));
  text_layer_set_text_alignment(tl_current_time, GTextAlignmentCenter);
  text_layer_set_background_color(tl_current_time, GColorBlack);
  text_layer_set_text_color(tl_current_time, GColorWhite);
  layer_add_child(window_layer, text_layer_get_layer(tl_current_time));

  tl_current_date = text_layer_create((GRect) { .origin = { 0, 60 }, .size = { bounds.size.w, 30 } });
  text_layer_set_font(tl_current_date, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_SIGNIKA_21)));
  text_layer_set_text_alignment(tl_current_date, GTextAlignmentCenter);
  text_layer_set_background_color(tl_current_date, GColorBlack);
  text_layer_set_text_color(tl_current_date, GColorWhite);
  layer_add_child(window_layer, text_layer_get_layer(tl_current_date));

  tl_next_class_subject = text_layer_create((GRect) { .origin = { 0, 100 }, .size = { bounds.size.w, 25 } });
  text_layer_set_font(tl_next_class_subject, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_SIGNIKA_18)));
  text_layer_set_text_alignment(tl_next_class_subject, GTextAlignmentCenter);
  text_layer_set_background_color(tl_next_class_subject, GColorBlack);
  text_layer_set_text_color(tl_next_class_subject, GColorWhite);
  layer_add_child(window_layer, text_layer_get_layer(tl_next_class_subject));

  tl_next_class_time = text_layer_create((GRect) { .origin = { 0, 124 }, .size = { bounds.size.w, 25 } });
  text_layer_set_font(tl_next_class_time, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_SIGNIKA_18)));
  text_layer_set_text_alignment(tl_next_class_time, GTextAlignmentCenter);
  text_layer_set_background_color(tl_next_class_time, GColorBlack);
  text_layer_set_text_color(tl_next_class_time, GColorWhite);
  layer_add_child(window_layer, text_layer_get_layer(tl_next_class_time));
}

static void handle_window_unload(Window *window) {
  text_layer_destroy(tl_current_time);
  text_layer_destroy(tl_current_date);
  text_layer_destroy(tl_next_class_time);
  text_layer_destroy(tl_next_class_time);
}

static void send_message_get() {
  Tuplet get_tuple = TupletInteger(MSG_KEY_GET, 1);
  DictionaryIterator *iter;
  if (app_message_outbox_begin(&iter) != APP_MSG_OK) return;
  if (dict_write_tuplet(iter, &get_tuple) != DICT_OK) return;
  app_message_outbox_send();
}

static void update_next_class_time(struct tm *tick_time) {
  uint16_t current_minutes = tick_time->tm_hour * 60 + tick_time->tm_min;
  int16_t next_class_minutes_left = next_class_minutes - current_minutes;
  if (is_nothing == 1) {
    text_layer_set_text(tl_next_class_subject, "No more classes.");
    text_layer_set_text(tl_next_class_time, "See you tomorrow.");
  } else {
    text_layer_set_text(tl_next_class_subject, next_class_subject);
    snprintf(next_class_time, 32, "%s in %d min.", next_class_verb, next_class_minutes_left);
    text_layer_set_text(tl_next_class_time, next_class_time);
  }
  if (next_class_minutes_left <= 0) send_message_get();
}

static void handle_minute_tick(struct tm *tick_time, TimeUnits units_changed) {
  static char time_text[] = "00:00";
  static char *time_format;
  if (clock_is_24h_style()) {
    time_format = "%R";
  } else {
    time_format = "%I:%M";
  }
  strftime(time_text, sizeof(time_text), time_format, tick_time);
  text_layer_set_text(tl_current_time, time_text);

  static char date_text[] = "Xxxxxxxxx 00";
  strftime(date_text, sizeof(date_text), "%B %e", tick_time);
  text_layer_set_text(tl_current_date, date_text);

  if (do_retry == 1) send_message_get();
  update_next_class_time(tick_time);
}

static void handle_message_receive(DictionaryIterator *iter, void *context) {
  do_retry = 0;
  Tuple *nothing_tuple = dict_find(iter, MSG_KEY_NOTHING);
  time_t now = time(NULL);
  struct tm *current_time = localtime(&now);
  if (nothing_tuple) {
    is_nothing = 1;
    uint16_t current_minutes = current_time->tm_hour * 60 + current_time->tm_min;
    next_class_minutes = current_minutes + 10;
  } else {
    is_nothing = 0;
    Tuple *subj_tuple = dict_find(iter, MSG_KEY_SUBJ);
    Tuple *time_tuple = dict_find(iter, MSG_KEY_TIME);
    Tuple *end_tuple = dict_find(iter, MSG_KEY_END);
    strncpy(next_class_subject, subj_tuple->value->cstring, 32);
    next_class_minutes = time_tuple->value->uint16;
    if (end_tuple) {
      next_class_verb = "ends";
    } else {
      next_class_verb = "begins";
    }
  }
  update_next_class_time(current_time);
}

static void handle_message_send_failed(DictionaryIterator *failed, AppMessageResult reason, void *context) {
  do_retry = 1;
  APP_LOG(APP_LOG_LEVEL_DEBUG, "FAIL: %d", reason);
  /* text_layer_set_text(tl_next_class_subject, "Please"); */
  /* text_layer_set_text(tl_next_class_time, "wait"); */
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
