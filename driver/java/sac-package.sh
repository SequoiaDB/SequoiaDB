#!/bin/bash

VERSION="1.0.0"
MODE="package"
RELEASE=false

function show_help {
  echo "Package sac sequoiadb java driver."
  echo "'deploy' mode will publish to the internal repository, please set up the settings file in advance."
  echo ""
  echo "Usage: sac-package.sh [OPTIONS]"
  echo ""
  echo "Options:"
  echo "  -v, --version    Set the package version"
  echo "  -m, --mode       Set the packaging mode, 'package' or 'deploy', default is 'package'"
  echo "  -r, --release    Whether to package as a release version, default is SNAPSHOT"
  echo "  -h, --help       Display this help message"
}

while [[ $# -gt 0 ]]; do
  case "$1" in
  -v | --version)
    VERSION="$2"
    shift
    ;;
  -m | --mode)
    MODE="$2"
    if [[ "$MODE" != "deploy" && "$MODE" != "package" ]]; then
      echo "Error: Invalid mode specified. Mode must be 'deploy' or 'package'."
      exit 1
    fi
    shift
    ;;
  -r | --release)
    RELEASE=true
    shift
    ;;
  -h | --help)
    show_help
    exit 0
    ;;
  *)
    echo "Invalid option: $1"
    exit 1
    ;;
  esac
  shift
done

mv src/main/java/org/bson src/main/java/com/sequoiadb/
rm -rf src/main/java/org
find src -type f -exec sed -i 's/org\.bson/com\.sequoiadb\.bson/g' {} \;

if $RELEASE; then
  new_version="$VERSION-sac"
else
  new_version="$VERSION-sac-SNAPSHOT"
fi

mvn -DnewVersion=$new_version versions:set
mvn clean $MODE -Dmaven.test.skip=true
