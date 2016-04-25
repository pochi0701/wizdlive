//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
#include <stdio.h>
#include "dregex.h"
int main(int argc, char* argv[])
{
  // 正規表現マッチ
  dregex rem;
  rem.compile("a$", REG_EXTENDED|REG_ICASE|REG_NOSUB);
  // コンストラクタでコンパイルもしてくれるので、インスタンス作成とコンパイルは、下記のように同時にやってもいい。
  // dregex rem("a$", REG_EXTENDED|REG_ICASE|REG_NOSUB);
  printf("%s\n", rem.match("aaa bbb ccc") ? "match": "no match");

  // 正規表現マッチ・インスタンス不要＆その都度パターンをコンパイルする版(REG_NOSUBは不要)
  printf("%s\n", dregex::match("aaa bbb ccc", "b+", REG_EXTENDED) ? "match" : "no match");

  // 正規表現置換
  dregex rer;
  string result;
  rer.compile("t\\w+", REG_EXTENDED|REG_ICASE);
  rer.replace(&result, "This is a dog. this is a fish.", "She", true);
  printf("%s\n", result.c_str());

  // 正規表現置換・インスタンス不要＆その都度パターンをコンパイルする版(REG_NOSUBしても無効)
  dregex::replace(&result, "Sasebo, JAPAN", "(\\w+), *(\\w+)", "\\2, \\1", REG_EXTENDED|REG_NOSUB, false);
  printf("%s\n", result.c_str());

  // 正規表現スプリット
  dregex res;
  vector<string> ar;
  res.compile("\\s*[,;]\\s*",REG_EXTENDED);
  res.split(&ar, "aaa, bbb,ccc; d ee ; ff", true);
  for (unsigned int i=0; i<ar.size(); i++)
    printf("%d: %s\n", i, ar.at(i).c_str());

  // 正規表現スプリット・インスタンス不要＆その都度パターンをコンパイルする版(REG_NOSUBしても無効)
  ar.clear();
  dregex::split(&ar, "a+b+c", "\\+", REG_EXTENDED, true);
  for (unsigned int i=0; i<ar.size(); i++)
    printf("%d: %s\n", i, ar.at(i).c_str());

  return 0;
}
 
