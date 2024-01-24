
EXEC sp_configure 'recovery interval (min)',60
RECONFIGURE
EXEC sp_configure
GO

CREATE DATABASE hsdb ON  PRIMARY
( NAME = N'hsdb01', FILENAME = N'C:\hsdb\data\hsdb01.mdf' , 
  SIZE = 20480000KB , FILEGROWTH = 51200KB ),
( NAME = N'hsdb02', FILENAME = N'C:\hsdb\data\hsdb02.ndf' , 
  SIZE = 20480000KB , FILEGROWTH = 51200KB ),
( NAME = N'hsdb03', FILENAME = N'C:\hsdb\data\hsdb03.ndf' , 
  SIZE = 20480000KB , FILEGROWTH = 51200KB ),
( NAME = N'hsdb04', FILENAME = N'C:\hsdb\data\hsdb04.ndf' , 
  SIZE = 20480000KB , FILEGROWTH = 51200KB ),
( NAME = N'hsdb05', FILENAME = N'C:\hsdb\data\hsdb05.ndf' , 
  SIZE = 20480000KB , FILEGROWTH = 51200KB ),
( NAME = N'hsdb06', FILENAME = N'C:\hsdb\data\hsdb06.ndf' , 
  SIZE = 20480000KB , FILEGROWTH = 51200KB ),
( NAME = N'hsdb07', FILENAME = N'C:\hsdb\data\hsdb07.ndf' , 
  SIZE = 20480000KB , FILEGROWTH = 51200KB ),
( NAME = N'hsdb08', FILENAME = N'C:\hsdb\data\hsdb08.ndf' , 
  SIZE = 20480000KB , FILEGROWTH = 51200KB )
LOG ON 
( NAME = N'hsdblog', FILENAME = N'C:\hsdb\log\hsdb.ldf' , 
  SIZE = 20480000KB , FILEGROWTH = 10%)
GO

EXEC sp_helpdb hsdb
GO

ALTER DATABASE hsdb SET AUTO_CREATE_STATISTICS OFF WITH NO_WAIT
ALTER DATABASE hsdb SET AUTO_UPDATE_STATISTICS OFF WITH NO_WAIT
ALTER DATABASE hsdb SET RECOVERY SIMPLE WITH NO_WAIT
GO

EXEC sp_dboption hsdb
GO
