USE master
CREATE LOGIN user1 WITH 
PASSWORD=N'pswd', DEFAULT_DATABASE=hsdb, 
CHECK_EXPIRATION=OFF, CHECK_POLICY=OFF
EXEC master..sp_addsrvrolemember @loginame = N'user1', @rolename = N'sysadmin'
GO
select name,dbname,sysadmin from syslogins where name not like '#%'
GO

USE hsdb
CREATE USER user1 FOR LOGIN user1
EXEC sp_addrolemember N'db_owner', N'user1'
GO
