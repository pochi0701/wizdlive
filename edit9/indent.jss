<?
///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
//リクエスト文字列取得
function array_get(arr, key) {
    return (array_key_exists(key, arr)) ?    arr[key] : null;
}
function v(arr,key) {
    return (array_key_exists(key, arr)) ?    arr[key] : 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
function h(str){
    //if(is_array(str)){
    //    return array_map("h",str);
    //}else{
        return htmlspecialchars(str);
    //}
}
function mymax(a,b){
    return (a>b)?a:b;
}
function space(n){
    spc = "";
    for( i = 0 ; i <n ; i++ ){
        spc = spc+" ";
    }
    return spc;
}
function myadd(arr,st,ed,add){
    for( i = st ; i <ed ; i++ ){
        arr[i] = v(arr,i)+add;
    }
    return arr;
}
code = _POST.filename;

if( code.length>0 ){
    data = FILE.loadFromFile(code);
    data = explode("\n",data);
    //ＴＲＩＭ
    for( i=0,size=count(data);i<size;i++){
        data[i] = trim(data[i]);
    }
    sp=array();
    for( i=0,sizei=count(data);i<sizei;i++){
        line = data[i];
        inq = NULL;
        bc = 0;
        for( j=0,sizej=strlen(line);j<sizej;j++){
            ch =  line[j];
            if( isset(inq) && ch == '\\' ){
                continue;
            }else if( isset(inq) && ch == inq ){
                unset(inq);
            }else if( isset(inq) ){
                continue;
            }else{
                if( ch == '"' ){
                    inq = '"';
                }else if( ch == "'" ){
                    inq = "'";
                }else{
                    if( ch == "{" ){
                        if( bc<=0){
                            bc++;
                        }else{
                            sp = myadd(sp,i+1,sizei,4);
                        }
                    }else if ( ch == "}" ){
                        if( bc<= 0){
                            sp = myadd(sp,i,sizei,-4);
                        }else{
                            bc--;
                        }
                    }
                }
            }
        }
        while( bc != 0 ){
            if( bc>0 ){
                sp = myadd(sp,i+1,sizei,4);
                bc--;
            }else if ( bc<0 ){
                sp = myadd(sp,i,sizei,-4);
                bc++;
            }
        }
    }
    
    for( i=0,sizei=count(data);i<sizei;i++){
        data[i] = space(v(sp,i))+data[i];
    }
    data = join("\n",data);
    //保存
    
    errMsg = "";
    //print data;
    if (file_put_contents(code, data) === false) {
        errMsg = 'error! (not saved)';
    }
    printi( errMsg );
    print( "<a href=\"./editor.php?path="+code+"\">"+code+"完了：戻る</a>";
}else{
    print( "<form  action=\"?\" method=\"post\">\n");
    print( "<input type=\"text\" name=\"filename\" value=\"\">\n");
    print( "<input type=\"submit\" value=\"Indent\">\n");
    print( "</form>\n");
    print( "<a href=\"./editor.php\">戻る</a>");
}
?>
