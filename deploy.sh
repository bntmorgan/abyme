#!/bin/bash
BASE=`pwd`
PYTHON=python3
MODULES=modules
DESTINATION=sources

map_folder() {
  local DIR=$1
  local FUNCTION=$2
  local FOLDERS=`ls $MODULES$DIR`
  local f
  echo -e "  "$FOLDERS
  echo
  local TAB="$3."
  for f in $FOLDERS; do
    $FUNCTION $DIR/$f $TAB
  done
}

do_deploy() {
  if [ -f $MODULES$1 ]; then
    echo $2 Deploiement de : $MODULES$1
    if [ "${1#*.}" = "w" ]; then
      filename=`basename $1`
      if [ "${filename%.*}"  = "root" ]; then
        echo $2 "=>" Fichier web racine
        python3.3 literale/prepare.py -b `dirname $MODULES$1` `basename $1` | python3.3 literale/tangle.py -d `dirname $DESTINATION$1`
      fi
    else
      cp -fpr $MODULES$1 $DESTINATION$1
    fi
  fi
  if [ -d $MODULES$1 ]; then
    echo $2 Deploiement de : $MODULES$1/
    mkdir -p $DESTINATION$1
    map_folder $1 do_deploy $2
  fi
}

do_all() {
  echo Dossier de travail : $MODULES
  echo Dossier destination : $DESTINATION
  echo
  echo Deploiement de : $MODULES
  map_folder "" do_deploy ""
}

# Point d'entrée
do_all
