<html>
<head><title>環境変数一覧</title></head>
<body>
<pre>
<?
   print( "_SERVER<br/>\n"); 
   for( i=0 ; i<Object.keys(_SERVER).length ; i++ ){
         print( Object.keys(_SERVER)[i]+"\n" );
   } 
   print( "_GET<br/>\n )";
   for( i=0 ; i<Object.keys(_GET).length ; i++ ){
         print( Object.keys(_GET)[i]+"\n" );
   }
?>
</pre>
</body>
</html>
