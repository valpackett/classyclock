package main

import (
	"crypto/rand"
	"database/sql"
	"encoding/binary"
	"encoding/json"
	"fmt"
	"io/ioutil"
	"net/http"
	"net/url"
	"os"
	"strings"

	now "github.com/jinzhu/now"
	_ "github.com/mattn/go-sqlite3"
)

func main() {
	var port = os.Getenv("PORT")
	http.Handle("/static/", http.StripPrefix("/static", http.FileServer(http.Dir("./settings-app/"))))
	http.HandleFunc("/import/myclassschedule", mcsImportHandler)
	http.HandleFunc("/import/ruz", ruzImportHandler)
	fmt.Println("Server started on port " + port)
	err := http.ListenAndServe(":"+port, nil)
	if err != nil {
		panic(err)
	}
}

type Class struct {
	Start string `json:"start"`
	End   string `json:"end"`
	Subj  string `json:"subj"`
}

type Day struct {
	Day      string  `json:"day"`
	Schedule []Class `json:"schedule"`
}

func respondWithSchedule(res http.ResponseWriter, req *http.Request, days []Day) {
	jsonBytes, _ := json.Marshal(days)
	var scheme string
	if strings.Contains(req.Host, "localhost") || strings.Contains(req.Host, "192.168") {
		scheme = "http"
	} else {
		scheme = "https"
	}
	res.Header().Set("Location", fmt.Sprintf("%s://%s%s/static/settings.html#%s", scheme, req.Host, os.Getenv("URL_PREFIX"), url.QueryEscape(string(jsonBytes))))
	res.WriteHeader(http.StatusFound)
}

func mcsImportHandler(res http.ResponseWriter, req *http.Request) {
	req.Body = http.MaxBytesReader(res, req.Body, 640*1024)
	dbFile, _, _ := req.FormFile("database")
	dbBytes, _ := ioutil.ReadAll(dbFile)
	var randNum int32
	binary.Read(rand.Reader, binary.LittleEndian, &randNum)
	dbPath := fmt.Sprintf("/tmp/tmpdb_%d.sqlite", randNum)
	ioutil.WriteFile(dbPath, dbBytes, os.FileMode(0600))
	defer os.Remove(dbPath)
	db, _ := sql.Open("sqlite3", dbPath)
	defer db.Close()
	rows, _ := db.Query("SELECT hours.from_time, hours.to_time, hours.monday, hours.tuesday, hours.wednesday, hours.thursday, hours.friday, hours.saturday, hours.sunday, courses.name FROM hours LEFT JOIN courses ON hours.course_uuid = courses.uuid WHERE hours.is_deleted = 0 AND courses.is_deleted = 0;")
	defer rows.Close()
	mon := Day{"Monday", make([]Class, 0)}
	tue := Day{"Tuesday", make([]Class, 0)}
	wed := Day{"Wednesday", make([]Class, 0)}
	thu := Day{"Thursday", make([]Class, 0)}
	fri := Day{"Friday", make([]Class, 0)}
	sat := Day{"Saturday", make([]Class, 0)}
	sun := Day{"Sunday", make([]Class, 0)}
	for rows.Next() {
		var from_time, to_time, monday, tuesday, wednesday, thursday, friday, saturday, sunday int
		var name string
		rows.Scan(&from_time, &to_time, &monday, &tuesday, &wednesday, &thursday, &friday, &saturday, &sunday, &name)
		start := fmt.Sprintf("%02d:%02d", from_time/60, from_time%60)
		end := fmt.Sprintf("%02d:%02d", to_time/60, to_time%60)
		class := Class{start, end, name}
		if monday == 1 {
			mon.Schedule = append(mon.Schedule, class)
		}
		if tuesday == 1 {
			tue.Schedule = append(tue.Schedule, class)
		}
		if wednesday == 1 {
			wed.Schedule = append(wed.Schedule, class)
		}
		if thursday == 1 {
			thu.Schedule = append(thu.Schedule, class)
		}
		if friday == 1 {
			fri.Schedule = append(fri.Schedule, class)
		}
		if saturday == 1 {
			sat.Schedule = append(sat.Schedule, class)
		}
		if sunday == 1 {
			sun.Schedule = append(sun.Schedule, class)
		}
	}
	rows.Close()
	respondWithSchedule(res, req, []Day{mon, tue, wed, thu, fri, sat, sun})
}

type RuzClass struct {
	Room  string `json:"auditorium"`
	Subj  string `json:"discipline"`
	Start string `json:"beginLesson"`
	End   string `json:"endLesson"`
	Day   int    `json:"dayOfWeek"`
}

func ruzImportHandler(res http.ResponseWriter, req *http.Request) {
	now.FirstDayMonday = true
	query := req.URL.Query()
	params := url.Values{}
	params.Add("fromdate", now.BeginningOfWeek().Format("2006.01.02"))
	params.Add("todate", now.EndOfWeek().Format("2006.01.02"))
	params.Add("email", query.Get("email"))
	ruzreq, err := http.NewRequest("GET", "http://ruz.hse.ru/RUZService.svc/personlessons?"+params.Encode(), nil)
	ruzreq.Header.Add("User-Agent", "Mozilla/5.0 (Linux; Android 5.1.1; One Build/LRX22C.H3) AppleWebKit/537.36 (KHTML, like Gecko) Version/4.0 Chrome/39.0.0.0 Mobile Safari/537.36")
	ruzreq.Header.Add("Accept", "application/json, text/plain, */*")
	ruzreq.Header.Add("X-Requested-With", "ru.hse.ruz")
	client := &http.Client{}
	ruzres, err := client.Do(ruzreq)
	defer ruzres.Body.Close()
	if err != nil {
		panic(err)
	}
	var rclasses []RuzClass
	err = json.NewDecoder(ruzres.Body).Decode(&rclasses)
	if err != nil {
		panic(err)
	}
	mon := Day{"Monday", make([]Class, 0)}
	tue := Day{"Tuesday", make([]Class, 0)}
	wed := Day{"Wednesday", make([]Class, 0)}
	thu := Day{"Thursday", make([]Class, 0)}
	fri := Day{"Friday", make([]Class, 0)}
	sat := Day{"Saturday", make([]Class, 0)}
	sun := Day{"Sunday", make([]Class, 0)}
	for _, rclass := range rclasses {
		class := Class{rclass.Start, rclass.End, rclass.Room + " " + rclass.Subj}
		switch rclass.Day {
		case 1:
			mon.Schedule = append(mon.Schedule, class)
		case 2:
			tue.Schedule = append(tue.Schedule, class)
		case 3:
			wed.Schedule = append(wed.Schedule, class)
		case 4:
			thu.Schedule = append(thu.Schedule, class)
		case 5:
			fri.Schedule = append(fri.Schedule, class)
		case 6:
			sat.Schedule = append(sat.Schedule, class)
		case 7:
			sun.Schedule = append(sun.Schedule, class)
		}
	}
	respondWithSchedule(res, req, []Day{mon, tue, wed, thu, fri, sat, sun})
}
