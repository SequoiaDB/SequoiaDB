/****************************************************************************
 *
 * Name: dataSource.cpp
 * Description: This program demostrates how to connect to SequoiaDB database
 *              with sdbDataSource.
 *
 * Auto Compile:
 * Linux: NA
 * Win: NA
 *
 * Manual Compile:
 *    Dynamic Linking:
 *    Linux:
 *       g++ common.cpp worker.cpp dataSource.cpp -o dataSource -I../../include -O0 -ggdb -Wno-deprecated -L../../lib -lsdbcpp -lm -ldl -lpthread
 *    Win:
 *       cl /Focommon.obj /c common.cpp /I..\..\include /wd4047 /Od /MDd /RTC1 /Z7 /TP
 *       cl /Foworker.obj /c worker.cpp /I..\..\include /wd4047 /Od /MDd /RTC1 /Z7 /TP
 *       cl /FodataSource.obj /c dataSource.cpp /I..\..\include /wd4047 /Od /MDd /RTC1 /Z7 /TP
 *       link /OUT:dataSource.exe /LIBPATH:..\..\lib\cpp\debug\dll sdbcppd.lib dataSource.obj common.obj worker.obj  /debug
 *       copy ..\..\lib\cpp\debug\dll\sdbcppd.dll .
 *    Static Linking:
 *    Linux: g++ dataSource.cpp  worker.cpp common.cpp -o dataSource.static -I../../include -O0 -ggdb -Wno-deprecated ../../lib/libstaticsdbcpp.a -lm -ldl -lpthread
 * Run:
 *    Linux: LD_LIBRARY_PATH=<path for libsdbcpp.so> ./dataSource 192.168.20.165:11810 192.168.20.166:11810
 *    Win: dataSource.exe 192.168.20.165:11810 192.168.20.166:11810
 *
 ******************************************************************************/
#include <vector>
#include <new>
#include "common.hpp"
#include "worker.hpp"
#include "dataSourceTask.hpp"
#include "sdbDataSource.hpp"

using namespace std;
using namespace sdbclient;
using namespace bson;
using namespace sample;

/* 函数声明 */
void setDataSourceConf(sdbDataSourceConf &conf);
INT32 initDataSource(sdbDataSource &ds,
    sdbDataSourceConf &conf,
    vector<string> &addrs);
INT32 closeDataSource(sdbDataSource &ds);
INT32 runTasks(sdbDataSource &ds, string &csName, string &clName, int taskNum);
void runCreateCLTask(void *args);
void runInsertTask(void *args);
void runQueryTask(void *args);

INT32 main(INT32 argc, CHAR **argv)
{
    INT32 rc        = SDB_OK;
    INT32 taskNum   = 10;
    string csName   = "foo";
    string clName   = "bar";
    BOOLEAN hasInit = FALSE;
    vector<string> vecAddrs;
    sdbDataSourceConf conf;
    sdbDataSource ds;

    /* 检测输入参数内容 */
    if (argc < 2)
    {
        cout << "Syntax:" << (CHAR*)argv[0]
        << " <hostname/ip:port> [<hostname/ip:port> ...] " << endl;
        exit ( 0 );
    }

    /* 准备coord节点地址 */
    for(INT32 i = 1; i < argc; ++i)
    {
        vecAddrs.push_back((CHAR*)argv[i]);
    }

    /* 设置连接池的配置参数 */
    setDataSourceConf(conf);

    /* 初始化连接池 */
    rc = initDataSource(ds, conf, vecAddrs);
    if (SDB_OK != rc)
    {
        goto error;
    }
    hasInit = TRUE;

    /* 在业务中使用连接池 */
    runTasks(ds, csName, clName, taskNum);

done:
    if (TRUE == hasInit)
    {
        /* 关闭连接池 */
        closeDataSource(ds);
    }
    return rc;
error:
    goto done;
}

void setDataSourceConf(sdbDataSourceConf &conf)
{
    /* 设置数据库鉴权的用户名密码。
       若数据库没开启鉴权，此处可填空字符串，或直接忽略此项。
       以下2个参数分别取默认值。 */
    conf.setUserInfo("", "");
    /* 第一个参数10表示连接池启动时，初始化10个可用连接。
       第二个参数10表示当连接池增加可用连接时，每次增加10个。
       第三个参数20表示当连接池空闲时，池中最多保留20个连接，多余的将被释放。
       第四个参数500表示连接池最多保持500个连接。
       以下4个参数分别取默认值。 */
    conf.setConnCntInfo(10, 10, 20, 500);
    /* 第一个参数60000，单位为毫秒。表示连接池以60秒为一个周期，
       周期性地删除多余的空闲连接及检测连接是否过长时间没有被使用(收发消息)。
       第二个参数0，单位为毫秒。表示池中空闲连接的存活时间。
       0毫秒表示连接池不关心连接隔了多长时间没有收发消息。
       以下2个参数分别取默认值。 */
    conf.setCheckIntervalInfo(60000, 0);
    /* 设置周期性从catalog节点同步coord节点地址的周期。单位毫秒。
       当设置为0毫秒时，表示不同步coord节点地址。默认值为0毫秒。 */
    conf.setSyncCoordInterval(30000);
    /* 设置使用coord地址负载均衡的策略获取连接。默认值为DS_STY_BALANCE。 */
    conf.setConnectStrategy(DS_STY_BALANCE);
    /* 连接出池时，是否检测连接的可用性，默认值为FALSE。 */
    conf.setValidateConnection(FALSE);
    /* 设置连接是否开启SSL功能，默认值为FALSE。 */
    conf.setUseSSL(FALSE);
}

