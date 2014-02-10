var defaultSchedule = [
    {
      "start": [23, 58],
      "end": [23, 59],
      "subj": "Edit schedule on phone"
    }
  ];

function getSchedule() {
  var ls = localStorage.getItem("y");
  if (ls !== null) return JSON.parse(ls)["schedule"] || defaultSchedule;
  return defaultSchedule;
}

function setSchedule(s) {
  return localStorage.setItem("y", JSON.stringify({"schedule": s}));
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
  return result;
}

function sendNextEvent() {
  Pebble.sendAppMessage(getNextEvent(flattenSchedule(getSchedule())),
    function(e) {
      console.log("Successfully delivered message with transactionId=" + e.data.transactionId);
    },
    function(e) {
      console.log("Unable to deliver message with transactionId=" + e.data.transactionId + " Error is: " + e.error.message);
    });
}

Pebble.addEventListener("ready", function(e) {
  console.log("READY. Event: " + JSON.stringify(e) + " Sched: " + JSON.stringify(getSchedule()));
  sendNextEvent();
});

Pebble.addEventListener("appmessage", function(e) {
  console.log("APPMESSAGE. Event: " + JSON.stringify(e));
  if (e.payload.get) sendNextEvent();
});

Pebble.addEventListener("showConfiguration", function(e) {
  Pebble.openURL("http://myfreeweb.github.io/classyclock/#" + encodeURIComponent(JSON.stringify({"schedule": getSchedule()})));
});

Pebble.addEventListener("webviewclosed", function(e) {
  var rsp = JSON.parse(decodeURIComponent(e.response));
  if (typeof rsp === "object") {
    setSchedule(rsp["schedule"]);
    sendNextEvent();
  }
});
