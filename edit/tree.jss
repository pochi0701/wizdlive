<?
function u(str){
    return encodeURI(str);
}
//URLエンコード
function size_num_read(size) {
    bytes = ["B","KB","MB","GB","TB"];
    for( i=0 ; i < 5 ; i++) {
        if(size > 1024){
            size = size / 1024;
        }else{
            break;
        }
    }
    //return "<span style=\"font-size: small\">".	round($size, 2).$val."</span>";
    return (round(size*100)/100)+bytes[i];
}
//変数取得
base    = _SERVER.DOCUMENT_ROOT;
root    = _GET.root;
newDir  = _GET.dirName;
newFile = _GET.FileName;
del     = _GET.del;
renf    = _GET.renf;
rent    = _GET.rent;
vaf     = _GET.vaf;
search  = _GET.search;

//ユーザ限定処理
//初回の処理
if( root.length==0){
    root = _SERVER.DOCUMENT_ROOT;
}
//ファイル削除
if( del.length>0 ){
    if( FILE.file_exists(del) ){
        FILE.unlink(del);
    }else if ( FILE.dir_exists(del) ){
        FILE.rmdir(del);
    }
    //リネーム
}else if( renf.length>0 && rent.length>0 ){
    if( FILE.file_exists(renf) && (! FILE.file_exists(rent))){
        FILE.rename(renf,rent);
    }else if ( FILE.file_exists(rent) ){
        print( "すでに同名のファイルが存在します" );
    }
    //パス名取得、作成
}else if (FILE.dir_exists(root) && newDir.length>0 ) {
    path = root+"/"+newDir;
    if (!FILE.file_exists(path)){
        FILE.mkdir(path);
    }
    root = path;
    //ファイル名取得、作成
}else if (FILE.dir_exists(root) && newFile.length>0 ) {
    path = root+"/"+newFile;
    if (!FILE.file_exists(path)){
        FILE.touch(path);
    }
}
//右端の/をなくす
while( root[root.length-1] == "/" ){
    root = root.substring(0,root.length-1);
}
me=_SERVER.SCRIPT_NAME;
?>
<!DOCTYPE html>
<html>
<head>
<title>file tree - <? print(root); ?></title>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
<!-- Bootstrap -->
<link href="css/bootstrap.min.css" rel="stylesheet" media="screen">
<link href="tree.css" rel="stylesheet" type="text/css" />
<link href="css/bootstrap-responsive.min.css" rel="stylesheet"  media="all" />

<script type="text/javascript">
<!--
var root = "<? print( root ); ?>";
function viewCode(url) {
    if( parent.right.pushCode){
        parent.right.pushCode(0);
    }
    parent.right.location.href=encodeURI(url);
    return false;
}
function viewData(path) {
    var base = "<? print( base ); ?>";
    //var pos = path.indexOf(path,root);
    path = "http://<? print( _SERVER.HTTP_HOST );?>"+path.substring(base.length,path.length);
    myWin = window.open(path);
}
function deleteFile(url,file) {
    if( window.confirm(file+"を削除します。よろしいですか？") ){
        location.href=encodeURI(url+"&root="+root);
    }
    return false;
}
function renameFile(url,file) {
    msg = window.prompt("変更するファイル名を入力してください",file);
    if( msg.length>0 && window.confirm(file+"を"+msg+"に変更します。よろしいですか？") ){
        location.href=encodeURI(url+"&rent="+root+"/"+msg+"&root="+root);
    }
    return false;
}
function createDir() {
    msg = window.prompt("作成するフォルダ名を入力してください");
    if( msg.length>0 && window.confirm("フォルダ"+msg+"を<? print( root ); ?>に作成します。よろしいですか？") ){
        location.href=encodeURI("<? print( me );?>?dirName="+msg+"&root="+root);
    }
    return false;
}
function createFile() {
    msg = window.prompt("作成するファイル名を入力してください");
    if( msg.length>0 && window.confirm("ファイル"+msg+"を<? print(root);?>に作成します。よろしいですか？") ){
        location.href=encodeURI("<? print( me );?>?FileName="+msg+"&root="+root);
    }
    return false;
}
function myreload(num) {
    location.href=encodeURI("<? print(me) ;?>?root="+root+"&vaf="+num);
}
function dl() {
    msg1 = window.prompt("ダウンロードするＵＲＬを入力してください","");
    msg2 = root;
    msg3 = "./dl.jss?target="+msg2+"&dl="+msg1;
    if( msg1.length>0){
        if( window.confirm(msg1+"をダウンロードします。よろしいですか？") ){
            parent.tmp.location.href=encodeURI(msg3);
        }
    }
}
function todo() {
    var Window_Option='titlebar=no,menubar=no,toolbar=no,location=no,scrollbars=no,status=no,resizable=no,width=500,height=200';
    var W1;
    W1=window.open("","mwin",Window_Option);
    W1.location.href='/main/todo.jss';//NN対策
}
function expt() {
    //ZIPを作って
    var W1;
    if( window.confirm(root+"を圧縮してダウンロードします。よろしいですか？") ){
        location.href= "zip.jss?path="+root;
    }
    return false;
}
function view() {
    var W1;
    W1=window.open("","_blank");
    W1.location.href= "view.jss?path="+root;
    //parent.right.location.href=encodeURI("./view.jss?path="+root);
    return false;
}

