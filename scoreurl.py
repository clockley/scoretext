#!/usr/bin/env python3.6
'''
    Copyright 2019 Christian Lockley. All rights reserved.
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
'''
import io
import json
import sys
from difflib import SequenceMatcher
import re
import justext
import requests
import urllib.request
import os
from inscriptis import get_text
from unidecode import unidecode
import magic
import struct
import base64

headers = {
    'User-Agent': 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/75.0.3770.100 Safari/537.36 readabilitychecker.com',
}

def similar(a, b):
    return SequenceMatcher(None, a, b).ratio()

stopList = justext.get_stoplist("English")

algo1 =  ""
algo2 =  ""
line  =  ""
while True:
    try:
        line = input()
    except:
        sys.exit()

    if not (line.startswith("http") or line.startswith("https")):
        line = "http://"+line
    try:
        article = requests.get("http://localhost:3000/parse?url="+line, headers=headers)
    except:
        algo1 = ""
        algo2 = ""
        print("\n\0EOF")
        continue
    parsed_json = article.json()
    if parsed_json['success'] == True:
        algo1 = get_text(parsed_json["article"]["content"])


    try:
        article = requests.get(line, headers=headers)
    except:
        algo1 = ""
        algo2 = ""
        print("\n\0EOF")
        continue
    try:
        paragraphs = justext.justext(article.content, stopList)
    except:
        algo1 = ""
        algo2 = ""
        print("\n\0EOF")
        continue

    for paragraph in paragraphs:
        if not paragraph.is_boilerplate:
            algo2 += paragraph.text+"\n\n"


    #print(unidecode(algo1)+"algo2")

    if similar(algo1, algo2) >= .5:
        for s in algo1.splitlines():
            print(unidecode(s).strip())
    else:
        if not algo2.strip() and magic.from_buffer(parsed_json["article"]["textContent"], mime=True) == "text/plain":
            print(unidecode(parsed_json["article"]["textContent"]))
        elif not algo2.strip() and magic.from_buffer(parsed_json["article"]["textContent"], mime=True) != "text/plain":
            sys.stdout.buffer.write(base64.b64encode(article.content))
            print("\n\0B64")
            algo1 = ""
            algo2 = ""
            continue
        else:
            print(unidecode(algo2))

    algo1 = ""
    algo2 = ""
    print("\n\0EOF")


