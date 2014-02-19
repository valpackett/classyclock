#include <pebble.h>
#include <stdbool.h>

#define SCHED_LENGTH 20
// 20 = 10 beginnings + 10 endings
// I hope nobody actually has more than 10 classes per day

typedef struct {
  char *subject;
  char *verb;
  uint16_t minutes;
  bool is_nothing;
} ClassEvent;

static ClassEvent schedule[SCHED_LENGTH];
static uint8_t schedule_weekday;

enum {
  MSG_KEY_GET = 0x0
};

static void set_schedule_entry(uint8_t j, uint16_t minutes, char *subject) {
  schedule[j].is_nothing = false;
  schedule[j].verb = j % 2 == 0 ? "begins" : "ends";
  schedule[j].minutes = minutes;
  schedule[j].subject = "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
  strncpy(schedule[j].subject, subject, 32);
  /* APP_LOG(APP_LOG_LEVEL_DEBUG, "schedule [%d] = %s %s in %d", j, schedule[j].subject, schedule[j].verb, schedule[j].minutes); */
}

static uint16_t extract_number(char *s, uint8_t from, uint8_t len) {
  char buf[len];
  strncpy(buf, s + from, len);
  return atoi(buf);
}

static void data_set_from_dict(DictionaryIterator* iter, struct tm *cur_time) {
  schedule_weekday = cur_time->tm_wday;
  uint8_t j = 0;
  for (uint8_t i = 1; i <= SCHED_LENGTH; i++) {
    Tuple *t = dict_find(iter, i);
    if (t) {
      set_schedule_entry(j,   extract_number(t->value->cstring, 0, 4), t->value->cstring + 8);
      set_schedule_entry(++j, extract_number(t->value->cstring, 4, 4), t->value->cstring + 8);
    }
    j++;
  }
}

static ClassEvent data_next_class_event(uint16_t current_minutes) {
  for (uint8_t i = 0; i < SCHED_LENGTH; i++) {
    if (schedule[i].minutes > current_minutes) return schedule[i];
  }
  return (ClassEvent) { .is_nothing = true, .minutes = 0, .subject = "Updating", .verb = "begins" };
}

static void data_request_from_phone() {
  Tuplet get_tuple = TupletInteger(MSG_KEY_GET, 1);
  DictionaryIterator *iter;
  if (app_message_outbox_begin(&iter) != APP_MSG_OK) return;
  if (dict_write_tuplet(iter, &get_tuple) != DICT_OK) return;
  app_message_outbox_send();
}
