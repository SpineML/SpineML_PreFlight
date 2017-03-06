#include <string>
#include <iostream>
#include "util.h"

using namespace std;
using namespace spineml;

int main()
{
    pair<string,string> p = Util::getDistWithDimension("UNI(1,2,3)ms");
    cout << "dist string '" << p.first << "' with dim '" << p.second << "'" << endl;
    p = Util::getDistWithDimension("UNI(1,2,3)");
    cout << "dist string '" << p.first << "' with dim '" << p.second << "'" << endl;
    p = Util::getDistWithDimension("UNI(1,2,3ms");
    cout << "dist string '" << p.first << "' with dim '" << p.second << "'" << endl;
    p = Util::getDistWithDimension("UNI(1,2,3");
    cout << "dist string '" << p.first << "' with dim '" << p.second << "'" << endl;
    return 0;
}
