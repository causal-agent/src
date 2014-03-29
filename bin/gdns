#!/bin/bash

if grep -q 'gdns.sh' /etc/resolv.conf; then
  sed -i '1,3d' /etc/resolv.conf
  echo 'Google DNS removed'
else
  sed -i '1i # Added by gdns.sh' /etc/resolv.conf
  sed -i '2i nameserver 8.8.8.8' /etc/resolv.conf
  sed -i '3i nameserver 8.8.4.4' /etc/resolv.conf
  echo 'Google DNS added'
fi
