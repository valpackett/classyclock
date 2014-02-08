var SCHED_KEY = "schedule";

if (!localStorage.getItem(SCHED_KEY)) {
  localStorage.setItem(SCHED_KEY, [
    {
      "start": [23, 58],
      "end": [23, 59],
      "subj": "Edit schedule on phone"
    }
  ]);
}

function flattenTime(time) {
  return time[0] * 60 + time[1];
}

function flattenSchedule(schedule) {
  var result = [];
  schedule.forEach(function(entry) {
    result.push({
      "time": flattenTime(entry["start"]),
      "subj": entry["subj"],
    });
    result.push({
      "time": flattenTime(entry["end"]),
      "subj": entry["subj"],
      "end": true
    });
  });
  return result;
}

function getNextEvent(flat_schedule) {
  var cur_date = new Date(),
      cur_time = cur_date.getHours() * 60 + cur_date.getMinutes(),
      result = {"nothing": true};
  flat_schedule.forEach(function (entry) {
    if (result["nothing"] && entry["time"] > cur_time) {
      result = entry;
    }
  });
  console.log("Time: " + result["time"] + " End: " + result["end"]);
  return result;
}

function sendNextEvent() {
  Pebble.sendAppMessage(getNextEvent(flattenSchedule(localStorage.getItem(SCHED_KEY))));
}

Pebble.addEventListener("ready", sendNextEvent);

Pebble.addEventListener("appmessage", function(e) {
  if (e.payload.get) {
    sendNextEvent();
  }
});

Pebble.addEventListener("showConfiguration", function(e) {
  Pebble.openURL("http://myfreeweb.github.io/classyclock/#" + encodeURIComponent(JSON.stringify(localStorage.getItem(SCHED_KEY))));
});

Pebble.addEventListener("webviewclosed", function(e) {
  localStorage.setItem(SCHED_KEY, JSON.parse(decodeURIComponent(e.response)));
  sendNextEvent();
});
