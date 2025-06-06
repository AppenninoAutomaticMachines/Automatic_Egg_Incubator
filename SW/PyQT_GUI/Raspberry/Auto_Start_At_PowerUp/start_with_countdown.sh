#!/bin/bash

# Attende finché il display X è disponibile
while ! xset q > /dev/null 2>&1; do
    echo "Attendo che il display grafico sia pronto..."
    sleep 1
done


DELAY=60
BAR_LENGTH=30

GREEN="\e[92m"
YELLOW="\e[93m"
RESET="\e[0m"

echo -e "${YELLOW}Starting application in $DELAY seconds...${RESET}"

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

    printf "\r${GREEN}[%s%s] %3d%% - Time left %s ${RESET}" "$bar" "$spaces" "$percent" "$time_left"
    sleep 1
done

echo -e "\n${YELLOW}Starting application now!${RESET}"
sleep 1
# Avvia lo script Python con output non bufferizzato
/home/filippo/Documents/PyQT_GUI/venv-py312/bin/python3 -u /home/filippo/Documents/PyQT_GUI/eggsIncubatorMVVM.py

echo -e "\nScript terminato. Premi un tasto per chiudere..."
read -n 1
