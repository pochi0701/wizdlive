#include <iostream>
#include <regex.h>

int main() {
    
    using namespace std;
    
    const char *target = "==aaaa==\nafdsfdsa==bbbb==fdfdsfd";
    
    const char *re = "==([^=]*[^=]*)==";
    
    regex_t regexBuffer;
    
    if( regcomp( &regexBuffer, re, REG_EXTENDED | REG_NEWLINE ) != 0 ) {
        cerr << "regex compile failed" << endl;
        return 1;
    }
    
    regmatch_t patternMatch[3];
    
    if( regexec( &regexBuffer, target, 1, patternMatch, REG_NEWLINE  ) == 0 ) {
        for( int i=0; i<3; i++ ) {
            int start = patternMatch[i].rm_so;
            int end = patternMatch[i].rm_eo;
            int len = end - start;
            cout << string(target, start, len) << endl;
        }
    }
    
    regfree( &regexBuffer );
    return 0;
}
