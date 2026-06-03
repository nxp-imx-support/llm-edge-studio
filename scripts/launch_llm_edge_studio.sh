#!/bin/bash
#
# Copyright 2026 NXP
#
# NXP Proprietary. This software is owned or controlled by NXP and may only be
# used strictly in accordance with the applicable license terms.  By expressly
# accepting such terms or by downloading, installing, activating and/or
# otherwise using the software, you are agreeing that you have read, and that
# you agree to comply with and are bound by, such license terms.  If you do
# not agree to be bound by the applicable license terms, then you may not
# retain, install, activate or otherwise use the software.

export DISPLAY=:0

xterm \
  -bg "#0000CD" \
  -fg "#FFD700" \
  -cr "#FFFF00" \
  -geometry 100x20 \
  -title "LLM Edge Studio" \
  -e bash -c "
    echo '=======================================================';
    echo '      LLM Edge Studio is loading, please wait...';
    echo '=======================================================';
    echo 'Remember to set date/time to download models.';
    echo ''; date; echo '';
    run_llm_edge_studio &
    APP_PID=\$!;
    while ! xdotool search --name 'LLM Edge Studio' 2>/dev/null | grep -qv \$\$; do
      sleep 0.5;
    done;
    exit 0
  " &
wait