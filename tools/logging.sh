#!/bin/bash
cd /bin
./JLinkRTTLogger -Device NRF52840_XXAA -if SWD -Speed 4000 -RTTChannel 0 /dev/stdout