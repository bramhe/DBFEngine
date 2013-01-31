#include "dbf.h"

using namespace std;

int main(int argc, char *argv[])
{

    std::cout << "Started"<< std::endl;

    DBF mydbf;

    int nRet = mydbf.open("/home/rostafichuk/Downloads/marker.dbf");
    if( nRet )
        std::cerr << "Unable to Open File" << std::endl;

    int nRecs = mydbf.GetNumRecords();
    int nFields = mydbf.GetNumFields();
    std::cout << "Open() found " << nRecs << " records, and " << nFields << " fields." << std::endl;

    // output the fields and records as a csv file
    for( int f=0; f < nFields ; f++ )
    {
        if( f > 0 )
            std::cout << ",";

        string sFieldName = mydbf.GetFieldName(f);
        std::cout << sFieldName ;
    }
    std::cout << std::endl;

    for( int r = 0; r < nRecs ; r++ )
    {
        mydbf.loadRec(r);
        for( int f=0; f < nFields ; f++ )
        {
            if( f > 0 )
                std::cout << ",";

            string s = mydbf.readField(f);
            std::cout << s;


        }
        std::cout << std::endl;

    }

    std::cout << "Done" << std::endl;

    return 0;
}
