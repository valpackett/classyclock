package main

import (
	"fmt"
	"io/ioutil"
	"net/http"
	"os"
	/* "database/sql" */ /* _ "github.com/mattn/go-sqlite3" */)

func main() {
	var port = os.Getenv("PORT")
	var settingsHtml []byte
	settingsHtml, errr := ioutil.ReadFile("settings.html")
	if errr != nil {
		panic(errr)
	}
	var hh = makeHtmlHandler(settingsHtml)
	http.HandleFunc("/", hh)
	fmt.Println("Server started on port " + port)
	err := http.ListenAndServe(":"+port, nil)
	if err != nil {
		panic(err)
	}
}

func makeHtmlHandler(html []byte) func(res http.ResponseWriter, req *http.Request) {
	return func(res http.ResponseWriter, req *http.Request) {
		res.Write(html)
	}
}
