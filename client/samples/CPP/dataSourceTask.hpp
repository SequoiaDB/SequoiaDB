/******************************************************************************
 *
 * Name: dataSourceTask.hpp
 * Description: Task class defined for data source sample.
 *
 ******************************************************************************/
#ifndef DATASOURCETASK_HPP__
#define DATASOURCETASK_HPP__

#include "common.hpp"
#include "sdbDataSource.hpp"

namespace sample
{

    /* 类taskBase定义 */
    class taskBase
    {
    protected:
        sdbDataSource &_ds;
        string _csName;
        string _clName;

    public:
        taskBase(sdbDataSource &ds, string &csName, string &clName)
            :_ds(ds), _csName(csName), _clName(clName)
        {
        }
        virtual ~taskBase(){}

    public:
        virtual INT32 run() = 0;
    };

    /* 类createCLTask定义 */
    class createCLTask: public taskBase
    {
    public:
        createCLTask(sdbDataSource &ds, string &csName, string &clName)
            :taskBase(ds, csName, clName)
        {
        }
        ~createCLTask(){}

    public:
        virtual INT32 run()
        {
            INT32 rc       = SDB_OK;
            BOOLEAN hasGet = FALSE;
            sdb *pDB       = NULL;
            sdbCollectionSpace cs;
            sdbCollection cl;

            /* 通过连接池获取连接 */
            rc = _ds.getConnection(pDB);
            if (SDB_OK != rc)
            {
                cout << "Failed to get connection to for creating collection, rc = "
                    << rc << endl ;
                goto error;
            }
            hasGet = TRUE;

            /* 使用连接进行业务（创建集合业务）操作 */
            pDB->dropCollectionSpace(_csName.c_str());

            rc = pDB->createCollectionSpace(_csName.c_str(), SDB_PAGESIZE_4K, cs);
            if (SDB_OK != rc)
            {
                cout << "Failed to create collection space, rc = " << rc << endl;
                goto error;
            }
            rc = cs.createCollection(_clName.c_str(), BSON("ReplSize" << 0), cl);
            if (SDB_OK != rc)
            {
                cout << "Failed to create collection, rc = " << rc << endl;
                goto error;
            }
            cout << "Success to create collection: " << _csName + "." + _clName << endl;

done:
            /* 业务结束后，将连接归还给连接池 */
            if (TRUE == hasGet)
            {
                _ds.releaseConnection(pDB);
            }
            return rc;
error:
            goto done;
        }
    };


    /* 类insertTask定义 */
    class insertTask: public taskBase
    {
    private:
        INT32 _num;
        INT32 _delay;

    public:
        /* 构造函数，参数num为插入内容，参数delay为最大延迟插入的时间，单位毫秒 */
        insertTask(sdbDataSource &ds, string &csName, string &clName, INT32 num, INT32 delay = 6000)
            :taskBase(ds, csName, clName), _num(num), _delay(delay)
        {
        }
        ~insertTask(){}

    public:
        virtual INT32 run()
        {
            INT32 rc          = SDB_OK;
            BOOLEAN hasGet    = FALSE;
            string clFullName = _csName + "." + _clName;
            sdb *pDB          = NULL;
            BSONObj obj       = BSON("num" << _num);
            sdbCollection cl;

            /* 随机睡眠若干时间，来模拟不同时刻插入数据的情况 */
            waiting(randNum() % _delay);

            /* 通过连接池获取连接 */
            rc = _ds.getConnection(pDB);
            if (SDB_OK != rc)
            {
                cout << "Failed to get connection to for inserting recored, rc = "
                    << rc << endl ;
                goto error;
            }
            hasGet = TRUE;

            /* 使用连接进行业务（插入数据业务） */
            rc = pDB->getCollection(clFullName.c_str(), cl);
            if (SDB_OK != rc)
            {
                cout << "Failed to get collection for inserting num: "
                    << _num << ", rc = " << rc << endl;
                goto error;
            }

            rc = cl.insert(obj);
            if (SDB_OK != rc)
            {
                cout << "Failed to insert num: "
                    << _num << ", rc = " << rc << endl;
                goto error;
            }
            cout << "Success to insert record: " << obj.toString(FALSE, TRUE) << endl;

done:
            /* 业务结束后，将连接归还给连接池 */
            if (TRUE == hasGet)
            {
                _ds.releaseConnection(pDB);
            }
            return rc;
error:
            goto done;
        }
    };

    /* 类queryTask定义 */
    class queryTask: public taskBase
    {
    private:
        INT32 _num;
        INT32 _timeout;

    public:
        /* 构造函数，参数num为要查询的内容，参数timeout为查询超时时间，单位毫秒 */
        queryTask(sdbDataSource &ds, string &csName, string &clName, INT32 num, INT32 timeout = 10000)
            :taskBase(ds, csName, clName), _num(num), _timeout(timeout)
        {
        }
        ~queryTask(){}

    public:
        virtual INT32 run()
        {
            INT32 rc          = SDB_OK;
            UINT32 totalSleep = 0;
            BOOLEAN hasGet    = FALSE;
            string clFullName = _csName + "." + _clName;
            sdb *pDB          = NULL;
            BSONObj matcher   = BSON("num" << BSON("$et" << _num));
            sdbCollection cl;

            /* 通过连接池获取连接 */
            rc = _ds.getConnection(pDB);
            if (SDB_OK != rc)
            {
                cout << "Failed to get connection for query, rc = "
                    << rc << endl ;
                goto error;
            }
            hasGet = TRUE;

            /* 使用连接进行业务（查询业务）*/
            rc = pDB->getCollection(clFullName.c_str(), cl);
            if (SDB_OK != rc)
            {
                cout << "Failed to get collection for querying num: "
                    << _num << ", rc = " << rc << endl;
                goto error;

            }
            while(totalSleep < _timeout)
            {
                sdbCursor cursor;
                rc = cl.query(cursor, matcher);
                if (SDB_OK == rc)
                {
                    BSONObj record;
                    rc = cursor.next(record);
                    if (SDB_OK == rc)
                    {
                        cout << "We get record: " << record.toString(FALSE, TRUE) << " from database" << endl;
                        cursor.close();
                        break;
                    }
                    else if (SDB_DMS_EOC == rc)
                    {
                        waiting(500);
                        totalSleep += 500;
                        continue;
                    }
                    else
                    {
                        goto error;
                    }
                }
                else
                {
                    goto error;
                }
            }

            /* 若查询超时，报错 */
            if(totalSleep >= _timeout)
            {
                cout << "Failed to query num: " << _num << ", timeout" << endl;
            }

done:
            /* 业务结束后，将连接归还给连接池 */
            if (TRUE == hasGet)
            {
                _ds.releaseConnection(pDB);
            }
            return rc;
error:
            cout << "Failed to query num: "
                << _num << ", rc = " << rc << endl;
            goto done;
        }
    };

} // namespace sample

#endif // DATASOURCETASK_HPP__

