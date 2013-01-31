#ifndef DBF_H
#define DBF_H

#include <string.h>
#include <stdio.h>
#include <iostream>
#include <sstream>
#include <errno.h>
#include <math.h>
#include <ctime>

using namespace std;

typedef unsigned char uint8;
typedef short int uint16;
typedef int uint32;

#define MAX_FIELDS 255
#define DBF_DELETED_RECORD_FLAG 'D'   // NOT SURE IF THIS IS CORRECT, MUST TEST!!!!
#define MAX_RECORD_SIZE 0xffff*50    // not idea if this is correct, but good enough for my needs

struct fileHeader
{
    uint8 u8FileType;
    uint8 u8LastUpdateYear;
    uint8 u8LastUpdateMonth;
    uint8 u8LastUpdateDay;
    uint32 uRecordsInFile;
    uint16 uPositionOfFirstRecord;
    uint16 uRecordLength; // includes the delete flag (byte) at start of record
    uint8 Reserved16[4*4]; // 16 bytes reserved, must always be set to zeros
    uint8 uTableFlags;
    uint8 uCodePage;
    uint8 Reserved2[2]; // 2 bytes reserved, must put zeros in all reserved fields
};

// after the file header, we can have n field definition records, terminated by the byte 0x0D
// must build as a union to make sure bytes are aligned correctly
struct fieldDefinition
{
    char cFieldName[11];
    char cFieldType;
    uint32 uFieldOffset; // location of field from start of record
    uint8 uLength; // Length of Field in bytes
    uint8 uNumberOfDecimalPlaces;
    uint8 FieldFlags;
    uint8 uNextAutoIncrementValue[4]; // 32 bit int
    uint8 uAutoIncrementStep;
    uint8 Reserved8[8]; // should always be zero
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
    int appendRecord(string *sValues, int nNumValues); // used to append records to the end of the dbf file

    int getFieldIndex(string sFieldName);
    int loadRec(int nRecord); // load the record into memory
    bool isRecordDeleted(); // check if loaded record is deleted
    string readField(int nField); // read the request field as a string always fromr the loaded record!

    void dumpAsCSV(); // output fields and records as csv to std output

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

    int ConvertStringToInt(string sInteger,int nSize, char *cRecord)
    {
        // convert the given string into an integer of nSize bytes (2 or 4 or 8 only)
        if( nSize == 2 )
        {
            union {
                short int i;
                uint8 n[4];
            } u;
            stringstream ss;
            ss << sInteger;
            ss >> u.i;

            for( int i = 0 ; i < nSize ; i++ )
                cRecord[i] = u.n[i];

            return 0;
        }
        else if( nSize == 4 )
        {
            union {
                int i;
                uint8 n[4];
            } u;
            stringstream ss;
            ss << sInteger;
            ss >> u.i;

            for( int i = 0 ; i < nSize ; i++ )
                cRecord[i] = u.n[i];

            return 0;
        }
        else if( nSize ==8 )
        {
            union {
                long i;
                uint8 n[8];
            } u;
            stringstream ss;
            ss << sInteger;
            ss >> u.i;

            for( int i = 0 ; i < nSize ; i++ )
                cRecord[i] = u.n[i];

            return 0;
        }

        // fail, clear the record
        for( int i = 0 ; i < nSize ; i++ )
            cRecord[i] = 0;
        return 1; // fail
    }

    int ConvertStringToFloat(string sFloat,int nSize, char *cRecord)
    {
        // convert the given string into a float or a double
        if( nSize == 4 )
        {
            union {
                float f;
                uint8 n[4];
            } u;
            stringstream ss;
            ss << sFloat;
            ss >> u.f;

            for( int i = 0 ; i < nSize ; i++ )
                cRecord[i] = u.n[i];

            return 0;
        } else if( nSize == 8 )
        {
            union {
                double d;
                uint8 n[8];
            } u;
            stringstream ss;
            ss << sFloat;
            ss >> u.d;

            for( int i = 0 ; i < nSize ; i++ )
                cRecord[i] = u.n[i];
            return 0;
        }

        // fail, clear the record
        for( int i = 0 ; i < nSize ; i++ )
            cRecord[i] = 0;
        return 1; // fail
    }

private:
    FILE * m_pFileHandle;
    string m_sFileName;

    bool m_bStructSizesOK; // this must be true for engine to work!
    bool m_bAllowWrite;
    fileHeader m_FileHeader;
    fieldDefinition m_FieldDefinitions[MAX_FIELDS]; // allow a max of 255 fields
    int m_nNumFields; // number of fields in use

    int updateFileHeader();

    char *m_pRecord;

};

#endif // DBF_H
