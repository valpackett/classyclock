#include <pebble.h>
#include <stdbool.h>
#include <limits.h>
#include "util.h"

#define SCHED_LENGTH   36
// 36 = 18 beginnings + 18 endings
// I hope nobody actually has this much classes per day

#define SUBJECT_LENGTH 160
#define VERB_LENGTH    7 // "Begins\0"

// The keys at the beginning of the keyspace (0, 1...) are the stored schedule
// Same keys for storage and incoming message
#define KEY_WEEKDAY         INT_MAX
#define KEY_VIBRATE_MINUTES INT_MAX-1
#define KEY_COLOR_BG        INT_MAX-10
#define KEY_COLOR_CLOCK     INT_MAX-11
#define KEY_COLOR_DATE      INT_MAX-12
#define KEY_COLOR_TIMER     INT_MAX-13
#define KEY_COLOR_SUBJECT   INT_MAX-14

typedef struct {
	uint16_t minutes;
	bool is_nothing;
	char subject[SUBJECT_LENGTH];
	char verb[VERB_LENGTH];
} ClassEvent;

static ClassEvent schedule[SCHED_LENGTH];
static uint8_t schedule_weekday;
static uint8_t vibrate_minutes = 1;
#ifdef PBL_COLOR
static uint32_t color_bg      = 0xffaaaa;
static uint32_t color_clock   = 0x555500;
static uint32_t color_date    = 0x555500;
static uint32_t color_timer   = 0x555500;
static uint32_t color_subject = 0x555500;
#endif

enum {
	MSG_KEY_GET = 0x0
};

static void set_schedule_entry(uint8_t j, uint16_t minutes, char *subject) {
	schedule[j].is_nothing = false;
	classy_strlcpy(schedule[j].verb, j % 2 == 0 ? "Begins" : "Ends", VERB_LENGTH);
	schedule[j].minutes = minutes;
	classy_strlcpy(schedule[j].subject, subject, SUBJECT_LENGTH);
}

static void data_persist() {
	persist_write_int(KEY_WEEKDAY,         schedule_weekday);
	persist_write_int(KEY_VIBRATE_MINUTES, vibrate_minutes);
	// Signed/unsigned cast preserves the value! So it's fine.
#ifdef PBL_COLOR
	persist_write_int(KEY_COLOR_BG,        color_bg);
	persist_write_int(KEY_COLOR_CLOCK,     color_clock);
	persist_write_int(KEY_COLOR_DATE,      color_date);
	persist_write_int(KEY_COLOR_TIMER,     color_timer);
	persist_write_int(KEY_COLOR_SUBJECT,   color_subject);
#endif
	for (uint8_t i = 0; i < SCHED_LENGTH; i++)
		persist_write_data(i, &schedule[i], sizeof(schedule[i]));
}

static void data_read_persisted() {
	if (persist_exists(KEY_WEEKDAY))          schedule_weekday = persist_read_int(KEY_WEEKDAY);
	if (persist_exists(KEY_VIBRATE_MINUTES))  vibrate_minutes  = persist_read_int(KEY_VIBRATE_MINUTES);
#ifdef PBL_COLOR
	if (persist_exists(KEY_COLOR_BG     ))    color_bg         = persist_read_int(KEY_COLOR_BG);
	if (persist_exists(KEY_COLOR_CLOCK  ))    color_clock      = persist_read_int(KEY_COLOR_CLOCK);
	if (persist_exists(KEY_COLOR_DATE   ))    color_date       = persist_read_int(KEY_COLOR_DATE);
	if (persist_exists(KEY_COLOR_TIMER  ))    color_timer      = persist_read_int(KEY_COLOR_TIMER);
	if (persist_exists(KEY_COLOR_SUBJECT))    color_subject    = persist_read_int(KEY_COLOR_SUBJECT);
#endif
	for (uint8_t i = 0; i < SCHED_LENGTH; i++)
		if (persist_exists(i))
			persist_read_data(i, &schedule[i], sizeof(schedule[i]));
}

static uint16_t extract_number(char *s, uint8_t from, uint8_t len) {
	char buf[len + 1];
	classy_strlcpy(buf, s + from, len + 1);
	return atoi(buf);
}

static void data_set_from_dict(DictionaryIterator* iter, struct tm *cur_time) {
	memset(&schedule, 0, sizeof(schedule));
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
	Tuple *vib         ; if ((vib         = dict_find(iter, KEY_VIBRATE_MINUTES )))   vibrate_minutes = (uint8_t)vib->value->int32;
#ifdef PBL_COLOR
	Tuple *tup_bg      ; if ((tup_bg      = dict_find(iter, KEY_COLOR_BG        )))   color_bg        = (uint32_t)tup_bg->value->int32;
	Tuple *tup_clock   ; if ((tup_clock   = dict_find(iter, KEY_COLOR_CLOCK     )))   color_clock     = (uint32_t)tup_clock->value->int32;
	Tuple *tup_date    ; if ((tup_date    = dict_find(iter, KEY_COLOR_DATE      )))   color_date      = (uint32_t)tup_date->value->int32;
	Tuple *tup_timer   ; if ((tup_timer   = dict_find(iter, KEY_COLOR_TIMER     )))   color_timer     = (uint32_t)tup_timer->value->int32;
	Tuple *tup_subject ; if ((tup_subject = dict_find(iter, KEY_COLOR_SUBJECT   )))   color_subject   = (uint32_t)tup_subject->value->int32;
#endif
	data_persist();
}

static ClassEvent data_next_event(uint16_t current_minutes) {
	for (uint8_t i = 0; i < SCHED_LENGTH; i++)
		if (schedule[i].minutes > current_minutes)
			return schedule[i];
	return (ClassEvent) { .is_nothing = true, .minutes = 0, .subject = "Updating", .verb = "begins" };
}

static void data_request_from_phone() {
	Tuplet get_tuple = TupletInteger(MSG_KEY_GET, 1);
	DictionaryIterator *iter;
	if (app_message_outbox_begin(&iter) != APP_MSG_OK) return;
	if (dict_write_tuplet(iter, &get_tuple) != DICT_OK) return;
	app_message_outbox_send();
}
