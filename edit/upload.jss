<!DOCTYPE html PUBLIC "-//IETF//DTD HTML 2.0//EN">
<html>
<head>
<meta http-equiv="Content-Type" content="text/html; charset=UTF-8">
<script type="text/javascript" src="js/jquery-1.2.3.min.js"></script>
<script type="text/javascript" src="js/jquery.form.js"></script>
<script type="text/javascript" src="js/jqueryprogressbar.js"></script>
<script type="text/javascript" src="js/APC.js"></script>
<link rel="stylesheet" type="text/css" href="js/progressbar.css">
<script type="text/javascript">
<!--
function disableSubmit( btn )
{
    progressStart();
}
//-->
</script>
</head>
<html>
<title>UPLOADER</title>
<body>
<?
function TrimPort($ip)
{
    $pos = strpos($ip,":");
    if( $pos ){
        return substr($ip,0,$pos);
    }else{
        return $ip;
    }
}
/*
* ファイルのアップロードを行う。
* @param $_FILES：アップロード情報（POST）
*/
$uf = "";
function uploadfile( $_FILES,$folder  ){
    $MAX_FILE_SIZE = 2000 * 1024 *1024;                                      // アップロードするファイルの最大バイト数の設定
    $uploaddir    = $folder;//dirname($_SERVER['SCRIPT_FILENAME'])."/";                                    // アップロードフォルダ名
    $file_name    = $_FILES['filename']['name'];                          // ファイル名
    $tmp_filename = $_FILES['filename']['tmp_name'];
    $retvalue = true;
    global $uf;
    $uf = "http://".array_get($_SERVER,'SERVER_NAME').dirname(array_get($_SERVER,'SCRIPT_NAME'))."/{$file_name}";
    
    if( file_exists( $tmp_filename ) ){
        // filesize() は 2GB より大きなファイルについては期待とは違う値を返すことがあります。
        $file_size = filesize( $tmp_filename );  // ファイルサイズ
        if( $file_size <= $MAX_FILE_SIZE ){
            // アップするファイルとして JPG GIF PNG の拡張子を持ったのもしか実行しない
            //if( ereg("(^[a-zA-Z0-9_-]+)\.(jpg)|(^[a-zA-Z0-9_-]+)\.(gif)(^[a-zA-Z0-9_-]+)\.(png)|", $file_name) ){
                $ret = move_uploaded_file( $tmp_filename , $uploaddir.$file_name );
                if( true == $ret ){
                    changePermission( $uploaddir.$file_name , 0766 );
                }else{
                    print ("アップロード失敗");
                    $retvalue = false;
                }
            //}
        }else{
            print ("アップロード失敗");
            $retvalue = false;
        }
    }
    return $retvalue;
}


/**
* パーミッション変更
* @param       filename        :       ファイル名
* @param       pm              :       属性値
*/
function changePermission( $filename , $pm )
{
    if( 0 == file_exists( $filename ) )
    {
        return;
    }
    $ret = chmod( $filename , $pm );
}

//本体
$uploaded = 0;
$folder      = array_get($_REQUEST,'folder');
if( isset($_POST['uploadhide']) ){
    $uploaded = 2;
    if( 0 < strlen( $_FILES['filename']['name'] ) ){
        //アップロード
        $ret = uploadfile($_FILES,$folder);
        if( true == $ret ){
            $uploaded = 1;
            //拡張子と元に分ける
        }
    }
}

print( "<div id=\"banner\">\n");
print( "<h1>UPLOADER</h1>\n");
print( "</div>\n");
print( $name."さんこんにちわ。<br/>\n");
print( "アップロード可能なファイルはjpg,png,gifです。");
print( "<form id = \"formname\" name = \"formname\" action = \"".htmlspecialchars($_SERVER['PHP_SELF'])."\" method = \"POST\" enctype = \"multipart/form-data\">\n");
print( "<div class=\"popup\">\n");
if( $uploaded == 1){
    print( "<input type=\"text\" size=\"50\" value=\"{$uf}\"/><BR/>\n");
}else if ( $uploaded == 2 ){
    print( "アップロード失敗<BR/>\n");
}
print( "<input type=\"hidden\"                   name=\"APC_UPLOAD_PROGRESS\" value=\"progress_key\" />\n");
print( "<input type=\"hidden\" id=\"uploadhide\" name=\"uploadhide\" value = \"upload\">\n" );
print( "<input type=\"hidden\" id=\"folder\"     name=\"folder\"     value = \"{$folder}\">\n" );
print( "登録するファイル名を設定して登録ボタンを押して下さい。<br/>\n");
print( "<TABLE border=\"1\" cellspacing=\"1\">\n");
print( "  <TR>\n");
print( "    <TH>登録するファイル</TH>\n");
print( "  </TR>\n");
print( "  <TR>\n");
print( "    <TD><input type = \"file\" id = \"filename\" name = \"filename\" size=\"30\"></TD><br>\n");
print( "  </TR>\n");
print( "</TABLE>\n");
print( "<input type = \"submit\" name=\"upload\" value = \"　登　録　\" onClick=\"disableSubmit(this);\">\n");
print( "</div>\n");
print( "</form>\n");
?>
<div id="progressbar"></div>
<div id="status"></div>
<br>
</body>
</html>
