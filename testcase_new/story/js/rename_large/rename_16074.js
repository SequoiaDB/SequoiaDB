/* *****************************************************************************
@discretion: rename cl
             seqDB-16074
@author��2018-10-15 chensiqin  Init
***************************************************************************** */
main( test );
function test ()
{
   /*
     1��cs�´������cl���������cl������4096��cl�� 
     2���޸�cl�����ֱ𸲸����³����� 
                  a���޸Ĳ���cl����4��clֻ�޸�2��cl�� 
                  b���޸�����cl���������������4096��cl�� 
     3������޸Ľ�� 
     4�����cl���Զ�Ӧcl����Ϣ 
   */
   var csName = CHANGEDPREFIX + "_cs16074";
   var oldClPerfix = CHANGEDPREFIX + "_cl16074_";
   var newClPerfix = CHANGEDPREFIX + "_newcl16074_";
   commDropCS( db, csName, true, "drop CS " + csName );
   var cs = commCreateCS( db, csName, true, "create CS1" );
   for( var i = 1; i <= 4096; i++ )
   {
      commCreateCL( db, csName, oldClPerfix + i, {}, true, false, "create cl in the beginning" );
   }
   //�޸Ĳ���cl
   for( var i = 1; i <= 100; i++ )
   {
      cs.renameCL( oldClPerfix + i, newClPerfix + i );
   }
   for( var i = 1; i <= 100; i++ )
   {
      checkRenameCLResult( csName, oldClPerfix + i, newClPerfix + i )
   }
   //�޸�ȫ��cl

   for( var i = 101; i <= 4096; i++ )
   {
      cs.renameCL( oldClPerfix + i, newClPerfix + i );
   }

   checkRenameAllCL( csName, newClPerfix );

   commDropCS( db, csName, true, "drop CS " + csName );
}

function checkRenameAllCL ( csName, newClPerfix )
{
   var cur = db.snapshot( SDB_SNAP_COLLECTIONSPACES, { Name: csName }, { Collection: 1 } );
   //var clInfos = cur["Collection"].toArray();
   while( cur.next() )
   {
      var ret = cur.current().toObj();
      var num = 0;
      for( var i = 0; i < ret.Collection.length; i++ )
      {
         var str = ret.Collection[i].Name;
         if( str.indexOf( newClPerfix ) != -1 )
         {
            num++;
         }
         else
         {
            println( "cl name is wrong : " + str );
         }
      }
      assert.equal( num, 4096 );
   }
   cur.close();
}