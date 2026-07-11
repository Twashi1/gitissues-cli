#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

JAVA_SRC="$SCRIPT_DIR/java"
HEADER_OUT="$SCRIPT_DIR/include"

mkdir -p "$HEADER_OUT"

echo "Generating JNI headers..."

javac \
    -h "$HEADER_OUT" \
    "$JAVA_SRC/gitissues/GitIssues.java"

echo "Generated:"
ls -1 "$HEADER_OUT"
