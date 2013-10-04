#!/bin/bash
BASE=`pwd`
PYTHON=python3
MODULES=modules
DESTINATION=sources

# Tous les dossiers traités en tant que dossier de code sont de niveau 2 dans
# l'arborescence

map_folder() {
  local DIR=$1
  local FUNCTION=$2
  local FOLDERS=`ls $MODULES/$DIR`
  local f
  echo $FOLDERS
  for f in $FOLDERS; do
    $FUNCTION $DIR/$f
  done
}

do_deploy() {
  echo Deploiment de : $MODULES$1
  if [ -f $MODULES$1 ]; then
    echo Fichier à copier
    echo cp -fpr $MODULES$1 $DESTINATION$1
    cp -fpr $MODULES$1 $DESTINATION$1
  fi
  if [ -d $MODULES$1 ]; then
    mkdir -p $DESTINATION$1
    map_folder $1 do_deploy
  fi
}

do_all() {
  echo Dossier de travail : $MODULES
  map_folder "" do_deploy
}

# Entry point
do_all
