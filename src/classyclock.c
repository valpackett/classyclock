#include <pebble.h>

static Window *window;
static TextLayer *tl_current_time;
static TextLayer *tl_current_date;
static TextLayer *tl_next_class_subject;
static TextLayer *tl_next_class_time;
static char next_class_subject[32];
static char next_class_time[32];
static uint8_t current_minutes;
static int8_t next_class_minutes_left;
static char *next_class_verb;

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

static void update_next_class_time() {
  snprintf(next_class_time, 32, "%s in %d min.", next_class_verb, next_class_minutes_left);
  text_layer_set_text(tl_next_class_time, next_class_time);
}

static void send_message_get(void) {
  Tuplet get_tuple = TupletInteger(MSG_KEY_GET, 1);
  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);
  if (iter == NULL) return;
  dict_write_tuplet(iter, &get_tuple);
  app_message_outbox_send();
}

static void handle_minute_tick(struct tm *tick_time, TimeUnits units_changed) {
  static char time_text[] = "00:00";
  strftime(time_text, sizeof(time_text), "%R", tick_time);
  text_layer_set_text(tl_current_time, time_text);

  static char date_text[] = "Xxxxxxxxx 00";
  strftime(date_text, sizeof(date_text), "%B %e", tick_time);
  text_layer_set_text(tl_current_date, date_text);

  current_minutes = tick_time->tm_hour * 60 + tick_time->tm_min;
  next_class_minutes_left -= 1;
  if (next_class_minutes_left <= 0) {
    send_message_get();
  } else {
    update_next_class_time();
  }
}

static void handle_message_receive(DictionaryIterator *iter, void *context) {
  Tuple *nothing_tuple = dict_find(iter, MSG_KEY_NOTHING);
  if (nothing_tuple) {
    text_layer_set_text(tl_next_class_subject, "No more classes today.");
    text_layer_set_text(tl_next_class_time, "See you tomorrow.");
  } else {
    Tuple *subj_tuple = dict_find(iter, MSG_KEY_SUBJ);
    Tuple *time_tuple = dict_find(iter, MSG_KEY_TIME);
    Tuple *end_tuple = dict_find(iter, MSG_KEY_END);
    strncpy(next_class_subject, subj_tuple->value->cstring, 32);
    text_layer_set_text(tl_next_class_subject, next_class_subject);
    next_class_minutes_left = time_tuple->value->uint8 - current_minutes;
    if (end_tuple) {
      next_class_verb = "ends";
    } else {
      next_class_verb = "begins";
    }
    update_next_class_time();
  }
}

static void handle_init(void) {
  window = window_create();
  window_set_background_color(window, GColorBlack);
  window_set_window_handlers(window, (WindowHandlers) {
    .load = handle_window_load,
    .unload = handle_window_unload,
  });
  tick_timer_service_subscribe(MINUTE_UNIT, handle_minute_tick);
  app_message_register_inbox_received(handle_message_receive);
  app_message_open(/* inbound_size: */ 64, /* outbound_size: */ 64);
  window_stack_push(window, /* animated: */ true);
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
