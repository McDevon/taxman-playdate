#!/bin/bash

cd "$(dirname "$0")"
cd ..

python scripts/tool_create_game_object.py $1
