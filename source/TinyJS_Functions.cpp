/*
 * TinyJS
 *
 * A single-file Javascript-alike engine
 *
 * - Useful language functions
 *
 * Authored By Gordon Williams <gw@pur3.co.uk>
 *
 * Copyright (C) 2009 Pur3 Ltd
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to do
 * so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "TinyJS_Functions.h"
#include <math.h>
#include <cstdlib>
#include <sstream>
#include <stdio.h>
using namespace std;
void headerCheck(int mysocket, int* printed, wString* headerBuf,int flag);
// ----------------------------------------------- Actual Functions
void js_print(CScriptVar *v, void *userdata) {
    CTinyJS* js = (CTinyJS*)userdata;
    headerCheck(js->socket, &(js->printed), js->headerBuf,1);
    wString str = v->getParameter("text")->getString();
    int num = write( js->socket, str.c_str(), str.length());
    if( num <0 ){
        debug_log_output( "Script Write Error at js_print" );
    }
    //debug_log_output( "print %s %d", str.c_str(), num );
    //printf("%s", v->getParameter("text")->getString().c_str());
}

void scTrace(CScriptVar *c, void *userdata) {
    CTinyJS *js = (CTinyJS*)userdata;
    js->root->trace();
}

void scObjectDump(CScriptVar *c, void *) {
    c->getParameter("this")->trace("> ");
}

void scObjectClone(CScriptVar *c, void *) {
    CScriptVar *obj = c->getParameter("this");
    c->getReturnVar()->copyValue(obj);
}

void scMathRand(CScriptVar *c, void *) {
    c->getReturnVar()->setDouble((double)rand()/RAND_MAX);
}

void scMathRandInt(CScriptVar *c, void *) {
    int min = c->getParameter("min")->getInt();
    int max = c->getParameter("max")->getInt();
    int val = min + (int)(rand()%(1+max-min));
    c->getReturnVar()->setInt(val);
}

void scCharToInt(CScriptVar *c, void *) {
    wString str = c->getParameter("ch")->getString();;
    int val = 0;
    if (str.length()>0)
        val = (int)str.c_str()[0];
    c->getReturnVar()->setInt(val);
}

void scStringIndexOf(CScriptVar *c, void *) {
    wString str = c->getParameter("this")->getString();
    wString search = c->getParameter("search")->getString();
    size_t p = str.find(search);
    int val = (p==string::npos) ? -1 : p;
    c->getReturnVar()->setInt(val);
}

void scStringSubstring(CScriptVar *c, void *) {
    wString str = c->getParameter("this")->getString();
    int lo = c->getParameter("lo")->getInt();
    int hi = c->getParameter("hi")->getInt();

    int l = hi-lo;
    if (l>0 && lo>=0 && lo+l<=(int)str.length())
      c->getReturnVar()->setString(str.substr(lo, l));
    else
      c->getReturnVar()->setString("");
}
void scStringSubstr(CScriptVar *c, void *) {
    wString str = c->getParameter("this")->getString();
    int lo = c->getParameter("lo")->getInt();
    int hi = c->getParameter("hi")->getInt();

    if (hi>0 && lo>=0 && lo+hi<=(int)str.length())
      c->getReturnVar()->setString(str.substr(lo, hi));
    else
      c->getReturnVar()->setString("");
}

void scStringCharAt(CScriptVar *c, void *) {
    wString str = c->getParameter("this")->getString();
    int p = c->getParameter("pos")->getInt();
    if (p>=0 && p<(int)str.length()){
      c->getReturnVar()->setString(str.substr(p, 1));
    }else{
      c->getReturnVar()->setString("");
    }
}

void scStringCharCodeAt(CScriptVar *c, void *) {
    wString str = c->getParameter("this")->getString();
    int p = c->getParameter("pos")->getInt();
    if (p>=0 && p<(int)str.length())
      c->getReturnVar()->setInt(str.at(p));
    else
      c->getReturnVar()->setInt(0);
}

void scStringSplit(CScriptVar *c, void *) {
    wString str = c->getParameter("this")->getString();
    wString sep = c->getParameter("separator")->getString();
    CScriptVar *result = c->getReturnVar();
    result->setArray();
    int length = 0;

    //consider sepatator length;
    int inc = sep.length();
    size_t pos = str.find(sep);
    while (pos != string::npos) {
      result->setArrayIndex(length++, new CScriptVar(str.substr(0,pos)));
      str = str.substr(pos+inc);
      pos = str.find(sep);
    }

    if (str.size()>0)
      result->setArrayIndex(length++, new CScriptVar(str));
}
//Replace
void scStringReplace(CScriptVar *c, void *) {
    wString str = c->getParameter("this")->getString();
    wString before = c->getParameter("before")->getString();
    wString after = c->getParameter("after")->getString();
    //strの中のbeforeを探す
    size_t pos = 0;
    pos = str.find(before,pos);
    while( pos != string::npos){
        str = str.substr(0,pos)+after+str.substr(pos+before.length());
        pos = str.find(before,pos); 
    }
    c->getReturnVar()->setString(str);
}

void scStringFromCharCode(CScriptVar *c, void *) {
    char str[2];
    str[0] = c->getParameter("char")->getInt();
    str[1] = 0;
    c->getReturnVar()->setString(str);
}

void scIntegerParseInt(CScriptVar *c, void *) {
    wString str = c->getParameter("str")->getString();
    int val = strtol(str.c_str(),0,0);
    c->getReturnVar()->setInt(val);
}

void scIntegerValueOf(CScriptVar *c, void *) {
    wString str = c->getParameter("str")->getString();

    int val = 0;
    if (str.length()==1)
      val = str[0];
    c->getReturnVar()->setInt(val);
}

void scJSONStringify(CScriptVar *c, void *) {
    wString result;
    c->getParameter("obj")->getJSON(result);
    c->getReturnVar()->setString(result.c_str());
}

void scExec(CScriptVar *c, void *data) {
    CTinyJS *tinyJS = (CTinyJS *)data;
    wString str = c->getParameter("jsCode")->getString();
    tinyJS->execute(str);
}

void scEval(CScriptVar *c, void *data) {
    CTinyJS *tinyJS = (CTinyJS *)data;
    wString str = c->getParameter("jsCode")->getString();
    c->setReturnVar(tinyJS->evaluateComplex(str).var);
}

void scArrayContains(CScriptVar *c, void *data) {
  CScriptVar *obj = c->getParameter("obj");
  CScriptVarLink *v = c->getParameter("this")->firstChild;

  bool contains = false;
  while (v) {
      if (v->var->equals(obj)) {
        contains = true;
        break;
      }
      v = v->nextSibling;
  }

  c->getReturnVar()->setInt(contains);
}

void scArrayRemove(CScriptVar *c, void *data) {
  CScriptVar *obj = c->getParameter("obj");
  vector<int> removedIndices;
  CScriptVarLink *v;
  // remove
  v = c->getParameter("this")->firstChild;
  while (v) {
      if (v->var->equals(obj)) {
        removedIndices.push_back(v->getIntName());
      }
      v = v->nextSibling;
  }
  // renumber
  v = c->getParameter("this")->firstChild;
  while (v) {
      int n = v->getIntName();
      int newn = n;
      for (size_t i=0;i<removedIndices.size();i++)
        if (n>=removedIndices[i])
          newn--;
      if (newn!=n)
        v->setIntName(newn);
      v = v->nextSibling;
  }
}

void scArrayJoin(CScriptVar *c, void *data) {
  wString sep = c->getParameter("separator")->getString();
  CScriptVar *arr = c->getParameter("this");

  wString sstr;
  int l = arr->getArrayLength();
  for (int i=0;i<l;i++) {
    if (i>0){
       sstr += sep;
    }
    sstr += arr->getArrayIndex(i)->getString();
  }

  c->getReturnVar()->setString(sstr.c_str());
}
//ファイル存在チェック
void scFileExists(CScriptVar *c, void *) {
    wString path = c->getParameter("path")->getString();
    int flag = wString::FileExists(path);
    c->getReturnVar()->setInt(flag);
}
//ディレクトリ存在チェック
void scDirExists(CScriptVar *c, void *) {
    wString path = c->getParameter("path")->getString();
    int flag = wString::DirectoryExists(path);
    c->getReturnVar()->setInt(flag);
}
//htmlspecialchars
void scHtmlSpecialChars(CScriptVar *c, void *) {
    wString uri = c->getParameter("uri")->getString();
    uri = uri.htmlspecialchars();
    c->getReturnVar()->setString(uri);
}
//encodeURI
void scEncodeURI(CScriptVar *c, void *) {
    wString uri = c->getParameter("uri")->getString();
    uri = uri.uri_encode();
    c->getReturnVar()->setString(uri);
}
//dirname
void scDirname(CScriptVar *c, void *) {
    wString uri = c->getParameter("uri")->getString();
    while( uri.length() > 0 && uri[uri.length()-1] != '/' ){
        uri = uri.SubString( 0, uri.length() - 1 );
    }
    if( uri.length() > 0 ){
        uri = uri.SubString( 0, uri.length() - 1 );
    }else{
        uri = ".";
    }
    c->getReturnVar()->setString(uri);
}
//base
void scBasename(CScriptVar *c, void *) {
    wString uri = c->getParameter("uri")->getString();
    int len = uri.length()-1;
    while( len >= 0 && uri[len] != '/' ){
       len--;
    }
    uri = uri.SubString( len+1, uri.length()-len-1 );
    c->getReturnVar()->setString(uri);
}

//ScanDir
void scScanDir(CScriptVar *c, void *) {
    wString uri = c->getParameter("uri")->getString();
    uri = wString::EnumFolder(uri);
    c->getReturnVar()->setString(uri);
}
//ExtractFileExt
void scExtractFileExt(CScriptVar *c, void *) {
    wString uri = c->getParameter("uri")->getString();
    uri = wString::ExtractFileExt(uri);
    c->getReturnVar()->setString(uri);
}
//toLowerCase
void scToLowerCase(CScriptVar *c, void *) {
    wString str = c->getParameter("this")->getString();
    char* String = str.c_str();
    for( unsigned int i = 0 ; i < str.length() ; i++ ){
        String[i] = tolower(String[i]);
    }
    c->getReturnVar()->setString(str);
}
//toUpperCase
void scToUpperCase(CScriptVar *c, void *) {
    wString str = c->getParameter("this")->getString();
    char* String = str.c_str();
    for( unsigned int i = 0 ; i < str.length() ; i++ ){
        String[i] = toupper(String[i]);
    }
    c->getReturnVar()->setString(str);
}
//ファイル属性など
void scFileStats(CScriptVar *c, void *) {
    wString path = c->getParameter("path")->getString();
    wString json = wString::FileStats(path);
    c->getReturnVar()->setString(json);
}
//ファイル内容取得
void scLoadFromFile(CScriptVar *c, void *) {
    wString path = c->getParameter("path")->getString();
    wString data;
    data.LoadFromFile(path);
    c->getReturnVar()->setString(data);
}
//ファイル削除
void scUnlink(CScriptVar *c, void *) {
    wString path = c->getParameter("path")->getString();
    int res = unlink(path.c_str());
    int ret = (res==0)?true:false;
    c->getReturnVar()->setInt(ret);
}
//ファイル作成
void scTouch(CScriptVar *c, void *) {
    wString path = c->getParameter("path")->getString();
    int ret=true;
    FILE* fp = fopen(path.c_str(),"w");
    if( fp == NULL ){
        ret = false;
    }else{
        fclose( fp );
    } 
    c->getReturnVar()->setInt(ret);
}
//リネーム
void scRename(CScriptVar *c, void *) {
    wString pathf = c->getParameter("pathf")->getString();
    wString patht = c->getParameter("patht")->getString();
    int ret = wString::RenameFile(pathf,patht);
    c->getReturnVar()->setInt(ret);
}
//フォルダ作成
void scMkdir(CScriptVar *c, void *) {
    wString path = c->getParameter("path")->getString();
    int ret = wString::CreateDir(path);
    c->getReturnVar()->setInt(ret);
}
//フォルダ削除
void scRmdir(CScriptVar *c, void *) {
    wString path = c->getParameter("path")->getString();
    int res = rmdir(path.c_str());
    int ret = (res==0)?true:false;
    c->getReturnVar()->setInt(ret);
}
//ファイル保存
void scSaveToFile(CScriptVar *c, void *) {
    wString path = c->getParameter("path")->getString();
    wString data = c->getParameter("data")->getString();
    int res = data.SaveToFile(path);
    int ret = (res==0)?true:false;
    c->getReturnVar()->setInt(ret);
}
//command実行
void scCommand(CScriptVar *c, void *) {
    wString path = c->getParameter("path")->getString();
    int res = system(path.c_str());
    int ret = (res==0)?true:false;
    c->getReturnVar()->setInt(ret);
}
//Header
void scHeader(CScriptVar *c, void *userdata) {
    CTinyJS* js = (CTinyJS*)userdata;
    headerCheck(js->socket, &(js->printed), js->headerBuf, 0);
    wString str = c->getParameter("str")->getString();
    int res = js->headerBuf->header(str.c_str());
    int ret = (res==0)?true:false;
    c->getReturnVar()->setInt(ret);
}
void scFileCopy(CScriptVar *c, void *) {
    wString pathf = c->getParameter("pathf")->getString();
    wString patht = c->getParameter("patht")->getString();
    int res = wString::FileCopy(pathf.c_str(),patht.c_str());
    int ret = (res==0)?true:false;
    c->getReturnVar()->setInt(ret);
}
// ----------------------------------------------- Register Functions
void registerFunctions(CTinyJS *tinyJS) {
    tinyJS->addNative("function exec(jsCode)", scExec, tinyJS); // execute the given code
    tinyJS->addNative("function eval(jsCode)", scEval, tinyJS); // execute the given wString (an expression) and return the result
    tinyJS->addNative("function trace()", scTrace, tinyJS);
    tinyJS->addNative("function Object.dump()", scObjectDump, 0);
    tinyJS->addNative("function Object.clone()", scObjectClone, 0);
    tinyJS->addNative("function Math.rand()", scMathRand, 0);
    tinyJS->addNative("function Math.randInt(min, max)", scMathRandInt, 0);
    tinyJS->addNative("function charToInt(ch)", scCharToInt, 0); //  convert a character to an int - get its value
    tinyJS->addNative("function String.indexOf(search)", scStringIndexOf, 0); // find the position of a wString in a string, -1 if not
    tinyJS->addNative("function String.substring(lo,hi)", scStringSubstring, 0);
    tinyJS->addNative("function String.substr(lo,hi)", scStringSubstr, 0);
    tinyJS->addNative("function String.charAt(pos)", scStringCharAt, 0);
    tinyJS->addNative("function String.charCodeAt(pos)", scStringCharCodeAt, 0);
    tinyJS->addNative("function String.fromCharCode(char)", scStringFromCharCode, 0);
    tinyJS->addNative("function String.split(separator)", scStringSplit, 0);
    tinyJS->addNative("function String.replace(before,after)",scStringReplace, 0 );
    tinyJS->addNative("function Integer.parseInt(str)", scIntegerParseInt, 0); // wString to int
    tinyJS->addNative("function Integer.valueOf(str)", scIntegerValueOf, 0); // value of a single character
    tinyJS->addNative("function JSON.stringify(obj, replacer)", scJSONStringify, 0); // convert to JSON. replacer is ignored at the moment

    // JSON.parse is left out as you can (unsafely!) use eval instead
    tinyJS->addNative("function Array.contains(obj)", scArrayContains, 0);
    tinyJS->addNative("function Array.remove(obj)", scArrayRemove, 0);
    tinyJS->addNative("function Array.join(separator)", scArrayJoin, 0);
    tinyJS->addNative("function print(text)", &js_print, tinyJS);
    tinyJS->addNative("function FILE.file_exists(path)", scFileExists, 0 );
    tinyJS->addNative("function FILE.dir_exists(path)", scDirExists, 0 );
    tinyJS->addNative("function htmlspecialchars(uri)", scHtmlSpecialChars, 0 );
    tinyJS->addNative("function encodeURI(uri)", scEncodeURI, 0 );
    tinyJS->addNative("function dirname(uri)", scDirname, 0 );
    tinyJS->addNative("function basename(uri)", scBasename, 0 );
    tinyJS->addNative("function FILE.scandir(uri)", scScanDir, 0 );
    tinyJS->addNative("function FILE.extractFileExt(uri)", scExtractFileExt, 0 );
    tinyJS->addNative("function String.toLowerCase()", scToLowerCase, 0 );
    tinyJS->addNative("function String.toUpperCase()", scToUpperCase, 0 );
    tinyJS->addNative("function FILE.file_stat(path)", scFileStats, 0 );
    tinyJS->addNative("function FILE.loadFromFile(path)", scLoadFromFile, 0 );
    tinyJS->addNative("function FILE.unlink(path)", scUnlink, 0 );
    tinyJS->addNative("function FILE.touch(path)", scTouch, 0 );
    tinyJS->addNative("function FILE.rename(pathf,patht)", scRename, 0 );
    tinyJS->addNative("function FILE.mkdir(path)", scMkdir, 0 );
    tinyJS->addNative("function FILE.rmdir(path)", scRmdir, 0 );
    tinyJS->addNative("function FILE.saveToFile(path,data)", scSaveToFile, 0 );
    tinyJS->addNative("function FILE.copy(pathf,patht)", scFileCopy, 0 );
    tinyJS->addNative("function command(path)", scCommand, 0 );
    tinyJS->addNative("function header(str)", scHeader, tinyJS );
}

