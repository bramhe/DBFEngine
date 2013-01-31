#ifndef DBF_H
#define DBF_H

#include <string.h>
#include <stdio.h>
#include <iostream>
#include <sstream>
#include <errno.h>
#include <math.h>

using namespace std;

typedef unsigned char uint8;
typedef short int uint16;
typedef int uint32;

#define MAX_FIELDS 255

struct fileHeader
{
    uint8 u8FileType;
    uint8 u8LastUpdateYear;
    uint8 u8LastUpdateMonth;
    uint8 u8LastUpdateDay;
    uint32 uRecordsInFile;
    uint16 uPositionOfFirstRecord;
    uint16 uRecordLength; // includes the delete flag (byte) at start of record
    uint32 Reserved1; // 16 bytes reserved
    uint32 Reserved2;
    uint32 Reserved3;
    uint32 Reserved4;
    uint8 uTableFlags;
    uint8 uCodePage;
    uint16 Reserved5; // put zeros in all reserved fields
};

// after the file header, we can have n field definition records, terminated by the byte 0x0D
struct fieldDefinition
{
    char cFieldName[11];
    char cFieldType;
    uint32 uFieldOffset; // location of field from start of record
    uint8 uLength; // Length of Field in bytes
    uint8 uNumberOfDecimalPlaces;
    uint8 FieldFlags;
    uint32 uNextAutoIncrementValue;
    uint8 uAutoIncrementStep;
    uint32 Reserved6;
    uint32 Reserved7;
};
// terminated by the byte 0x0D then 263 bytes of 0x00
// then the records start


class DBF
{
public:
    DBF();
    ~DBF();

    int open(string sFileName,bool bAllowWrite=false); // open an existing dbf file
    int close();

    int markAsDeleted(int nRecord); // mark this record as deleted
    int create(string sFileName,int nNumFields); // create a new dbf file with space for nNumFields
    int assignField(fieldDefinition myFieldDef,int nField); // used to assign the field info ONLY if num records in file = 0 !!!
    int addRecord(string sCSVRecord); // used to add records to the dbf file

    int getFieldIndex(string sFieldName);
    int loadRec(int nRecord); // load the record into memory
    string readField(int nField); // read the request field as a string always!


    int GetNumRecords()
    {
        return m_FileHeader.uRecordsInFile;
    }
    int GetNumFields()
    {
        return m_nNumFields;
    }
    string GetFieldName(int nField)
    {
        return string(m_FieldDefinitions[nField].cFieldName);
    }

    string convertInt(int number)
    {
       stringstream ss;//create a stringstream
       ss << number;//add number to the stream
       return ss.str();//return a string with the contents of the stream
    }

    string convertNumber(uint8 *n, int nSize)
    {
       // convert any size of number (represented by n[] ) into a string
       long long nResult = 0;
       for( int i = 0 ; i < nSize ; i++ )
       {
           nResult += (((unsigned long long) n[i]) << (i*8) );
       }
       stringstream ss; //create a stringstream
       ss << nResult; //add number to the stream
       return ss.str(); //return a string with the contents of the stream
    }

private:
    FILE * m_pFileHandle;
    string m_sFileName;

    bool m_bAllowWrite;
    fileHeader m_FileHeader;
    fieldDefinition m_FieldDefinitions[MAX_FIELDS]; // allow a max of 255 fields
    int m_nNumFields; // number of fields in use

    int updateFileHeader();

    char m_Record[8000]; // max of 8K per record (not sure if this is reasonable, good enough for my needs, no documentation available!)

};

#endif // DBF_H
