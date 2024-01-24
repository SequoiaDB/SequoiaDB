Compile and run example:
 * Auto Compile:
 * Linux: ./buildApp.sh query
 * Win: buildApp.bat query
 * Manual Compile:
 * Linux: cc query.c common.c -o query -I../../include -L../../lib -lsdbc
 * Win:
 *    cl /Foquery.obj /c query.c /I..\..\include /wd4047
 *    cl /Focommon.obj /c common.c /I..\..\include /wd4047
 *    link /OUT:query.exe /LIBPATH:..\..\lib\c\debug\dll sdbcd.lib query.obj common.obj
 *    copy ..\..\lib\c\debug\dll\sdbcd.dll .
 * Run:
 * Linux: LD_LIBRARY_PATH=<path for libsdbc.so> ./query <hostname> <servicename> \
 *        <Username> <Username>
 * eg: ./query localhost 50000 "" ""
 * Win: query.exe <hostname> <servicename> <Username> <Username>
 * eg: query.exe localhost 50000 "" ""
