##NAME##

snapshot - get snapshot

##SYNOPSIS##

**db.snapshot(\<snapType\>,[cond],[sel],[sort])**

**db.snapshot(\<snapType\>,[SdbSnapshotOption])**

##CATEGORY##

Sdb

##DESCRIPTION##

This function is used to get the specified snapshot and view the current system status. 

##PARAMETERS##

| Name    | Type   | Description    | Required or Not |
|---------|--------|----------------|-----------------|
| snapType | macro | The snapshot to be obtained, the value can refer to [Snapshot type][snapshot].| Required 	   |
| cond     | object     | Match condictions and [position parameter][parameter]. | Not 	   |
| sel      | object     | Select the returned field name. When it is null, return all field names.         | Not 	   |
| sort     |  object    | Sort the returned records by the selected field. The values are as follow:<br> 1: ascending <br>-1: descending        | Not 	   |
| SdbSnapshotOption | object  | Use an object to specify the snapshot query parameters, the usage method can refer to [SdbSnapshotOption][shotOption]. | Not |

>**Note:**
>
>* sel parameter is a json structure,like:{Field name:Field value}，The field value is generally specified as an empty string.The field name specified in sel exists in the record,setting the field value does not take effect;return the field name and field value specified in the sel otherwise.
>* The field value type in the record is an array.User can specify the field name in sel,and use "." operator with double marks ("") to refer to the array elements.

##RETURN VALUE##

When the function executes successfully, it will return an object of type SdbCursor.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens，use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v2.0 and above

##EXAMPLES##

* Specify the value of snapType as SDB_LIST_CONTEXTS.

    ```lang-javascript
    > db.snapshot( SDB_SNAP_CONTEXTS )
    {
      "SessionID": "vmsvr1-cent-x64-1:11820:22",
      "Contexts": [
        {
          "ContextID": 8,
          "Type": "DUMP",
          "Description": "BufferSize:0",
          "DataRead": 0,
          "IndexRead": 0,
          "QueryTimeSpent": 0,
          "StartTimestamp": "2013-12-28-16.07.59.146399"
        }
      ]
    }
    {
      "SessionID": "vmsvr1-cent-x64-1:11830:22",
      "Contexts": [
        {
          "ContextID": 6,
          "Type": "DUMP",
          "Description": "BufferSize:0",
          "DataRead": 0,
          "IndexRead": 0,
          "QueryTimeSpent": 0,
          "StartTimestamp": "2013-12-28-16.07.59.147576"
        }
      ]
    }
    {
      "SessionID": "vmsvr1-cent-x64-1:11840:23",
      "Contexts": [
        {
          "ContextID": 7,
          "Type": "DUMP",
          "Description": "BufferSize:0",
          "DataRead": 0,
          "IndexRead": 0,
          "QueryTimeSpent": 0,
          "StartTimestamp": "2013-12-28-16.07.59.148603"
        }
      ]
    }
    ```

* Query the snapshot information of a replication group by group name or group ID.

    ```lang-javascript
    > db.snapshot( SDB_SNAP_CONTEXTS, { GroupName:'data1' } )
    > db.snapshot(SDB_SNAP_CONTEXTS,{GroupID:1000})
    {
      "SessionID": "vmsvr1-cent-x64-1:11820:22",
      "Contexts": [
        {
          "ContextID": 11,
          "Type": "DUMP",
          "Description": "BufferSize:0",
          "DataRead": 0,
          "IndexRead": 0,
          "QueryTimeSpent": 0,
          "StartTimestamp": "2013-12-28-16.13.57.864245"
        }
      ]
    }
    {
      "SessionID": "vmsvr1-cent-x64-1:11840:23",
      "Contexts": [
        {
          "ContextID": 10,
          "Type": "DUMP",
          "Description": "BufferSize:0",
          "DataRead": 0,
          "IndexRead": 0,
          "QueryTimeSpent": 0,
          "StartTimestamp": "2013-12-28-16.13.57.865103"
        }
      ]
    }
    ```

* Query the snapshot information of a node by specifying the group name, host name and service name, or specifying the group ID and node ID.

    ```lang-javascript
    > db.snapshot( SDB_SNAP_CONTEXTS, { GroupName: 'data1', HostName: "vmsvr1-cent-x64-1", svcname: "11820" } )
    > db.snapshot(SDB_SNAP_CONTEXTS,{GroupID:1000,NodeID:1001})
    {
      "SessionID": "vmsvr1-cent-x64-1:11820:22",
      "Contexts": [
        {
          "ContextID": 11,
          "Type": "DUMP",
          "Description": "BufferSize:0",
          "DataRead": 0,
          "IndexRead": 0,
          "QueryTimeSpent": 0,
          "StartTimestamp": "2013-12-28-16.13.57.864245"
        }
      ]
    }
    ```

* Query the snapshot information of a node by specifying the host name and service name.

    ```lang-javascript
    > db.snapshot( SDB_SNAP_CONTEXTS, { HostName: "ubuntu-200-043", svcname: "11820" } )
    {
      "NodeName": "ubuntu-200-043:11820",
      "SessionID": 18,
      "Contexts": [
        {
          "ContextID": 31,
          "Type": "DUMP",
          "Description": "IsOpened:1,HitEnd:0,BufferSize:0",
          "DataRead": 0,
          "IndexRead": 0,
          "QueryTimeSpent": 0,
          "StartTimestamp": "2016-10-27-17.53.45.042061"
        }
      ]
    }
    ```

* Return the original data before coord aggregation.

    ```lang-javascript
    > db.snapshot( SDB_SNAP_DATABASE, { RawData: true } ,{ NodeName: null, GroupName: null, TotalDataRead: null } )
    {
      "NodeName": "ubuntu1604-yt:30000",
      "GroupName": "SYSCatalogGroup",
      "TotalDataRead": 276511
    }
    {
      "NodeName": "ubuntu1604-yt:20000",
      "GroupName": "db1",
      "TotalDataRead": 16542209
    }
    {
      "NodeName": "ubuntu1604-yt:40000",
      "GroupName": "db2",
      "TotalDataRead": 959
    }
    Return 3 row(s).
    ```

[^_^]:
     links
[snapshot]:manual/Manual/Snapshot/snapshot.md
[parameter]:manual/Manual/Sequoiadb_Command/location.md
[shotOption]:manual/Manual/Sequoiadb_Command/AuxiliaryObjects/SdbSnapshotOption.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md