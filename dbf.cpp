#include "dbf.h"

DBF::DBF()
{
    m_pFileHandle = NULL;
    m_nNumFields = 0;
    m_bAllowWrite = false;
    if( sizeof( fileHeader ) != 32 )
        std::cerr << __FUNCTION__ << " Structure is padded and will not work! " << sizeof( fileHeader ) << std::endl;
}

DBF::~DBF()
{
    if( m_pFileHandle != NULL )
        fclose(m_pFileHandle);
}

int DBF::open(string sFileName,bool bAllowWrite)
{
    // open a dbf file for reading only
    m_sFileName = sFileName;
    m_bAllowWrite = bAllowWrite;

    char cMode[10] = "r";
    if( m_bAllowWrite )
        strncpy(cMode,"r+",3); // change to read write mode

    m_pFileHandle = fopen(sFileName.c_str(),cMode);
    if( m_pFileHandle == NULL )
    {
        std::cerr << __FUNCTION__ << " Unable to open file " << sFileName << std::endl;
       return errno;
    }

    // open is ok, so read in the File Header
    int nBytesRead = fread (&m_FileHeader,1,32,m_pFileHandle);
    if( nBytesRead != 32 )
    {
        std::cerr << __FUNCTION__ << " Bad read for Header, wanted 32, got " << nBytesRead << std::endl;
        return 1; // fail
    }

    std::cout << "Header: Type=" << m_FileHeader.u8FileType << std::endl
      << "  Last Update=" << (int) m_FileHeader.u8LastUpdateDay << "/" << (int) m_FileHeader.u8LastUpdateMonth << "/" << (int) m_FileHeader.u8LastUpdateYear << std::endl
      << "  Num Recs=" << m_FileHeader.uRecordsInFile << std::endl
      << "  Rec0 position=" << m_FileHeader.uPositionOfFirstRecord << std::endl
      << "  Rec length=" << m_FileHeader.uRecordLength << std::endl
      << "  CodePage=" << (int) m_FileHeader.uCodePage << std::endl
      << "  TableFlags=" << (int) m_FileHeader.uTableFlags << std::endl;

    m_nNumFields = 0;
    // now read in all the field definitions
    std::cout << "Fields: " << std::endl;
    do
    {
        int nBytesRead = fread(&(m_FieldDefinitions[m_nNumFields]),1,32,m_pFileHandle);
        if( nBytesRead != 32 )
        {
            std::cerr << __FUNCTION__ << " Bad read for Field, wanted 32, got " << nBytesRead << std::endl;
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
        std::cerr << __FUNCTION__ << " Bad Rec 0 file position calculated " << nFilePosForRec0 << ", header says " << m_FileHeader.uPositionOfFirstRecord << std::endl;
        return 1;
    }

    return 0; // ok
}

int DBF::close()
{
    return fclose(m_pFileHandle);
    m_pFileHandle = NULL;
    m_sFileName = "";
    m_nNumFields = 0;
    m_bAllowWrite = false;
    m_FileHeader.u8FileType = 0;

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
          std::cerr << __FUNCTION__ << " Error seeking to record " << nRecord << std::endl;

    if( nRes != 0)
    {
        for( unsigned int i=0;i<sizeof(m_Record);i++)
            m_Record[i]=0; // clear record to indicate it is invalid
        return 1; //fail
    }
    int nBytesRead = fread(&m_Record[0],1,m_FileHeader.uRecordLength,m_pFileHandle);
    if( nBytesRead != m_FileHeader.uRecordLength )
    {
        std::cerr << __FUNCTION__ << " read(" << nRecord << ") failed, wanted " << m_FileHeader.uRecordLength << ", but got " << nBytesRead << " bytes";
        for( unsigned int i=0;i<sizeof(m_Record);i++)
            m_Record[i]=0; // clear record to indicate it is invalid
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

    /* possible types
      C=char
      Y=Currency?
      N=Numeric (really just a char)
      F=Float
      D=Date (? format unknown)
      T=Date Time (? format unknown)
      B=Double
      I=Integer
      L=Logical (char[1] with T=true,?=Null,F=False)
      M=memo (big char field?)
      G=General (?)
      P=Picture (?)
      ... others?  really no idea
      treat all unhandled field types as C for now
    */

    if( cType == 'I' )
    {
        // convert integer numbers up to 16 bytes long into a string
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
    } else
    {
        // Character type fields (default)
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
    return "FAIL";
}


int DBF::create(string sFileName,int nNumFields)
{
    // create a new dbf file with space for nNumFields
    if( m_pFileHandle != NULL )
    {
        close();
    }
    m_sFileName = sFileName;
    m_bAllowWrite = true;
    m_nNumFields = nNumFields;

    m_pFileHandle = fopen(sFileName.c_str(),"wb+"); // create a new empty file for binary writing
    if( m_pFileHandle == NULL )
    {
       std::cerr << __FUNCTION__ << " Unable to create file " << sFileName << std::endl;
       return errno;
    }
    // create is ok

    // setup the file header
    m_FileHeader.u8FileType = 0x30;
    m_FileHeader.Reserved1 = 0;
    m_FileHeader.Reserved2 = 0;
    m_FileHeader.Reserved3 = 0;
    m_FileHeader.Reserved4 = 0;
    m_FileHeader.Reserved5 = 0;
    m_FileHeader.u8LastUpdateDay = 0;
    m_FileHeader.u8LastUpdateMonth = 0;
    m_FileHeader.u8LastUpdateYear = 13;
    m_FileHeader.uCodePage = 3; // copied from another db, no idea what this corresponds to
    m_FileHeader.uPositionOfFirstRecord = 32+32*nNumFields+264; // calculated based on the file header size plus the n*FieldDef size + 1 term char + 263 zeros
    m_FileHeader.uRecordLength = 0;
    m_FileHeader.uRecordsInFile = 0;
    m_FileHeader.uTableFlags = 0; // bit fields, copied from another db , 0x01=has a .cdx?, 0x02=Has Memo Fields, 0x04=is a .dbc?

    // write in the File Header for the first time!
    fwrite(&m_FileHeader,1,sizeof(m_FileHeader),m_pFileHandle);

    // now write dummy field definition records until the real ones can be specified
    for( int i = 0; i < nNumFields ; i++ )
    {
        for( int j = 0; j < 11 ; j++ )
            m_FieldDefinitions[i].cFieldName[j]=0; // clear entire name
        m_FieldDefinitions[i].cFieldType='C';
        m_FieldDefinitions[i].FieldFlags=0;
        m_FieldDefinitions[i].uAutoIncrementStep=0;
        m_FieldDefinitions[i].uNextAutoIncrementValue=0;
        m_FieldDefinitions[i].uLength=0;
        m_FieldDefinitions[i].uNumberOfDecimalPlaces=0;
        m_FieldDefinitions[i].Reserved6=0;
        m_FieldDefinitions[i].Reserved7=0;
        // write the definitions
        fwrite(&m_FieldDefinitions[i],1,sizeof(m_FieldDefinitions),m_pFileHandle);
    }
    // write the field definition termination character
    char FieldDefTermination = 0x0D;
    fwrite(&FieldDefTermination,1,1,m_pFileHandle);
    // write the 263 bytes of 0
    char cZero[263];
    for( int j=0; j<263;j++)
        cZero[j]=0;

    fwrite(&cZero[0],1,263,m_pFileHandle);
    // this is now the starting point for the first record

    // ready to assign the field definitions!

    return 0;
}

int DBF::updateFileHeader()
{
    // move to file start
    int nRes = fseek(m_pFileHandle,0,SEEK_SET);
    if( nRes != 0)
        return 1; //fail

    // write the current header info
    int nBytesWritten = fwrite(&m_FileHeader,1,sizeof(m_FileHeader),m_pFileHandle);
    if( nBytesWritten != sizeof(m_FileHeader) )
    {
        // error!
        std::cerr << __FUNCTION__ << " Failed to update header!" << std::endl;
        return 1;
    }
    return 0;
}

int DBF::assignField(fieldDefinition myFieldDef,int nField)
{
    // used to assign the field info ONLY if num records in file = 0 !!!
    if( m_FileHeader.uRecordsInFile != 0)
    {
        std::cerr << __FUNCTION__ << " Failed to AssignField Can not change Fields once the File has records in it!" << std::endl;
        return 1; // fail
    }


    int nPosOfFieldDef = 32+nField*32;
    int nRes = fseek(m_pFileHandle,nPosOfFieldDef,SEEK_SET);
    if( nRes != 0)
        return 1; //fail
    int nBytesWritten = fwrite(&myFieldDef,1,sizeof(fieldDefinition),m_pFileHandle);
    if( nBytesWritten != sizeof(m_FileHeader) )
    {
        // error!
        std::cerr << __FUNCTION__ << " Failed to update Field Definition!" << std::endl;
        return 1;
    }
    // update the in memory definition too
    m_FieldDefinitions[nField] = myFieldDef;

    // update the total record length, and the header record!
    m_FileHeader.uRecordLength = 0;
    for( int i=0;i< m_nNumFields ;i++ )
        m_FileHeader.uRecordLength += m_FieldDefinitions[i].uLength;
    updateFileHeader();

    return 0;
}

int DBF::addRecord(string sCSVRecord)
{
    // used to add records to the dbf file (append to end of file only)
    int nRes = fseek(m_pFileHandle,0,SEEK_END);
    if (ferror (m_pFileHandle))
          std::cerr << __FUNCTION__ << " Error seeking to end of file " << std::endl;
    if( nRes != 0)
        return 1; //fail


// NOT DONE

    // convert each csv value into the proper data type,
    // write the record to the end of the file















    // update the header to reflect the record count
    m_FileHeader.uRecordsInFile++;
    updateFileHeader();
    return 0;
}

int DBF::markAsDeleted(int nRecord)
{
    // mark this record as deleted
    int nPos = m_FileHeader.uPositionOfFirstRecord + m_FileHeader.uRecordLength*nRecord;
    int nRes = fseek(m_pFileHandle,nPos,SEEK_SET);
    if (ferror (m_pFileHandle))
          std::cerr << __FUNCTION__ << " Error loading record " << nRecord << std::endl;
    if( nRes != 0)
        return 1; //fail

    char Rec[2];

    int nBytesRead = fread(&Rec[0],1,1,m_pFileHandle);
    if( nBytesRead != m_FileHeader.uRecordLength )
    {
        std::cerr << __FUNCTION__ << "read(" << nRecord << ") failed, wanted " << m_FileHeader.uRecordLength << ", but got " << nBytesRead << " bytes";
        return 1; //fail
    }

    if( Rec[0] == ' ' )
    {
        // ok to delete, not marked as deleted yet
        Rec[0] = '*'; // I hope this is the delete character, NOT SURE!!!
        fwrite(&Rec[0],1,1,m_pFileHandle);
        m_FileHeader.uRecordsInFile--;
        updateFileHeader();
    }

    // done
    return 0;
}
