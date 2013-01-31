#include "dbf.h"

DBF::DBF()
{
    m_pFileHandle = NULL;
    m_nNumFields = 0;
    if( sizeof( fileHeader ) != 32 )
        std::cerr << "Structure is padded and will not work! " << sizeof( fileHeader ) << std::endl;
}

DBF::~DBF()
{
    if( m_pFileHandle != NULL )
        fclose(m_pFileHandle);
}

int DBF::open(string sFileName)
{
    // open a dbf file for reading only
    m_sFileName = sFileName;
    m_pFileHandle = fopen(sFileName.c_str(),"r");
    if( m_pFileHandle == NULL )
    {
       return errno;
    }

    // open is ok, so read in the File Header
    int nBytesRead = fread (&m_FileHeader,1,32,m_pFileHandle);
    if( nBytesRead != 32 )
    {
        std::cerr << "Bad read for Header, wanted 32, got " << nBytesRead << std::endl;
        return 1; // fail
    }

    std::cout << "Header: Type=" << m_FileHeader.u8FileType << std::endl
      << "  Last Update=" << (int) m_FileHeader.u8LastUpdateDay << "/" << (int) m_FileHeader.u8LastUpdateMonth << "/" << (int) m_FileHeader.u8LastUpdateYear << std::endl
      << "  Num Recs=" << m_FileHeader.uRecordsInFile << std::endl
      << "  Rec0 position=" << m_FileHeader.uPositionOfFirstRecord << std::endl
      << "  Rec length=" << m_FileHeader.uRecordLength << std::endl;

    m_nNumFields = 0;
    // now read in all the field definitions
    std::cout << "Fields: " << std::endl;
    do
    {
        int nBytesRead = fread(&(m_FieldDefinitions[m_nNumFields]),1,32,m_pFileHandle);
        if( nBytesRead != 32 )
        {
            std::cerr << "Bad read for Field, wanted 32, got " << nBytesRead << std::endl;
            return 1;
        }

        if( m_FieldDefinitions[m_nNumFields].cFieldName[0] == 0x0D || strlen(m_FieldDefinitions[m_nNumFields].cFieldName) <= 1 )
        {
            // end of fields
            break;
        }
        // show field in std out
        std::cout << "  " << m_FieldDefinitions[m_nNumFields].cFieldName << ", Type=" << m_FieldDefinitions[m_nNumFields].cFieldType
              << ", Disp=" << (int) m_FieldDefinitions[m_nNumFields].uFieldOffset << ", len=" << (int) m_FieldDefinitions[m_nNumFields].uLength
              << ", Dec=" << (int) m_FieldDefinitions[m_nNumFields].uNumberOfDecimalPlaces << ", Flag=" << (int) m_FieldDefinitions[m_nNumFields].FieldFlags << std::endl;

        m_nNumFields++;
    }while(!feof(m_pFileHandle));

    // move to start of first record
    int nFilePosForRec0 = 32+32*m_nNumFields+264;
    if( m_FileHeader.uPositionOfFirstRecord != nFilePosForRec0 )
    {
        // bad Rec0 position calc!!!  debug it!
        std::cerr << "Bad Rec 0 file position calculated " << nFilePosForRec0 << ", header says " << m_FileHeader.uPositionOfFirstRecord << std::endl;
        return 1;
    }

    return 0; // ok
}

int DBF::close()
{
    return fclose(m_pFileHandle);
    m_pFileHandle = NULL;
}

int DBF::getFieldIndex(string sFieldName)
{
    for( int i = 0 ; i < m_nNumFields ; i++ )
    {
        if( strncmp(m_FieldDefinitions[i].cFieldName,sFieldName.c_str(),10) == 0 )
            return i;
    }
    return -1; // not found
}

int DBF::loadRec(int nRecord)
{
    // read as a string always!  All modern languages can convert it later
    int nPos = m_FileHeader.uPositionOfFirstRecord + m_FileHeader.uRecordLength*nRecord;
    int nRes = fseek(m_pFileHandle,nPos,SEEK_SET);
    if (ferror (m_pFileHandle))
          std::cerr << "Error loading record " << nRecord << std::endl;
    if( nRes != 0)
        return 1; //fail

    int nBytesRead = fread(&m_Record[0],1,m_FileHeader.uRecordLength,m_pFileHandle);
    if( nBytesRead != m_FileHeader.uRecordLength )
    {
        std::cerr << "read(" << nRecord << ") failed, wanted " << m_FileHeader.uRecordLength << ", but got " << nBytesRead << " bytes";
        return 1; //fail
    }

    // record is now ready to be used
    return 0;
}

string DBF::readField(int nField)
{
    // read the field from the record, and output as a string because all modern languages can use a string

    // depending on the field type, get the field and convert to a string  ( do not have documentation on the types, so this is all guesswork)
    char cType = m_FieldDefinitions[nField].cFieldType;
    int nOffset = m_FieldDefinitions[nField].uFieldOffset;
    int nMaxSize = m_FieldDefinitions[nField].uLength;

    if( cType == 'C' || cType == 'M' || cType == 'G' || cType == 'N' )
    {
        char dest[255];
        strncpy(&dest[0],&m_Record[nOffset],nMaxSize);
        dest[nMaxSize]=0; // make sure string is terminated!
        // trim end of string (std c++ is so lame!)
        for( int i=strlen(dest)-1 ; i >= 0 ; i-- )
        {
            if( dest[i] == ' ' )
                dest[i] = 0;
            else
                break;
        }

        return string(dest);
    }
    else if( cType == 'I' )
    {
        // convert integer numbers up to 16 bytes long
        uint8 n[16];
        for( int i = 0 ; i < nMaxSize ; i++ )
            n[i] = (uint8 ) m_Record[nOffset+i];

        return convertNumber(&n[0],nMaxSize);
    }
    else if( cType== 'F' || cType == 'B' || cType == 'O' )
    {
        // float
        if( nMaxSize == 4)
        {
            // float
            union name1
            {
                uint8   n[4];
                float     f;
            } uvar;
            uvar.f = 0;

            uvar.n[0] = (uint8 ) m_Record[nOffset];
            uvar.n[1] = (uint8 ) m_Record[nOffset+1];
            uvar.n[2] = (uint8 ) m_Record[nOffset+2];
            uvar.n[3] = (uint8 ) m_Record[nOffset+3];

            stringstream ss;
            ss << uvar.f;
            return ss.str();
        } else if( nMaxSize == 8)
        {
            // double
            union name1
            {
                uint8   n[8];
                double     d;
            } uvar;
            uvar.d = 0;

            uvar.n[0] = (uint8 ) m_Record[nOffset];
            uvar.n[1] = (uint8 ) m_Record[nOffset+1];
            uvar.n[2] = (uint8 ) m_Record[nOffset+2];
            uvar.n[3] = (uint8 ) m_Record[nOffset+3];
            uvar.n[4] = (uint8 ) m_Record[nOffset+4];
            uvar.n[5] = (uint8 ) m_Record[nOffset+5];
            uvar.n[6] = (uint8 ) m_Record[nOffset+6];
            uvar.n[7] = (uint8 ) m_Record[nOffset+7];

            stringstream ss;
            ss << uvar.d;
            return ss.str();
        }
    }
    else if( cType == 'L' )
    {
        // Logical ,T = true, ?=NULL, F=False
        if( strncmp(&(m_Record[nOffset]),"T",1) == 0 )
            return "T";
        else if( strncmp(&(m_Record[nOffset]),"?",1) == 0 )
            return "?";
        else
            return "F";
    }

    return "FAIL CASE"; // missing a case for this DB
}
