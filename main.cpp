/*
   ��������� ������ ������ ������������ (����������� ������������ �����) ���������� �������� ������� �� ��������� �����.
   ���������� ��� ������ �� ���������� ����������� ��������, ��� ������ � �������, 
   ������������� ���: ����� ������� ������ �������� ����������������; ����� ������� ��������� ��������� 
   ��������� ���������� ���� � ��������� ��������� (��������); ��������� ��������� ������ ���, ����� �� ���������� 
   � ����� ���������� ����������� ��������� ���� �� ������������.
*/

#include <vector>
#include <iostream>
#include "../libs/FirstHeader.h"
#include "../libs/FileSystem.h"
#include "../libs/strings.hpp"
#include "../libs/TestHelper.h" 
#include <shlwapi.h>
#include "../libs/CoutConvert.h"

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
void LoadItems( const TString &Dir, TItems &Items, const TString &FilePrefix = _T("") )
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
         CurItem.Size = CalcDirSize( Dir + _T("\\") + FindData.cFileName );  
      else
         CurItem.Size = GetSize( FindData );

      Items.push_back(CurItem);
   }
   while (FindNextFile(hFind, &FindData) != 0);

   FindClose(hFind);
   return;
}
/////////////////////////////////////////////////////////////////////////////////

//�������� ������� �������
void PrintSolve( TFileSize FreeSpace, const TSolve &Solve)
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

//�������� ������� ����������� ����������
void OptimalFillImpl( TItems::const_iterator Begin, TItems::const_iterator End, TSolve &CurSolve, TFileSize FreeSpace, TFileSize &BestFreeSpace, bool &StopFlag )
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
            PrintSolve(BestFreeSpace, CurSolve);

            if( BestFreeSpace == 0 )
               StopFlag = true;
         }

         //���� �� ����� �� ������� �������� �� ���������� [������; �������]
         //��������� ���� ��������� �� ���������� �� ���������� �����
         ++Begin;

         OptimalFillImpl( Begin, End, CurSolve, CurFreeSpace, BestFreeSpace, StopFlag );
      }
      else
         ++Begin;

      CurSolve.pop_back();
   }
}
/////////////////////////////////////////////////////////////////////////////////

//��������� ����������� ���������� ����������
void OptimalFill( TItems &Items, TFileSize FreeSpace )
{
   //�� ��������� ����� ��� ���� ����� ��� ������ ������� ������� ����������� ����� 
   //������� �����, � ������ �������� �������������� ��������������� ���������� �������
   //��� ��������� ������� �������� �������
   std::sort( Items.begin(), Items.end(), SizeCmp() );

   TSolve Solve;
   bool StopFlag = false;

   OptimalFillImpl( Items.begin(), Items.end(), Solve, FreeSpace, FreeSpace, StopFlag );
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

const wchar_t * const szDescription = 
   L"OptimalDiskFill v1.8.1\n" 
   L"������� ��������� (c) 2010\n"
   L"��������� �������� ���������� ��������� ������� CD, DVD ������ ��� ������\n"
   L"��������� ��� ������ ������ �� ���. ��� ��������� ������� � ���������\n" 
   L"������ ��� ������ � ��������� ���������, ����� ����� ������� �����\n" 
   L"���������� ������ � ���������, ��� ������� ��� ������ ������������ ������,\n" 
   L"�� �� ������ ���������.\n"
   L"\n"
   L"�������������:\n"
   L"      OptimalDiskFill \"<������� 1>\" \"[������� 2]\" \"[������� N]\"\n"
   L"                      \"<�������� �����>[Tb|Gb|Mb|Kb|b]\"\n"
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

      for( int curArg = 1; curArg < argc - 1; ++curArg )
      {
          Dirs.push_back(argv[curArg]);

          if( PathIsDirectory(Dirs.back().c_str()) == FALSE )
              APL_THROW( _T("������� '") << Dirs.back().c_str() << _T("' �� ������") );
      }

      FreeSpace = TFormat::ConvertDims( argv[argc - 1] );
 
      if( FreeSpace <= 0 )
         APL_THROW( _T("�������� ������ ������ ���� ������ ����") );
#else
      //Dirs.push_back(_T("c:\\_games\\_PSP\\_nead_burn") );
      Dirs.push_back(_T("c:\\!My_docs\\_���������\\_NeadBurn\\������"));
      FreeSpace = TFileSize(4483) * 1024 * 1024;
#endif

      TItems Items;
      
      std::cout << "�������� �����:  " << TFormat(FreeSpace) << std::endl;

      for( TDirs::const_iterator it = Dirs.begin(); it != Dirs.end(); ++it )
      {
          std::basic_stringstream<TCHAR> FilePrefix;

          if( Dirs.size() > 1 )
            FilePrefix << it - Dirs.begin() + 1 << _T("\\");

          std::cout << "������ �������� " << FilePrefix.str() << ": " << *it << " ..." << std::endl;
          LoadItems( *it, Items, FilePrefix.str() ); 
      }
          
      //PrintFileSizes( Items );
      PrintNotFitToDestDirs( Items, FreeSpace );

      std::cout << "\n�������� ����� �������..." << std::endl;
      OptimalFill( Items, FreeSpace );

      std::cout << "\n����� ��������. ��������� ������� �����������." << std::endl;
   }
   APL_CATCH()

   return 0;
}