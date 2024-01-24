java -Dlog4j.configuration=file:log4j.properties -jar wordConvertor.jar -i E:\indexes.html -o E:\indexes.doc -t

java -jar wordConvertor.jar -h
usage: WordConvertor
 -h,--help           help
 -i,--input <arg>    input html file
 -o,--output <arg>   output word file
 -p,--period <arg>   period count of pictures(default 40). adjust pictures may cause out of memory, 
                     decrease this count can decrease memory use
 -t,--table          add table of content 
 -v,--visible        make word visible for debugging
 


before running, we should add the PATH:
add libs/jacob-1.18-x64.dll & libs/jacob-1.18-x86.dll 's path to the system path