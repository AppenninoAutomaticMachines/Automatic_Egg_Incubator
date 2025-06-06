#!/bin/bash

DELAY=60
BAR_LENGTH=30

GREEN="\e[92m"
YELLOW="\e[93m"
RESET="\e[0m"

echo -e "${YELLOW}Avvio del programma tra $DELAY secondi...${RESET}"

for ((i=0; i<=DELAY; i++)); do
    percent=$((100 * i / DELAY))
    filled=$((BAR_LENGTH * i / DELAY))
    empty=$((BAR_LENGTH - filled))

    bar=$(printf "%0.s#" $(seq 1 $filled))
    spaces=$(printf "%0.s-" $(seq 1 $empty))

    remaining=$((DELAY - i))
    min=$((remaining / 60))
    sec=$((remaining % 60))
    time_left=$(printf "%02d:%02d" $min $sec)

    printf "\r${GREEN}[%s%s] %3d%% - Mancano %s ${RESET}" "$bar" "$spaces" "$percent" "$time_left"
    sleep 1
done

echo -e "\n${YELLOW}Avvio del programma ora!${RESET}"

# Avvio script Python
/home/filippo/Documents/PyQT_GUI/venv-py312/bin/python3 /home/filippo/Documents/PyQT_GUI/eggsIncubatorMVVM.py 2>&1 | tee /home/filippo/Documents/PyQT_GUI/eggs_log.txt

echo -e "${YELLOW}Script terminato. Premi un tasto per chiudere.${RESET}"
read -n 1
