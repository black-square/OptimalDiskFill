/*
   ��������� ������ ������ ������������ (����������� ������������ �����) ���������� �������� ������� �� ��������� �����.
   ���������� ��� ������ �� ���������� ����������� ��������, ��� ������ � �������, 
   ������������� ���: ����� ������� ������ �������� ����������������; ����� ������� ��������� ��������� 
   ��������� ���������� ���� � ��������� ��������� (��������); ��������� ��������� ������ ���, ����� �� ���������� 
   � ����� ���������� ����������� ��������� ���� �� ������������.
*/

#include <vector>
#include <map>
#include <iostream>
#include "../nwlibs/FirstHeader.h"
#include "../nwlibs/FileSystem.h"
#include "../nwlibs/strings.hpp"
#include "../nwlibs/TestHelper.h" 
#include <shlwapi.h>
#include "../nwlibs/CoutConvert.h"
#include <limits>

typedef std::basic_string<TCHAR> TString;
typedef LONGLONG TFileSize;


//���������� ������ m_Size � ���������� ������� ���������� b|kb|mb|gb
class TFormat
{
   const TFileSize &m_Size;

private:
   static const char * const m_rgDims[];
   static const TFileSize m_DimMultiple;

public:
   TFormat(const TFileSize &Size ): m_Size(Size) {}
    
   void Print( std::ostream &os ) const;

   friend std::ostream &operator<<( std::ostream &os, const TFormat &F )
   {
      F.Print(os);
      return os;
   }

   //��������� �������������� ����� � ������� 128MB � �����
   static TFileSize ConvertDims( const TString &S );
};
/////////////////////////////////////////////////////////////////////////////////
const char * const TFormat::m_rgDims[] = { "B", "KB", "MB", "GB", "TB" };
const TFileSize TFormat::m_DimMultiple = 1024;
/////////////////////////////////////////////////////////////////////////////////
void TFormat::Print( std::ostream &os ) const
{
   const double Multiple = static_cast<double>(m_DimMultiple);

   double CurValue = static_cast<double>( m_Size );
   size_t Index = 0;

   if( CurValue < Multiple )
   {
      os << m_Size << " " << m_rgDims[0];    
   }
   else
   {   
      do
      {
         CurValue /= Multiple;
         ++Index;
      }
      while( Index < APL_ARRSIZE(m_rgDims) && CurValue > Multiple );

      os.precision(2);
      os << std::fixed;

      os << CurValue << " " << m_rgDims[Index] << " (" <<  m_Size << " " << m_rgDims[0] << ")"; 
   }
}
///////////////////////////////////////////////////////////////////////////////

TFileSize TFormat::ConvertDims( const TString &S )
{
   TFileSize RetVal;

   TString::const_iterator I = NWLib::ConvertStringToInteger( S.begin(), S.end(), RetVal );

   if( I == S.begin() )
      APL_THROW( _T("������ ��� �������������� ��������� '") << S << _T("' � �����") );

   while( I != S.end() && *I == _T(' ') ) 
      ++I;

   if( I == S.end() )
      return RetVal;

   TString Dim( I, S.end() );
   NWLib::ToLower(Dim, Dim);

   TString Tmp;

   for( int i = 0; i < APL_ARRSIZE(m_rgDims); ++i, RetVal *= m_DimMultiple )
   {
      Tmp = NWLib::ConvertToTStr( m_rgDims[i] );   
      NWLib::ToLower(Tmp, Tmp);

      if( Dim == Tmp )
         return RetVal;
   }
   
   APL_THROW( _T("������ ��� ����������� ������ ��������� ��������� '") << S << _T("'") );

   return 0;
}
///////////////////////////////////////////////////////////////////////////////

//���������� �� ��������� ���������� ��� �����
struct TItem
{
   TString Name;     //��� �������� ��� �����
   TFileSize Size;   //������ � ������

   TItem(): Size(0) {} 
};

typedef std::vector<TItem> TItems; 

//�������� �������������� ������ �� �������
struct SizeCmp: std::binary_function<TItem, TItem, bool>
{
   bool operator()( const TItem &a1, const TItem &a2 ) const
   {
      return a1.Size > a2.Size;
   }
};

//������� �������
typedef std::vector<TItems::const_iterator> TSolve;

//���������� ������� ����
typedef std::map<TString, TString> TPathReplaces;

/////////////////////////////////////////////////////////////////////////////////

//�������� ������ ����� �� ��������� FD
inline TFileSize GetSize( const WIN32_FIND_DATA &FD )
{
   TFileSize RetVal = FD.nFileSizeHigh;
   
   RetVal *= TFileSize(MAXDWORD) + 1;
   RetVal += FD.nFileSizeLow;
   return RetVal;
}
/////////////////////////////////////////////////////////////////////////////////

