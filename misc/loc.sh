declare -i out
declare -i sum

sum=0

#engine
cd ./SequoiaDB
dirs=`find . -type d -print | grep -v svn | grep -v pcre | grep -v bson | grep -v del_files | grep -v snappy | grep -v gtest`
for dir in $dirs
do
   out=0
   o=`find $dir -maxdepth 1 \( -name "*.cpp" -o -name "*.hpp" -o -name "*.h" -o -name "*.c" \) -exec wc {} \; | awk '{print($1)}'`
   for i in $o
   do
      out=$out+$i
      sum=$sum+$i
   done
   echo $dir ": " $out
done
cd ..
#driver
cd ./driver
dirs=`find . -type d -print | grep -v temp | grep -v svn | grep -v mongodb | grep -v bson | grep -v google | grep -v "php-" | grep -v Bson`
for dir in $dirs
do
   out=0
   o=`find $dir -maxdepth 1 \( -name "*.cpp" -o -name "*.hpp" -o -name "*.h" -o -name "*.c" -o -name "*.java" \) -exec wc {} \; | awk '{print($1)}'`
   for i in $o
   do
      out=$out+$i
      sum=$sum+$i
   done
   if [ $out -ne 0 ]; then
      echo $dir ": " $out
   fi
done

echo "total: " $sum
