prtscnDir=prtscn
showNum=6
showInterval=2s

while true; do
   clear
   find ${prtscnDir} -name "*.log" | xargs tail -n ${showNum}
   sleep ${showInterval}
done
