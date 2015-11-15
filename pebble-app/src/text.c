#include <pebble.h>

#define NEXT_CLASS_TIME_SIZE 32

static char* format_time(struct tm *tick_time) {
	static char time_text[] = "00:00";
	static char *time_format;
	if (clock_is_24h_style()) {
		time_format = "%R";
	} else {
		time_format = "%I:%M";
	}
	strftime(time_text, sizeof(time_text), time_format, tick_time);
	if (!clock_is_24h_style() && (time_text[0] == '0')) {
		// Remove the leading zero when 12-hour clock is used
		memmove(time_text, &time_text[1], sizeof(time_text) - 1);
	}
	return time_text;
}

static char* format_date(struct tm *tick_time) {
	static char date_text[] = "Xxxxxxxxx 00";
	strftime(date_text, sizeof(date_text), "%a %b %e", tick_time);
	return date_text;
}

static char* format_next_class_time(int next_class_minutes_left, char* next_class_verb) {
	static char next_class_time[NEXT_CLASS_TIME_SIZE];
	if (next_class_minutes_left > 60) {
		snprintf(next_class_time, NEXT_CLASS_TIME_SIZE, "%s in %dh%dm:", next_class_verb, next_class_minutes_left / 60, next_class_minutes_left % 60);
	} else {
		snprintf(next_class_time, NEXT_CLASS_TIME_SIZE, "%s in %d min:", next_class_verb, next_class_minutes_left);
	}
	return next_class_time;
}
