/********************************************************************
*@Description : test Oma Initialization
*               TestLink: 10600  使用不存在的主机初始化Oma对象 
*                         10601  使用非cm端口初始化Oma对象
*@author      : Liang XueWang
********************************************************************/
main( test );

function test ()
{
   var illegalOma;

   // 测试Oma使用不存在的主机初始化
   var obj = { "hostname": "IllegalHost" };
   illegalOma = new OmaTest( obj, CMSVCNAME, false, true );
   illegalOma.testInit();

   // 测试Oma使用非cm端口初始化
   obj = { "hostname": COORDHOSTNAME };
   illegalOma = new OmaTest( obj, COORDSVCNAME, true, false );
   illegalOma.testInit();
}


