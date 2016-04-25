/*
* TinyJS
*
* A single-file Javascript-alike engine
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

/*
* This is a program to run all the tests in the tests folder...
*/

#include "TinyJS.h"
#include "TinyJS_Functions.h"
#include "TinyJS_MathFunctions.h"
#include <assert.h>
#include <sys/stat.h>
#include <string>
#include <sstream>
#include <stdio.h>
bool
run_test (const char *filename)
{
    struct stat results;
    if (!stat (filename, &results) == 0)
    {
        printf ("Cannot stat file! '%s'\n", filename);
        return false;
    }
    int size = results.st_size;
    FILE *file = fopen (filename, "rb");
    
    /* if we open as text, the number of bytes read may be > the size we read */
    if (!file)
    {
        printf ("Unable to open file! '%s'\n", filename);
        return false;
    }
    char *buffer = new char[size + 1];
    long actualRead = fread (buffer, 1, size, file);
    buffer[actualRead] = 0;
    buffer[size] = 0;
    fclose (file);
    CTinyJS s;
    registerFunctions (&s);
    registerMathFunctions (&s);
    s.root->addChild ("result", new CScriptVar ("0", SCRIPTVAR_INTEGER));
    try
    {
        s.execute (buffer);
    }
    catch (CScriptException * e)
    {
        printf ("ERROR: %s\n", e->text.c_str ());
    }
    bool pass = s.root->getParameter ("result")->getBool ();
    return pass;
}

int
main (int argc, char **argv)
{
    run_test("/home/kanazawa/tinyjs/trunk/tests/test013.js");
    return 0;
}