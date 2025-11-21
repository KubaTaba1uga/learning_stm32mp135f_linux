#!/usr/bin/env bash

set -euo pipefail

if [ "$#" -ne 3 ]; then
    echo "Usage: $0 <name> <tag> <url>"
    exit 1
fi

NAME="$1"
TAG="$2"
URL="$3"

mkdir -p third_party

# Ensure we are inside a git repo
if ! git rev-parse --is-inside-work-tree >/dev/null 2>&1; then
    echo "Error: this script must be run inside a git repository."
    exit 1
fi

git status

git add . || true
git commit -m "WIP" || true

if git remote get-url "$NAME" >/dev/null 2>&1; then
    echo "Remote '$NAME' already exists, using existing remote."
else
    git remote add "$NAME" "$URL"
fi

git subtree add --prefix="third_party/$NAME" "$NAME" "$TAG" --squash
