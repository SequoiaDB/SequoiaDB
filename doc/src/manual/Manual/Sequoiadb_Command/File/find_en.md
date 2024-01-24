##NAME##

find - find files

##SYNOPSIS##

**File.find(\<options\>, \[filter\])**

##CATEGORY##

File

##DESCRIPTION##

This function is used to find files in the specified directory.

##PARAMETERS##

| Name | Type| Description | Required or not |
| ---- | --- | ----------- | --------------- |
| options   | object   | Find Patterns and Find What.|required      |
| filter    | object   | Filter conditions, support filtering the result set by "and", "or", "not" or exact match calculation. |not      |

The options parameter is detailed as follows:

| Name | Type| Description | Required or not |
| ---- | --- | ----------- | --------------- |
| mode     | string   | The type of search, the values are as follows:<br>'n': Find files by filename.<br>'u': Find files by username.<br>'g': Find files by user group name.<br>'p': Find files by permissions. | required |
| pathname | string | The search path, the default is to search for files in the current path. | not |
| value    | string | What to find.                       | required |

##RETURN VALUE##

When the function executes successfully, it will return an object of type BSONArray. Users can get the path of the file through this object.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v3.4 and above

##EXAMPLES##

- Find the file `file.txt` in the directory `/opt` by file name.

    ```lang-javascript
    > File.find({mode: 'n', value: "file.txt", pathname: "/opt"})
    {
        "pathname": "/opt/sequoiadb1/file.txt"
    }
    {
        "pathname": "/opt/sequoiadb2/file.txt"
    }
    {
        "pathname": "/opt/sequoiadb3/file.txt"
    }
    ```

- Find the file `file.txt` in the directory `/opt` by file name, and specify filter conditions.

    ```lang-javascript
    > File.find({mode: 'n', value: "file.txt", pathname: "/opt"}, {$or: [{pathname: "/opt/sequoiadb1/file.txt"}, {pathname: "/opt/sequoiadb2/file.txt"}]})
     {
         "pathname": "/opt/sequoiadb1/file.txt"
     }
     {
         "pathname": "/opt/sequoiadb2/file.txt"
     }
    ```
[^_^]:
    Links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md