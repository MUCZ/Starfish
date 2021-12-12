package main

import "net/http"

func main(){
	http.HandleFunc("/hello",func(w http.ResponseWriter, r *http.Request){
		w.Write([]byte("hello, world!\n"))
	})
	http.HandleFunc("/bye",func(w http.ResponseWriter, r *http.Request){
		w.Write([]byte("bye!\n"))
	})
	http.ListenAndServe(":80",nil)
}