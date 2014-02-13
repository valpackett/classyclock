package main

import (
	"crypto/rand"
	"database/sql"
	"encoding/binary"
	"encoding/json"
	"fmt"
	_ "github.com/mattn/go-sqlite3"
	"io/ioutil"
	"net/http"
	"net/url"
	"os"
	"strings"
)

func main() {
	var port = os.Getenv("PORT")
	var settingsHtml []byte
	settingsHtml, err := ioutil.ReadFile("settings.html")
	if err != nil {
		panic(err)
	}
	var hh = makeHtmlHandler(settingsHtml)
	http.HandleFunc("/", hh)
	http.HandleFunc("/import/myclassschedule", mcsImportHandler)
	fmt.Println("Server started on port " + port)
	err = http.ListenAndServe(":"+port, nil)
	if err != nil {
		panic(err)
	}
}

func makeHtmlHandler(html []byte) func(res http.ResponseWriter, req *http.Request) {
	return func(res http.ResponseWriter, req *http.Request) {
		res.Write(html)
	}
}

type Class struct {
	Start []int  `json:"start"`
	End   []int  `json:"end"`
	Subj  string `json:"subj"`
}

type Day struct {
	Day      string  `json:"day"`
	Schedule []Class `json:"schedule"`
}

func mcsImportHandler(res http.ResponseWriter, req *http.Request) {
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
		start := []int{from_time / 60, from_time % 60}
		end := []int{to_time / 60, from_time % 60}
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
	jsonBytes, _ := json.Marshal([]Day{mon, tue, wed, thu, fri, sat, sun})
	var scheme string
	if strings.Contains(req.Host, "localhost") {
		scheme = "http"
	} else {
		scheme = "https"
	}
	res.Header().Set("Location", fmt.Sprintf("%s://%s/#%s", scheme, req.Host, url.QueryEscape(string(jsonBytes))))
	res.WriteHeader(http.StatusFound)
}