function memo() {
    var W1;
    W1=window.open("","_blank");
    W1.location.href= "data:text/html, <html contenteditable>";
    return false;
}

function logout() {
    //$_SESSION = array(); //すべてのセッション変数を初期化
    //session_destroy(); //セッションを破棄
    //top.window.close();
    top.location.href=encodeURI("login.jss?mode=LOGOUT");
    return true;
}
function sea() {
    msg1 = window.prompt("検索する文字列を入力してください","");
    if( msg1.length>0){
        location.href=encodeURI("<? print(me);?>?root="+root+"&vaf=1&search="+msg1);
    }
}
// -->
</script>
</head>
<body>
<div class="container">
  <div class="btn-toolbar">
    <div class="btn-group">
    <button class="btn dropdown-toggle btn-small" data-toggle="dropdown">Action <span class="caret"></span></button>
    <ul class="dropdown-menu">
    <li><a href="#" onClick="myreload(0);">再読込</a></li>
    <li><a href="#" onClick="createDir();">フォルダ作成</a></li>
    <li><a href="#" onClick="createFile();">ファイル作成</a></li>
    <li><a href="upload.jss?folder=<? print( encodeURI(root+"/")); ?>" target="_blank">アップロード</a></li>
    <li><a href="#" onClick="dl();">ダウンロード</a></li>
    <li><a href="#" onClick="todo();">Todo</a></li>
    <li><a href="#" onClick="view();">グラフィック表示</a></li>
    <li><a href="pawfaliki.jss" target="_blank">Ｗｉｋｉ</a></li>
    <li><a href="https://neon.cx/phpMyAdmin/" target="_blank">PHPMyAdmin</a></li>
    <li><a href="env.jss" target="_blank">環境変数</a></li>
    <li><a href="#" onClick="myreload(1);">全表示</a></li>
    <li><a href="#" onClick="sea();">検索</a></li>
    <li><a href="#" onClick="expt();return false;">エクスポート</a></li>
    <li><a href="#" onClick="memo();return false;">メモ</a></li>
    <li class="divider"></li>
    <li><a href="#" onClick="logout();">ログアウト</a></li>
    </ul>
    </div>
  </div>
  [<? print( root ); ?>]
  <div id="tree" />
  <table>
