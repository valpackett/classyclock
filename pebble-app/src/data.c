#include <pebble.h>
#include <stdbool.h>

typedef struct {
  char *subject;
  char *verb;
  uint16_t minutes;
  bool is_nothing;
} ClassEvent;

static ClassEvent next_class_event;

enum {
  MSG_KEY_NOTHING = 0x0,
  MSG_KEY_TIME = 0x1,
  MSG_KEY_SUBJ = 0x2,
  MSG_KEY_END = 0x3,
  MSG_KEY_GET = 0x4
};

static void data_set_from_dict(DictionaryIterator* iter) {
  Tuple *nothing_tuple = dict_find(iter, MSG_KEY_NOTHING);
  if (nothing_tuple) {
    next_class_event.is_nothing = true;
  } else {
    Tuple *subj_tuple = dict_find(iter, MSG_KEY_SUBJ);
    Tuple *time_tuple = dict_find(iter, MSG_KEY_TIME);
    Tuple *end_tuple = dict_find(iter, MSG_KEY_END);
    next_class_event.is_nothing = false;
    next_class_event.verb = end_tuple ? "ends" : "begins";
    next_class_event.minutes = time_tuple->value->uint16;
    next_class_event.subject = "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
    strncpy(next_class_event.subject, subj_tuple->value->cstring, 32);
  }
}

static ClassEvent data_next_class_event() {
  return next_class_event;
}

static void data_request_from_phone() {
  Tuplet get_tuple = TupletInteger(MSG_KEY_GET, 1);
  DictionaryIterator *iter;
  if (app_message_outbox_begin(&iter) != APP_MSG_OK) return;
  if (dict_write_tuplet(iter, &get_tuple) != DICT_OK) return;
  app_message_outbox_send();
}
