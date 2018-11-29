package main

import (
	"crypto/rand"
	"database/sql"
	"encoding/binary"
	"encoding/json"
	"fmt"
	"io/ioutil"
	"net"
	"net/http"
	"net/url"
	"os"
	"os/signal"
	"strconv"
	"strings"
	"syscall"
	"time"

	"github.com/braintree/manners"
	"github.com/janekolszak/go-pebble"
	"github.com/jinzhu/now"
	"github.com/lemenkov/systemd.go"
	"github.com/akrylysov/algnhsa"
	_ "github.com/mattn/go-sqlite3"
)

func main() {
	handler := http.NewServeMux()
	handler.Handle("/static/", http.StripPrefix("/static", http.FileServer(http.Dir("./settings-app/"))))
	handler.HandleFunc("/import/myclassschedule", mcsImportHandler)
	handler.HandleFunc("/timeline", timelineHandler)
	if os.Getenv("AWS_LAMBDA_FUNCTION_NAME") != "" {
		algnhsa.ListenAndServe(handler, &algnhsa.Options {
			BinaryContentTypes: []string{"application/octet-stream", "application/x-sqlite3"},
		})
	} else {
		runOnSocket(handler)
	}
}

func runOnSocket(handler http.Handler) {
	var listener net.Listener
	var err error
	port := os.Getenv("PORT")
	sockets := systemd.ListenFds()
	if sockets == nil {
		listener, err = net.Listen("tcp", ":"+port)
		fmt.Println("Server started on port " + port)
	} else {
		listener, err = net.FileListener(sockets[0])
	}
	if err != nil {
		panic(err)
	}
	msrv := manners.NewWithServer(&http.Server{
		Addr:           ":" + port,
		Handler:        handler,
		ReadTimeout:    2 * time.Minute,
		WriteTimeout:   2 * time.Minute,
		MaxHeaderBytes: 1 << 20,
	})
	c := make(chan os.Signal, 1)
	signal.Notify(c, os.Interrupt, syscall.SIGTERM)
	go func() {
		for _ = range c {
			msrv.Close()
		}
	}()
	err = msrv.Serve(listener)
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

func timelineHandler(res http.ResponseWriter, req *http.Request) {
	req.Body = http.MaxBytesReader(res, req.Body, 64*1024)
	var classes []Class
	err := json.NewDecoder(req.Body).Decode(&classes)
	if err != nil {
		panic(err)
	}
	token := req.URL.Query().Get("token")
	year, month, day := now.MustParse(req.URL.Query().Get("date")).Date()
	tz_js, err := strconv.Atoi(req.URL.Query().Get("tz"))
	if err != nil {
		panic(err)
	}
	tz := time.FixedZone("", tz_js*-60) // minutes west of UTC -> seconds east of UTC
	client := &http.Client{}
	for _, class := range classes {
		start_t := now.MustParse(class.Start)
		end_t := now.MustParse(class.End)
		start := time.Date(year, month, day, start_t.Hour(), start_t.Minute(), 0, 0, tz)
		end := time.Date(year, month, day, end_t.Hour(), end_t.Minute(), 0, 0, tz)
		layout := pebble.Layout{
			Type:     "calendarPin",
			Title:    class.Subj,
			TinyIcon: "system://images/SCHEDULED_EVENT",
		}
		pin := pebble.Pin{
			Id:       fmt.Sprintf("%s%s%s", token[:32], start.Format("2006-01-02T15:04"), end.Format("2006-01-02T15:04")), // The id must be 64 chars!!
			Time:     start.Format(time.RFC3339),
			Duration: int(end.Sub(start).Minutes()),
			Layout:   &layout,
		}
		uPin := pebble.UserPin{
			Pin:   pin,
			Token: token,
		}
		uPin.Put(client)
	}
}
