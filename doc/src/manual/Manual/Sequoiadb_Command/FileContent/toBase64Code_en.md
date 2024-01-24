
##NAME##

toBase64Code - Convert binary stream to base64 encoded.

##SYNOPSIS##

**toBase64Code()**

##CATEGORY##

FileContent

##DESCRIPTION##

Convert binary stream to base64 encoded.

##PARAMETERS##

NULL

##RETURN VALUE##

On success, return the string in base64 encoded format of binary stream.

On error, exception will be thrown.

##ERRORS##

when exception happen, use [getLastError()](manual/Manual/Sequoiadb_command/Global/getLastError.md) to get the [error code](manual/Manual/Sequoiadb_error_code.md)  and use [getLastErrMsg()](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please  reference to [Troubleshooting](manual/FAQ/faq_sdb.md).

##EXAMPLES##

* Open a banary file and get a file descriptor.

    ```lang-javascript
    > var file = new File( "/opt/sequoiadb/file.dump" )
    ```

* Read the contents of the file into the fileContent object( For more detail, refer to [File::readContent](manual/Mannual/Sequoiadb_Command/File/readContent) ).

    ```lang-javascript
    > var content = file.readContent( 10 )
    ```

* Convert the binary stream in the fileContent to base64 encoded.

    ```lang-javascript
    > var base64String = content.toBase64Code()
    ```

* You can write the string after conversion to a new file, so you can easily view the string.

    ```lang-javascript
    > var base64StringFile = new File( "/opt/sequoiadb/file.dump.base64" ) 
    > base64StringFile.write( base64String )
    ```

* Read the contents of the file.

    ```lang-javascript
    > base64StringFile.seek(0)
    > base64StringFile.read()
    BQAGAAgA8////w==
    ```