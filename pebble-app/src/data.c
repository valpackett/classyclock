#include <pebble.h>
#include <stdbool.h>
#include <limits.h>
#include "util.c"

#define SCHED_LENGTH 40
// 40 = 20 beginnings + 20 endings
// I hope nobody actually has more than 20 classes per day

#define SUBJECT_LENGTH 32
#define VERB_LENGTH 7

typedef struct {
  uint16_t minutes;
  bool is_nothing;
  char subject[SUBJECT_LENGTH];
  char verb[VERB_LENGTH];
} ClassEvent;

static ClassEvent schedule[SCHED_LENGTH];
static uint8_t schedule_weekday;

enum {
  MSG_KEY_GET = 0x0
};

static void set_schedule_entry(uint8_t j, uint16_t minutes, char *subject) {
  schedule[j].is_nothing = false;
  fucking_copy_string(schedule[j].verb, j % 2 == 0 ? "begins" : "ends", VERB_LENGTH);
  schedule[j].minutes = minutes;
  fucking_copy_string(schedule[j].subject, subject, SUBJECT_LENGTH);
}

static void data_persist() {
  persist_write_int(INT_MAX, schedule_weekday);
  for (uint8_t i = 0; i < SCHED_LENGTH; i++) {
    persist_write_data(i, &schedule[i], sizeof(schedule[i]));
  }
}

static void data_read_persisted() {
  if (persist_exists(INT_MAX)) {
    schedule_weekday = persist_read_int(INT_MAX);
  }
  for (uint8_t i = 0; i < SCHED_LENGTH; i++) {
    if (persist_exists(i)) persist_read_data(i, &schedule[i], sizeof(schedule[i]));
  }
}

static uint16_t extract_number(char *s, uint8_t from, uint8_t len) {
  char buf[len]; // I don't have any fucking idea why adding +1 for the \0 fucks everything up
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
  data_persist();
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
