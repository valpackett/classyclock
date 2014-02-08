function getSchedule() {
  var ls = localStorage.getItem("x");
  if (ls !== null) return JSON.parse(ls);
  return [
    {
      "start": [23, 58],
      "end": [23, 59],
      "subj": "Edit schedule on phone"
    }
  ];
}

function setSchedule(s) {
  return localStorage.setItem("x", JSON.stringify(s));
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
  Pebble.sendAppMessage(getNextEvent(flattenSchedule(getSchedule())),
    function(e) {
      console.log("Successfully delivered message with transactionId="
                  + e.data.transactionId);
    },
    function(e) {
      console.log("Unable to deliver message with transactionId="
                  + e.data.transactionId
                  + " Error is: " + e.error.message);
    });
}

Pebble.addEventListener("ready", function(e) { });

Pebble.addEventListener("appmessage", function(e) {
  sendNextEvent();
});

Pebble.addEventListener("showConfiguration", function(e) {
  Pebble.openURL("http://myfreeweb.github.io/classyclock/#" + encodeURIComponent(JSON.stringify(getSchedule())));
});

Pebble.addEventListener("webviewclosed", function(e) {
  setSchedule(JSON.parse(decodeURIComponent(e.response)));
  sendNextEvent();
});
