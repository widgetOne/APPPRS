#!/bin/bash
# This script turns the laptop into an access point
nmcli nm wifi off
rfkill unblock wlan
ip addr add 192.168.16.1/24 dev wlan0
sleep 5
hostapd -B hostapd.conf
nmcli nm wifi off
rfkill unblock wlan
ip addr add 192.168.16.1/24 dev wlan0
sleep 5
hostapd -B hostapd.conf
