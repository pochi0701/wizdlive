<?
path = _POST.path;
errMsg = '<!-- no error -->';
code = _POST.code;
if( code.length > 0 ){
    //bakファイル作成
    bak = path+".bak";
    if( FILE.file_exists(path) ){
        FILE.copy(path,bak);
    }
    //保存
    if (! FILE.saveToFile(path, code)) {
        print( "<html><head></head><body><script type=\"text/javascript\">alert( 'error! ("+path+" not saved)' );</script></body><html>");
    }else{
        print( "<html><head></head><body><script type=\"text/javascript\">alert( '"+path+" saved!' );</script></body><html>");
    }
}
?>