INT32 initDataSource(sdbDataSource &ds,
    sdbDataSourceConf &conf,
    vector<string> &addrs)
{
    INT32 rc = SDB_OK;

    /* 初始化连接池 */
    rc = ds.init(addrs, conf);
    if (SDB_OK != rc)
    {
        cout << "Failed to init sdbDataSource, rc = " << rc << endl;
        goto error;
    }
    /* 启用连接池 */
    rc = ds.enable();
    if (SDB_OK != rc)
    {
        cout << "Failed to enable sdbDataSource, rc = " << rc << endl;
        goto error;
    }

done:
    return rc;
error:
    goto done;
}

INT32 closeDataSource(sdbDataSource &ds)
{
    INT32 rc = SDB_OK;

    /* 禁用连接池 */
    rc = ds.disable();
    if (SDB_OK != rc)
    {
        cout << "Failed to disable sdbDataSource, rc = " << rc << endl;
    }
    /* 关闭连接池 */
    ds.close();
    return rc;
}

INT32 runTasks(sdbDataSource &ds, string &csName, string &clName, int taskNum)
{
    INT32 rc                        = SDB_OK;
    createCLTask *pCreateCLTask     = NULL;
    worker *pCreateCLWorker         = NULL;
    vector<insertTask*> vecInsertTasks;
    vector<queryTask*> vecQueryTasks;
    vector<worker*> vecInsertWorkers;
    vector<worker*> vecQueryWorkers;

    /* 初始化create collection任务 */
    pCreateCLTask = (createCLTask *)(new(std::nothrow) createCLTask(ds, csName, clName));
    pCreateCLWorker = (worker *)(new(std::nothrow) worker(runCreateCLTask, (void*)pCreateCLTask));
    if (NULL == pCreateCLTask || NULL == pCreateCLWorker)
    {
        rc = SDB_OOM;
        goto error;
    }

    /* 初始化insert任务 */
    for(INT32 i = 0; i < taskNum; ++i)
    {
        insertTask *pTask = NULL;
        worker *pWorker   = NULL;
        pTask = new(std::nothrow) insertTask(ds, csName, clName, i);
        if (NULL == pTask)
        {
            rc = SDB_OOM;
            goto error;
        }
        vecInsertTasks.push_back(pTask);
        pWorker = new(std::nothrow) worker(runInsertTask, (void*)(pTask));
        if (NULL == pWorker)
        {
            rc = SDB_OOM;
            goto error;
        }
        vecInsertWorkers.push_back(pWorker);
    }

    /* 初始化query任务 */
    for(INT32 i = 0; i < taskNum; ++i)
    {
        queryTask *pTask = NULL;
        worker *pWorker   = NULL;
        pTask = new(std::nothrow) queryTask(ds, csName, clName, i);
        if (NULL == pTask)
        {
            rc = SDB_OOM;
            goto error;
        }
        vecQueryTasks.push_back(pTask);
        pWorker = new(std::nothrow) worker(runQueryTask, (void*)(pTask));
        if (NULL == pWorker)
        {
            rc = SDB_OOM;
            goto error;
        }
        vecQueryWorkers.push_back(pWorker);
    }

    /* 启动创建集合foo.bar任务 */
    pCreateCLWorker->start();
    pCreateCLWorker->waitStop();

    /* 启动插数据任务和查询数据任务 */
    for(vector<worker*>::iterator itr = vecInsertWorkers.begin();
        itr != vecInsertWorkers.end();
        ++itr)
    {
        (*itr)->start();
    }
    for(vector<worker*>::iterator itr = vecQueryWorkers.begin();
        itr != vecQueryWorkers.end();
        ++itr)
    {
        (*itr)->start();
    }

    /* 等待任务结束 */
    for(vector<worker*>::iterator itr = vecInsertWorkers.begin();
        itr != vecInsertWorkers.end();
        ++itr)
    {
        (*itr)->waitStop();
    }
    for(vector<worker*>::iterator itr = vecQueryWorkers.begin();
        itr != vecQueryWorkers.end();
        ++itr)
    {
        (*itr)->waitStop();
    }

done:
    /* 释放资源 */
    if (NULL != pCreateCLTask)
    {
        delete pCreateCLTask;
        pCreateCLTask = NULL;
    }
    if (NULL != pCreateCLWorker)
    {
        delete pCreateCLWorker;
        pCreateCLWorker = NULL;
    }
    for(vector<insertTask*>::iterator itr = vecInsertTasks.begin();
        itr != vecInsertTasks.end();
        ++itr)
    {
        delete *itr;
    }
    for(vector<queryTask*>::iterator itr = vecQueryTasks.begin();
        itr != vecQueryTasks.end();
        ++itr)
    {
        delete *itr;
    }
    for(vector<worker*>::iterator itr = vecInsertWorkers.begin();
        itr != vecInsertWorkers.end();
        ++itr)
    {
        delete *itr;
    }
    for(vector<worker*>::iterator itr = vecQueryWorkers.begin();
        itr != vecQueryWorkers.end();
        ++itr)
    {
        delete *itr;
    }
    vecInsertTasks.clear();
    vecQueryTasks.clear();
    vecInsertWorkers.clear();
    vecQueryWorkers.clear();
    return rc;
error:
    goto done;
}

void runCreateCLTask(void *args)
{
    createCLTask *pTask = (createCLTask *)args;
    pTask->run();
}

void runInsertTask(void *args)
{
    insertTask *pTask = (insertTask *)args;
    pTask->run();
}

void runQueryTask(void *args)
{
    queryTask *pTask = (queryTask *)args;
    pTask->run();
}
