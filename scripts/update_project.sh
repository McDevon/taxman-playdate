#!/bin/bash

cd "$(dirname "$0")"
cd ..

IFS='='
read -a strarr <<< "$(head -n 1 assets/pdxinfo)"
GAMENAME=$(echo "${strarr[1]}" | sed 's/ //g')

python scripts/tool_rebuild_project.py $GAMENAME