<?
  //親ディレクトリ
  root1 = root;
  filePath = dirname(root1);
  if ( filePath != ""  && filePath.indexOf(base)>=0 ){
      url = "?root="+encodeURI(filePath);
      print( "<tr><td><a href=\""+url+"\" class=\"lnk\">[<img src=\"image/up.gif\" border=\"0\">]</a></td><td></td><td></td></tr>\n");
  }
  //ディレクトリの場合
  if (FILE.dir_exists(root)) {
      //ディレクトリ読み込み
      files = eval(FILE.scandir(root));
      for( i=files.length-1; i>=0 ;i--){
          if( basename(files[i]) == "." || basename(files[i]) == ".." ){
              files.remove(files[i]);
          }
      }
      if( files.length>0 ){
          //sort($files);
          //check each folders
          for( i = 0 ; i < files.length ; i++ ){
              file = files[i];
              //print( basename(file)+"<br/>\n");
              filePath = root1+"/"+basename(file);
              //print( "["+filePath+"]<br/>\n" );
              if( FILE.dir_exists(file) ){
                  //make link tag
                  stat = eval(FILE.file_stat(file));
                  title = stat.date+" "+stat.permission;
                  url1 = "?root="+encodeURI(filePath);
                  url2 = "?root="+filePath+"&del="+filePath;
                  url3 = "?root="+filePath+"&renf="+filePath;
                  print( "<tr><td><a href=\""+url1+"\" class=\"lnk\" title=\""+title+"\">["+basename(file)+"]</a></td>");
                  //print( "<td>".stat.date."</td>";
                  //print "<td></td>"
                  print( "<td><a href=\"#\" onClick=deleteFile(\""+url2+"\",\""+file+"\") title=\"Delete Folder\"><img src=\"image/trash.gif\" border=\"0\"/></a></td>");
                  print( "<td><a href=\"#\" onClick=renameFile(\""+url3+"\",\""+file+"\") title=\"Rename Folder\"><img src=\"image/rename.gif\" border=\"0\"/></a></td></tr>\n");
              }
          }
          //check each files.
          for( i=0 ; i < files.length ; i++ ){
              file = files[i];
              filePath = root1+"/"+basename(file);
              if( FILE.file_exists(filePath) ){
                  ext = FILE.extractFileExt(file);
                  if( vaf!=0 || ext.toLowerCase() != "bak"){
                      //make link tag
                      stat = FILE.file_stat(filePath);
                      title = stat.date+" "+stat.permission;
                      url1 = "editor.jss?path="+filePath;
                      url2 = me+"?del="+filePath;
                      url3 = me+"?renf="+filePath;
                      lnk = "lnk";
                      //if( ! in_array(ext,array("swf","jpg","gif","png"))){
                      //    if( search.length>0){
                      //        fil = FILE.loadFromFile(filePath);
                      //        if( fil.indexOf(search)<0){
                      //        }else{
                      //            lnk = "lnkf";
                      //        }
                      //    }
                          print( "<tr><td><a href=\"#\" class=\""+lnk+"\" title=\""+title+"\" onClick=viewCode(\""+url1+"\") >"+basename(file)+"</a></td>");
                      //}else{
                      //    print( "<tr><td><a href=\"#\" class=\""+lnk+"\" title=\""+title+"\" onClick=viewData(\""+filePath+"\") >"+basename(file)+"</a></td>" );
                      //}
                      //print "<td>".date("m/d H:i:s", filemtime($filePath))."</td>"; 
                      //print "<td align=\"right\">".size_num_read(filesize($filePath))."</td>";
                      print( "<td><a href=\"#\" onClick=deleteFile(\""+url2+"\",\""+basename(file)+"\") title=\"Delete File\"><img src=\"image/trash.gif\" border=\"0\"/></a></td>");
                      print( "<td><a href=\"#\" onClick=renameFile(\""+url3+"\",\""+basename(file)+"\") title=\"REname File\"><img src=\"image/rename.gif\" border=\"0\"/></a></td>");
                      print( "</tr>\n");
                  }
              }
          }
      }
  }
  print("</table>\n");
  print( "<hr/>");
?>
  <script src="js/jquery.js"></script>
  <script src="js/bootstrap.min.js"></script>
</div>
</body>
</html>
