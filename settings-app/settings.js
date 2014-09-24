var RU_API = '/ru/api/v1/';

var ruSpinnerTarget = document.getElementById("ru-spinner");
var ruSpinner = new Spinner({});

function splitTime(t) {
	return t.split(":").slice(0, 2).map(function (i) { return parseInt(i); });
}

function zeroPad(t) {
	return t < 10 ? "0" + t : "" + t;
}

function mergeTime(t) {
	return zeroPad(t[0]) + ":" + zeroPad(t[1]);
}

function arrSwap(arr, index, dir) {
	var nxt = index + dir;
	if (nxt < 0 || nxt > arr.length) {
	} else {
		var tmp = arr[index];
		arr.replace(index, arr[nxt]);
		arr.replace(nxt, tmp);
	}
}

var dayNames = ["Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"];
var initialSchedule;

if (document.location.hash == "") {
	initialSchedule = dayNames.map(function(d) { return {"day": d, "schedule": [{}]}; });
} else {
	var obj = JSON.parse(decodeURIComponent(document.location.hash.slice(1)));
	var root = Array.isArray(obj) ? obj : obj["schedules"];
	initialSchedule = root.map(function (e) {
		e["schedule"] = e["schedule"].map(function(s) {
			s["start"] = mergeTime(s["start"]);
			s["end"] = mergeTime(s["end"]);
			s["subj"] = s["subj"].replace("+", " ");
			return s;
		});
		return e;
	});
}

window.v = new Vue({
	el: "#events",
	data: {
		schedule: initialSchedule,
		days: dayNames.map(function(d, i) { return {"id": i, "name": d, "shortName": d[0]}; }),
		currentDay: {"id": 0, "name": "Monday"},
		isJSONVisible: false,
		ruLoadStage: 0,
		ruCities: [],
		json: ""
	},
	methods: {
		cancel: function(event) {
			document.location = "pebblejs://close";
		},
		save: function(event) {
			var fields = document.getElementsByTagName('input');
			for (var i = 0; i < fields.length; i++) {
				if (fields[i].checkValidity() === false) {
					fields[i].focus();
					alert("Invalid value: " + fields[i].value);
					return;
				}
			}
			var schedule = this.schedule.map(function(s) {
				s["schedule"] = s["schedule"].filter(function(e) {
					return typeof(e["subj"]) !== "undefined" && typeof(e["start"]) !== "undefined" && typeof(e["end"]) !== "undefined";
				}).map(function(e) {
					e["start"] = splitTime(e["start"]);
					e["end"] = splitTime(e["end"]);
					return e;
				}).sort(schedCompare);
				return s;
			});
			var data = JSON.stringify({"schedules": schedule});
			console.log(data);
			document.location = "pebblejs://close#" + encodeURIComponent(data);
		},
		add: function(event) {
			this.schedule[this.currentDay.id]["schedule"].push({});
		},
		remove: function(event) {
			this.schedule[this.currentDay.id]["schedule"].remove(parseInt(event.target.dataset.id));
		},
		moveUp: function(event) {
			arrSwap(this.schedule[this.currentDay.id]["schedule"], parseInt(event.target.dataset.id), -1);
		},
		moveDown: function(event) {
			arrSwap(this.schedule[this.currentDay.id]["schedule"], parseInt(event.target.dataset.id), 1);
		},
		openDay: function(day) {
			this.currentDay.id = day.id;
			this.currentDay.name = day.name;
		},
		backup: function(event) {
			this.json = JSON.stringify(this.schedule);
		},
		restore: function(event) {
			this.schedule = JSON.parse(this.json);
		},
		ruLoadCities: function(event) {
			ruGet.bind(this)("cities", function(data) {
				this.ruCities = data;
				this.ruLoadStage = 1;
			});
		},
		ruLoadUniversities: function(event) {
			ruGet.bind(this)('cities/' + this.ruSelectedCity + '/alluniversities', function(data) {
				this.ruUniversities = data.universities;
				this.ruLoadStage = 2;
			});
		},
		ruLoadFaculties: function(event) {
			ruGet.bind(this)('universities/' + this.ruSelectedUniversity + '/faculties', function(data) {
				this.ruFaculties = data;
				this.ruLoadStage = 3;
			});
		},
		ruLoadGroups: function(event) {
			ruGet.bind(this)('faculties/' + this.ruSelectedFaculty + '/groups', function(data) {
				this.ruGroups = data;
				this.ruLoadStage = 4;
			});
		},
		ruLoadSchedule: function(event) {
			ruGet.bind(this)('groups/' + this.ruSelectedGroup, function(data) {
				this.schedule = convertRuSchedule(data);
				this.backup();
				this.ruLoadStage = 5;
			});
		}
	}
});

function ruGet(url, callback) {
	ruSpinner.spin(ruSpinnerTarget);
	superagent.get(RU_API + url).timeout(10000).end(function(err, res) {
		ruSpinner.stop();
		if (err || res.error) this.ruLoadStage = -1;
		else {
			if (res.body.success) callback.bind(this)(res.body.data);
			else this.ruLoadStage = -1
		}
	}.bind(this));
}

function convertRuSchedule(data) {
	return data.days.map(function(day) {
		return {
			day: dayNames[day.weekday - 1],
			schedule: day.lessons.map(function(lesson) {
				var rooms = lesson.auditories.map(function(aud) { return aud.auditory_name });
				return {
					subj: (rooms.length > 0 ? rooms.join(',') + ' ' : '') + lesson.subject,
					start: lesson.time_start,
					end: lesson.time_end
				}
			}).sort(schedCompare)
		}
	});
}

function schedCompare(a, b) {
	return (a["start"][0] * 60 + a["start"][1]) - (b["start"][0] * 60 + b["start"][1]);
}
