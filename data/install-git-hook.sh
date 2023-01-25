#!/bin/bash

cd "$MESON_SOURCE_ROOT"

[ -d .git ] || exit 0 # not a git repo
[ ! -f .git/hooks/pre-commit ] || exit 0 # already installed

echo "Copying pre commit hook"
cp data/pre-commit-hook .git/hooks/pre-commit
echo "Copying helper"
cp data/canonicalize_filename.sh .git/hooks/canonicalize_filename.sh
echo "Done"