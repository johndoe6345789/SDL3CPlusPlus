#!/bin/bash
# Simple wrapper script - runs executable from the same folder
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
"$DIR/build/standalone_workflow_cubes"
