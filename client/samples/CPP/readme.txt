Compile and run example:
 * Auto Compile:
 * Linux: ./buildApp.sh query
 * Win: buildApp.bat query
 * Manual Compile:
 * Linux: 
 *       g++ query.cpp common.cpp -o query -I../../include \
 *       -L../../lib -lsdbcpp
 * Win:
 *    cl /Foquery.obj /c query.cpp /I..\..\include /wd4047
 *    cl /Focommon.obj /c common.cpp /I..\..\include /wd4047
 *    link /OUT:query.exe /LIBPATH:..\..\lib sdbcpp.lib query.obj common.obj
 *    copy ..\..\lib\sdbcpp.dll .
 * Run:
 * Linux: LD_LIBRARY_PATH=<path for libsdbcpp.so> ./query <hostname> \
 *        <servicename> <username> <password>
 * eg: ./query localhost 50000 "" ""
 * Win: query.exe <hostname> <servicename> <username> <password>
 * eg: query.exe localhost 50000 "" ""
