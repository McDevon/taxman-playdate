#!/bin/bash

cd "$(dirname "$0")"
cd ..

python scripts/tool_create_scene.py $1
