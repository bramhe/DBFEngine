#include "dbf.h"

using namespace std;

int main(int argc, char *argv[])
{

    std::cout << "Test of DBFEngine Started"<< std::endl;

    // any param is assumed to be a filename to do a read test and dump to std output
    if( argc > 1 )
    {
        string sFileReadTest = "test.dbf";
        sFileReadTest = argv[1]; // do a read test on the given file

        DBF mydbf;
        std::cerr << "Read Test of " << sFileReadTest << std::endl;
        int nRet = mydbf.open(sFileReadTest);
        if( nRet )
        {
            std::cerr << "Unable to Open File " << sFileReadTest << std::endl;
        } else
        {
            int nRecs = mydbf.GetNumRecords();
            int nFields = mydbf.GetNumFields();
            std::cout << "Open() found " << nRecs << " records, and " << nFields << " fields." << std::endl;
            mydbf.dumpAsCSV();
            std::cout << "Done Read Test " << std::endl;

            mydbf.close();
        }
    } else
    {
        // no param, means to run a test to create, read and delete a record in the dbf

        // now create a db
        std::cout << "Create file TestCreate.dbf from code" << std::endl;
        DBF newdbf;
        int nRet = newdbf.create("TestCreate.dbf",5);
        if( nRet == 0 )
        {
            // create the 5 fields
            fieldDefinition fd;
            strncpy(fd.cFieldName,"ID",10);
            fd.cFieldType='I';
            fd.FieldFlags=0;
            fd.uAutoIncrementStep=0;
            fd.uFieldOffset=0;
            fd.uLength=4;
            fd.uNumberOfDecimalPlaces=0;
            newdbf.assignField(fd,0);

            strncpy(fd.cFieldName,"FirstName",10);
            fd.cFieldType='C';
            fd.FieldFlags=0;
            fd.uAutoIncrementStep=0;
            fd.uFieldOffset=0;
            fd.uLength=20;
            fd.uNumberOfDecimalPlaces=0;
            newdbf.assignField(fd,1);

            strncpy(fd.cFieldName,"Weight",10);
            fd.cFieldType='F';
            fd.FieldFlags=0;
            fd.uAutoIncrementStep=0;
            fd.uFieldOffset=0;
            fd.uLength=4;
            fd.uNumberOfDecimalPlaces=0;
            newdbf.assignField(fd,2);

            strncpy(fd.cFieldName,"Age",10);
            fd.cFieldType='N';
            fd.FieldFlags=0;
            fd.uAutoIncrementStep=0;
            fd.uFieldOffset=0;
            fd.uLength=3;
            fd.uNumberOfDecimalPlaces=0;
            newdbf.assignField(fd,3);

            strncpy(fd.cFieldName,"Married",10);
            fd.cFieldType='L';
            fd.FieldFlags=0;
            fd.uAutoIncrementStep=0;
            fd.uFieldOffset=0;
            fd.uLength=1;
            fd.uNumberOfDecimalPlaces=0;
            newdbf.assignField(fd,4);


            // now create some records to test it!
            string s1[5]= {"1" ,"Ric G","210.5","43","T"};
            newdbf.appendRecord(&s1[0],5);

            string s2[5]={"1000" ,"Paul F","196.2","33","T"};
            newdbf.appendRecord(s2,5);

            string s3[5]={"20000" ,"Dean K","186.1","23","F"};
            newdbf.appendRecord(s3,5);

            string s4[5]={"300000" ,"Gary\" Q","175.9","13","F"};
            newdbf.appendRecord(s4,5);

            string s5[5]={"4000000" ,"Dan'e \"D, with comma and over sized field that will be truncated","65.2","6","F"};
            newdbf.appendRecord(s5,5);

            newdbf.close();

            std::cout << "Done Creating the DBF" << std::endl;

            std::cout << "Open the DBF for reading and writing" << std::endl;
            // now read the dbf back to test that it worked!
            DBF readTest;
            readTest.open("TestCreate.dbf",true);
            int nRecs = readTest.GetNumRecords();
            int nFields = readTest.GetNumFields();
            std::cout << "Open() found " << nRecs << " records, and " << nFields << " fields." << std::endl;
            readTest.dumpAsCSV();
            std::cout << "Done Reading a freshly created DBF! " << std::endl;

            std::cout << "Test Delete Record 1 in DBF! " << std::endl;
            readTest.markAsDeleted(1);
            readTest.dumpAsCSV();
            std::cout << "Done Test Delete Record DBF! " << std::endl;

            readTest.close();
        }
    }
    return 0;
}
