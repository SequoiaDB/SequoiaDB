profileDir=./prop
prtscnDir=./prtscn
ls ${profileDir} | while read profileName; do
   java -Xmx1024m -Xms512m -jar ecmtest.jar ${profileDir}/${profileName} > ${prtscnDir}/${profileName}.log 2>&1 &
done
