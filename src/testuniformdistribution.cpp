#include <string>
#include <iostream>
#include "uniformdistribution.h"

using namespace std;
using namespace spineml;

int main()
{
    UniformDistribution ud;
    ud.setFromString ("OUNI(10.27,2,125)");
    cout << "ud with min " << ud.minimum << ", max " << ud.maximum << ", seed " <<  ud.seed << endl;
    return 0;
}
