#!/bin/bash

sakura -e "zsh -c 'cd `pwd`/../debug_client ; ./start-target.sh ; zsh'" &
sakura -e "zsh -c 'cd `pwd`/../debug_client ; tail -f info_target_0 ; zsh'" &
sakura -e "zsh -c 'cd `pwd`/../../eric/eric_tools ; ./start.sh ; zsh'" &
sakura -e "zsh -c 'cd `pwd`/../../eric/tftpy ; ./start.sh ; zsh'" &
