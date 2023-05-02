#!/bin/bash

cd "$MESON_SOURCE_ROOT"

[ -d .git ] || exit 2 # not a git repo
[ ! -f .git/hooks/pre-commit ] || exit 2 # already installed

cp data/pre-commit-hook .git/hooks/pre-commit
cp data/canonicalize_filename.sh .git/hooks/canonicalize_filename.sh
chmod +x .git/hooks/pre-commit
echo "Activated pre-commit hook"