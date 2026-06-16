#!/bin/sh

while true; do
    DATETIME=$(date +"%H:%M")

    RAW_TEMP=$(sysctl -n hw.acpi.thermal.tz0.temperature 2>/dev/null)
    CPU_TEMP="${RAW_TEMP%%.*}"
    CPU_FREQ=$(sysctl -n dev.cpu.0.freq 2>/dev/null)

    BAT_LIFE=$(sysctl -n hw.acpi.battery.life 2>/dev/null)

    P_SIZE=$(sysctl -n vm.stats.vm.v_page_size)
    P_ACTIVE=$(sysctl -n vm.stats.vm.v_active_count)
    P_WIRE=$(sysctl -n vm.stats.vm.v_wire_count)

    TOTAL_P=$(( P_ACTIVE + P_WIRE ))
    BYTES=$(( TOTAL_P * P_SIZE ))

    GB_INT=$(( BYTES / 1024 / 1024 / 1024 ))
    GB_FRAC=$(( (BYTES * 100 / 1024 / 1024 / 1024) % 100 ))
    RAM_GB="${GB_INT}.${GB_FRAC}"

    BRIGHT=$(backlight 2>/dev/null | awk -F: '/brightness/ {gsub(/ /,"",$2); print $2}')
    [ -z "$BRIGHT" ] && BRIGHT="N/A"

    VOL=$(mixer 2>/dev/null | sed -n 's/^[[:space:]]*vol[[:space:]]*=[[:space:]]*\([0-9.]*\):.*/\1/p' | head -n1)
    [ -z "$VOL" ] && VOL="N/A"
    
    STATUS_STR="[BRI: ${BRIGHT}] [VOL: ${VOL}] [RAM: ${RAM_GB}GB] [CPU: ${CPU_TEMP}°C @ ${CPU_FREQ}MHz] [BAT: ${BAT_LIFE}%] ${DATETIME}"
    xsetroot -name "$STATUS_STR"
    sleep 1
done &
