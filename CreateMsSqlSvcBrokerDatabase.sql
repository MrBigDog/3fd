IF NOT EXISTS (
	SELECT name 
		FROM master.dbo.sysdatabases 
		WHERE ('[' + name + ']') = '[SvcBrokerTest]'
			OR name = 'SvcBrokerTest'
)
BEGIN
	
	CREATE DATABASE [SvcBrokerTest]
		CONTAINMENT = NONE
		ON PRIMARY 
		(
			NAME = N'SvcBrokerTest'
			, FILENAME = N'C:\Users\felip\Documents\Visual Studio 2015\Projects\3FD\main\SQLDATA\SvcBrokerTest.mdf'
			, SIZE = 5120KB
			, FILEGROWTH = 1024KB
		)
		LOG ON 
		(
			NAME = N'SvcBrokerTest_log'
			, FILENAME = N'C:\Users\felip\Documents\Visual Studio 2015\Projects\3FD\main\SQLDATA\SvcBrokerTest_log.ldf'
			, SIZE = 1024KB
			, FILEGROWTH = 10%
		);
	
	ALTER DATABASE [SvcBrokerTest] SET COMPATIBILITY_LEVEL = 120;
	ALTER DATABASE [SvcBrokerTest] SET ANSI_NULL_DEFAULT OFF;
	ALTER DATABASE [SvcBrokerTest] SET ANSI_NULLS OFF;
	ALTER DATABASE [SvcBrokerTest] SET ANSI_PADDING OFF;
	ALTER DATABASE [SvcBrokerTest] SET ANSI_WARNINGS OFF;
	ALTER DATABASE [SvcBrokerTest] SET ARITHABORT OFF;
	ALTER DATABASE [SvcBrokerTest] SET AUTO_CLOSE OFF;
	ALTER DATABASE [SvcBrokerTest] SET AUTO_SHRINK OFF;
	ALTER DATABASE [SvcBrokerTest] SET AUTO_CREATE_STATISTICS ON(INCREMENTAL = OFF);
	ALTER DATABASE [SvcBrokerTest] SET AUTO_UPDATE_STATISTICS ON;
	ALTER DATABASE [SvcBrokerTest] SET CURSOR_CLOSE_ON_COMMIT OFF;
	ALTER DATABASE [SvcBrokerTest] SET CURSOR_DEFAULT  GLOBAL;
	ALTER DATABASE [SvcBrokerTest] SET CONCAT_NULL_YIELDS_NULL OFF;
	ALTER DATABASE [SvcBrokerTest] SET NUMERIC_ROUNDABORT OFF;
	ALTER DATABASE [SvcBrokerTest] SET QUOTED_IDENTIFIER OFF;
	ALTER DATABASE [SvcBrokerTest] SET RECURSIVE_TRIGGERS OFF;
	ALTER DATABASE [SvcBrokerTest] SET ENABLE_BROKER;
	ALTER DATABASE [SvcBrokerTest] SET AUTO_UPDATE_STATISTICS_ASYNC OFF;
	ALTER DATABASE [SvcBrokerTest] SET DATE_CORRELATION_OPTIMIZATION OFF;
	ALTER DATABASE [SvcBrokerTest] SET PARAMETERIZATION SIMPLE;
	ALTER DATABASE [SvcBrokerTest] SET READ_COMMITTED_SNAPSHOT OFF;
	ALTER DATABASE [SvcBrokerTest] SET READ_WRITE;
	ALTER DATABASE [SvcBrokerTest] SET RECOVERY SIMPLE;
	ALTER DATABASE [SvcBrokerTest] SET MULTI_USER;
	ALTER DATABASE [SvcBrokerTest] SET PAGE_VERIFY CHECKSUM;
	ALTER DATABASE [SvcBrokerTest] SET TARGET_RECOVERY_TIME = 0 SECONDS;
	ALTER DATABASE [SvcBrokerTest] SET DELAYED_DURABILITY = DISABLED;
END
GO

USE [SvcBrokerTest]
GO

IF NOT EXISTS (
	SELECT name
		FROM sys.filegroups
		WHERE is_default=1
			AND name = N'PRIMARY'
)
BEGIN
	ALTER DATABASE [SvcBrokerTest]
		MODIFY FILEGROUP [PRIMARY] DEFAULT;
END
GO
