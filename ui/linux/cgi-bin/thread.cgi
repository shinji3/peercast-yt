#!/usr/bin/env python3
# -*- coding: utf-8 -*-
import cgi, json, sys

sys.path.append('cgi-bin')

import bbs_reader

form = cgi.FieldStorage()

if "category" not in form or "board_num" not in form or "id" not in form:
  common.print_bad_request("bad parameter")
  sys.exit()

if "first" not in form:
  first = 1
else:
  first = int(form['first'].value)

board = bbs_reader.Board(form["category"].value, form["board_num"].value)
thread = board.thread(form["id"].value)

posts = [{
  "no":    post.no,
  "name":  post.name,
  "mail":  post.mail,
  "body":  post.body,
  "date":  post.date
} for post in thread.posts(range(first, 1000))]

print("Content-Type: text/json; charset=UTF-8\n")
print(json.dumps({
  "status": "ok",
  "id":     thread.id,
  "title":  thread.title,
  "last":   thread.last,
  "posts":  posts
}))
