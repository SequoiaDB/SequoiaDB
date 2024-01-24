#!/bin/bah


out=`jps|grep Master|grep -v grep|awk '{print $1}'`

if [ "$outA" != "A" ];then
   jps|grep Master|grep -v grep|awk '{print $1}' | xargs kill 
fi

out=`jps|grep Slave|grep -v grep|awk '{print $1}'`
if [ "$outA" != "A" ];then
   jps|grep Slave|grep -v grep |awk '{print $1}' | xargs kill 
fi
