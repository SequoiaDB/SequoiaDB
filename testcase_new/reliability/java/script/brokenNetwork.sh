#!/bin/bash
trap "" 1 2 3 24
eth=`route | grep default | awk '{print $8}'`
ifconfig ${eth} down
sleep $1 &
wait
ifconfig ${eth} up
service network restart
/etc/init.d/networking restart