//��������� ������ �������� � ������
TFileSize CalcDirSize( const TString &Dir )
{
   TFileSize RetVal = 0;

   NWLib::CDirectoryFileList DFL(Dir);

   while( DFL.Next() )
      RetVal += GetSize( DFL.GetFindData() );

   return RetVal;
}

//������� � �������� Dir ��� ����� � ��������, �������� ��������� � ��������� Items
void LoadItems( const TString &Dir, TItems &Items, const TString &FilePrefix = _T(""), TFileSize maxDirSize = std::numeric_limits<TFileSize>::max() )
{
   TItem CurItem;

   WIN32_FIND_DATA FindData;
   HANDLE hFind = FindFirstFile( (Dir + _T("\\*")).c_str(), &FindData );

   if (INVALID_HANDLE_VALUE == hFind) 
       return;
   do
   {
      if( _tcscmp(FindData.cFileName, _T("..")) == 0 || 
         _tcscmp(FindData.cFileName, _T(".")) == 0
      )
         continue;
      
      CurItem.Name = FilePrefix + FindData.cFileName;
      
      if( FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
      {
         const TString curDirName = Dir + _T("\\") + FindData.cFileName;
      
         CurItem.Size = CalcDirSize( curDirName );  
         
         if( CurItem.Size > maxDirSize )
         {
            LoadItems( curDirName, Items, FilePrefix + FindData.cFileName + _T("\\"), maxDirSize );
            continue;    
         } 
      } 
      else
         CurItem.Size = GetSize( FindData );

      Items.push_back(CurItem);
   }
   while (FindNextFile(hFind, &FindData) != 0);

   FindClose(hFind);
   return;
}
/////////////////////////////////////////////////////////////////////////////////

//�������� ������� ����������� ����������
template<class PrintStrategyT>
void OptimalFillImpl( TItems::const_iterator Begin, TItems::const_iterator End, TSolve &CurSolve, TFileSize FreeSpace, TFileSize &BestFreeSpace, bool &StopFlag, const PrintStrategyT &printStrategy)
{  
   //�� ���� ��������� ���� ���������� ������:
   //"������ �� ���������� ����������� ��������" 
   //http://www.wikiznanie.ru/ru-wz/index.php/������_��_����������_�����������_��������
   //��� �������� ���������� �� ���� ������� ������ � ������ (� �� �������� 
   //���������� ������� �� ������ ���� ��� ��� �������� � �������� ����������� �� ��� � ���� ���� � ������).
   //������ � ������� �������� ����������������, � � ������ ������ �������� �������� ������ ���.
   //�������� ���������� ��� ��������, ��� ��� ������� ������ ��������� ����, ������� � ������� � ��� ����
   //������������ ��� ��������� ���������� ������ ������, ����� ����, ���� ����� ���������.
   //����� ��������� ����������� ����������.

   TFileSize CurFreeSpace;

   while( Begin != End && !StopFlag )
   {
      //��� ���� ����� ���� �������� ���������� ������� �� ��������� ������� ������
      CurSolve.push_back( Begin ); 

      if( Begin->Size <= FreeSpace ) //���� ���� ���������� � ��������
      {
         //�������� ���������� ����� ����� ��������� �����
         CurFreeSpace = FreeSpace - Begin->Size;
         
         if( CurFreeSpace < BestFreeSpace )
         {
            //����� ����� �������
            BestFreeSpace = CurFreeSpace;
            printStrategy(BestFreeSpace, CurSolve);

            if( BestFreeSpace == 0 )
               StopFlag = true;
         }

         //���� �� ����� �� ������� �������� �� ���������� [������; �������]
         //��������� ���� ��������� �� ���������� �� ���������� �����
         ++Begin;

         OptimalFillImpl( Begin, End, CurSolve, CurFreeSpace, BestFreeSpace, StopFlag, printStrategy );
      }
      else
         ++Begin;

      CurSolve.pop_back();
   }
}
/////////////////////////////////////////////////////////////////////////////////

//��������� ����������� ���������� ����������
template<class PrintStrategyT>
void OptimalFill( TItems &Items, TFileSize FreeSpace, const PrintStrategyT &printStrategy )
{
   //�� ��������� ����� ��� ���� ����� ��� ������ ������� ������� ����������� ����� 
   //������� �����, � ������ �������� �������������� ��������������� ���������� �������
   //��� ��������� ������� �������� �������
   std::sort( Items.begin(), Items.end(), SizeCmp() );

   TSolve Solve;
   bool StopFlag = false;

   OptimalFillImpl( Items.begin(), Items.end(), Solve, FreeSpace, FreeSpace, StopFlag, printStrategy );
}
/////////////////////////////////////////////////////////////////////////////////

void PrintFileSizes( const TItems &Items )
{
   std::ofstream flOut( (NWLib::ConvertToTStr(NWLib::GetExeDirPath()) + _T("sizes.txt")).c_str() );

   for( TItems::const_iterator I = Items.begin(); I != Items.end(); ++I )
      flOut << I->Name << "\t" << TFormat(I->Size) << std::endl;
}
/////////////////////////////////////////////////////////////////////////////////

//���������� �����/�������� ������� �� ���������� � �������� ������
void PrintNotFitToDestDirs( const TItems &Items, TFileSize DestSpace )
{
   bool First = true;
   
   for( TItems::const_iterator I = Items.begin(); I != Items.end(); ++I )
   {
      if( I->Size > DestSpace )
      {
         if( First )
         {
            First = false;

            std::cout << "\n" << APL_LINE;
            std::cout << "���������� �����/�������� ������� �� ���������� � �������� �����:" << std::endl;
         }
         
         std::cout << I->Name << "\t" << TFormat(I->Size - DestSpace) << std::endl;
      }
   }

   if( !First )
      std::cout << APL_LINE;
}
/////////////////////////////////////////////////////////////////////////////////

//�������� ������� �������
void PrintSolve( TFileSize FreeSpace, const TSolve &Solve )
{
  std::cout << "\n----- ������� ������� ������� -----" << std::endl;
  std::cout << "�������� �� �������������: " << TFormat(FreeSpace) << std::endl;

  TFileSize TotalSize = 0;
  for( TSolve::const_iterator I = Solve.begin(); I != Solve.end(); ++I )
  {
    std::cout << (*I)->Name << std::endl;
    TotalSize += (*I)->Size;
  }

  std::cout << "�����: " << TFormat(TotalSize) << std::endl;
}
/////////////////////////////////////////////////////////////////////////////////

class CoutRedirect
{
public:
  CoutRedirect( const std::ofstream &os )
  {
    std::cout.flush();
    m_pPrevBuf = std::cout.rdbuf( os.rdbuf() );  
  }
  
  ~CoutRedirect()
  {
    std::cout.rdbuf( m_pPrevBuf );  
  }

private:
  std::streambuf *m_pPrevBuf;  
};
/////////////////////////////////////////////////////////////////////////////////

void MakeBatFile( TFileSize FreeSpace, const TSolve &Solve, const TPathReplaces &PathReplaces )
{
  std::ofstream flOut( "Solution.bat" );
  
  //�������������� ����� std::cout � flOut ��� ��������� �������������� 
  //����������� � OEM866
  CoutRedirect CoutRedirect(flOut);

  std::cout << "@echo �������� �� �������������: " << TFormat(FreeSpace) << std::endl;

  for( TSolve::const_iterator it = Solve.begin(); it != Solve.end(); ++it )
  {
    const TString::size_type prefixEnd = (*it)->Name.find( _T('\\') );
    
    if( prefixEnd == TString::npos )
      APL_THROW( _T("�������� ������ ����� �����: ") << (*it)->Name );
      
    const TString prefix( (*it)->Name.begin(), (*it)->Name.begin() + prefixEnd + 1 );
    const TString pathOnly( (*it)->Name.begin() + prefixEnd + 1, (*it)->Name.end() );
       
    const TPathReplaces::const_iterator originDirIter = PathReplaces.find( prefix );
    
    if( originDirIter == PathReplaces.end() )
      APL_THROW( _T("����������� �������: ") << prefix << _T(", ") << (*it)->Name );
      
    std::cout << _T("@call ProcessSolution.bat \"") << originDirIter->second << _T("\" \"") << pathOnly << _T("\"") << std::endl;  
  }
}

/////////////////////////////////////////////////////////////////////////////////

struct PringOnScreenStrat
{
  void operator()( TFileSize FreeSpace, const TSolve &Solve ) const
  {
    PrintSolve( FreeSpace, Solve );
  }
};
/////////////////////////////////////////////////////////////////////////////////

struct PringOnScreenAndFile
{
  explicit PringOnScreenAndFile( const TPathReplaces &PathReplaces ):
    PathReplaces(PathReplaces)
  {} 


  void operator()( TFileSize FreeSpace, const TSolve &Solve ) const
  {
    PrintSolve( FreeSpace, Solve );
    MakeBatFile( FreeSpace, Solve, PathReplaces );
  }
  
private:
  const TPathReplaces &PathReplaces;   
};
/////////////////////////////////////////////////////////////////////////////////

const wchar_t * const szDescription = 
   L"OptimalDiskFill v1.10.0\n" 
   L"������� ��������� (c) 2013\n"
   L"��������� �������� ���������� ��������� ������� CD, DVD ������ ��� ������\n"
   L"��������� ��� ������ ������ �� ���. ��� ��������� ������� � ���������\n" 
   L"������ ��� ������ � ��������� ���������, ����� ����� ������� �����\n" 
   L"���������� ������ � ���������, ��� ������� ��� ������ ������������ ������,\n" 
   L"�� �� ������ ���������.\n"
   L"\n"
   L"�������������:\n"
   L"      OptimalDiskFill [--look_into_oversized_dirs] [--make_bat_file] \n" 
   L"                      \"<������� 1>\" \"[������� 2]\" \"[������� N]\"\n"
   L"                      \"<�������� �����>[Tb|Gb|Mb|Kb|b]\"\n"
   L"\n"
   L"      look_into_oversized_dirs - ����� ������ ��������, ���� ��� ������\n" 
   L"                                 ��������� <�������� �����>\n"
   L"\n"
   L"����������:\n"
   L"      ��������� � ����� ���� ������ ������ ����������� ���������� ��������\n" 
   L"      ��������������� �������, ��������� ������� ������� ������� �� ����\n" 
   L"      �� ���������� � ����� �������� ����� �����. ���� ��� ��� ����������\n" 
   L"      ������� �������, ���������� ��� � �������� ���������,\n" 
   L"      ����� CTRL+C ��� CTRL+BREAK.\n";
/////////////////////////////////////////////////////////////////////////////////

int _tmain(int argc, _TCHAR* argv[])
{
   NWLib::TConsoleAutoStop CAS;

   APL_TRY()
   {
      typedef std::vector<TString> TDirs;
      TDirs Dirs;
      TFileSize FreeSpace;

      std::cout << szDescription << std::endl;

#if defined(NDEBUG) || 1
      if( argc < 3 )
         APL_THROW( _T("������������ ���������� ����������. ��. ������������") );

      bool lookIntoOversizedDirs = false;
      bool makeBatFile = false;

      for( int curArg = 1; curArg < argc - 1; ++curArg )
      {
          if( _tcscmp(argv[curArg], _T("--look_into_oversized_dirs")) == 0 )
          { 
            lookIntoOversizedDirs = true;
            continue;
          }
          
          if( _tcscmp(argv[curArg], _T("--make_bat_file")) == 0 )
          { 
            makeBatFile = true;
            continue;
          }
      
          Dirs.push_back(argv[curArg]);

          if( PathIsDirectory(Dirs.back().c_str()) == FALSE )
              APL_THROW( _T("������� '") << Dirs.back().c_str() << _T("' �� ������") );
      }

      FreeSpace = TFormat::ConvertDims( argv[argc - 1] );
 
      if( FreeSpace <= 0 )
         APL_THROW( _T("�������� ������ ������ ���� ������ ����") );
#else
      //Dirs.push_back(_T("c:\\_games\\_PSP\\_nead_burn") );
      //Dirs.push_back(_T("c:\\!My_docs\\_MOVIES\\_new"));
      Dirs.push_back(_T("c:\\!My_docs\\_PHOTOS\\!NOT_BURN!"));
      FreeSpace = TFileSize(4483) * 1024 * 1024;
      bool lookIntoOversizedDirs = true;
#endif
      TItems Items;
      TPathReplaces PathReplaces;
      const TFileSize maxDirSize = lookIntoOversizedDirs ? FreeSpace : std::numeric_limits<TFileSize>::max();
      
      std::cout << "�������� �����:  " << TFormat(FreeSpace) << std::endl;

      for( TDirs::const_iterator it = Dirs.begin(); it != Dirs.end(); ++it )
      {
          std::basic_stringstream<TCHAR> FilePrefix;

          if( Dirs.size() > 1 )
            FilePrefix << it - Dirs.begin() + 1 << _T("\\");
          else
            FilePrefix << _T("\\");

          std::cout << "������ �������� " << FilePrefix.str() << " = " << *it << " ..." << std::endl;
          LoadItems( *it, Items, FilePrefix.str(), maxDirSize ); 
          
          PathReplaces.insert( TPathReplaces::value_type(FilePrefix.str(), *it) );
      }
          
      //PrintFileSizes( Items );
      PrintNotFitToDestDirs( Items, FreeSpace );

      std::cout << "\n�������� ����� �������..." << std::endl;
      
      if( !makeBatFile )
        OptimalFill( Items, FreeSpace, PringOnScreenStrat() );
      else
        OptimalFill( Items, FreeSpace, PringOnScreenAndFile(PathReplaces) );

      std::cout << "\n����� ��������. ��������� ������� �����������." << std::endl;
   }
   APL_CATCH()

   return 0;
}