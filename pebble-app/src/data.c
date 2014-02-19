#include <pebble.h>

static char next_class_subject[32];
static uint16_t next_class_minutes;
static char *next_class_verb = "xxxxxx";

enum {
  MSG_KEY_NOTHING = 0x0,
  MSG_KEY_TIME = 0x1,
  MSG_KEY_SUBJ = 0x2,
  MSG_KEY_END = 0x3,
  MSG_KEY_GET = 0x4
};

static uint8_t data_set_from_dict(DictionaryIterator* iter) {
  Tuple *nothing_tuple = dict_find(iter, MSG_KEY_NOTHING);
  if (nothing_tuple) {
    return 1;
  } else {
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
    return 0;
  }
}

static void data_request_from_phone() {
  Tuplet get_tuple = TupletInteger(MSG_KEY_GET, 1);
  DictionaryIterator *iter;
  if (app_message_outbox_begin(&iter) != APP_MSG_OK) return;
  if (dict_write_tuplet(iter, &get_tuple) != DICT_OK) return;
  app_message_outbox_send();
}
