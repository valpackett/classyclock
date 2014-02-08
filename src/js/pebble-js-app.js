var schedule = [
  {
    "start": [20, 10],
    "end": [20, 30],
    "subj": "Rocket Science"
  }
];

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

Pebble.addEventListener("ready", function(e) {
  Pebble.sendAppMessage(getNextEvent(flattenSchedule(schedule)));
});

Pebble.addEventListener("appmessage", function(e) {
  if (e.payload.get) {
    Pebble.sendAppMessage(getNextEvent(flattenSchedule(schedule)));
  }
});

