/*
   Программа решает задачу оптимального (минимизируя незаполненое место) заполнения болванки файлами из некоторой папки.
   Фактически эта задача об одномерной оптимальной упаковке, или задача о рюкзаке, 
   формулируется так: пусть имеется рюкзак заданной грузоподъемности; также имеется некоторое множество 
   предметов различного веса и различной стоимости (ценности); требуется упаковать рюкзак так, чтобы он закрывался 
   и сумма стоимостей упакованных предметов была бы максимальной.
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


//Напечатать размер m_Size в подходящей системе исчисления b|kb|mb|gb
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

   //Позволяет конвертировать числа в формате 128MB в байты
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
      APL_THROW( _T("Ошибка при преобразовании параметра '") << S << _T("' в число") );

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
   
   APL_THROW( _T("Ошибка при определении единиц измерения параметра '") << S << _T("'") );

   return 0;
}
///////////////////////////////////////////////////////////////////////////////

//Информации по отдельной директории или файлу
struct TItem
{
   TString Name;     //Имя каталога или файла
   TFileSize Size;   //Размер в байтах

   TItem(): Size(0) {} 
};

typedef std::vector<TItem> TItems; 

//Предикат упорядочивания файлов по размеру
struct SizeCmp: std::binary_function<TItem, TItem, bool>
{
   bool operator()( const TItem &a1, const TItem &a2 ) const
   {
      return a1.Size > a2.Size;
   }
};

//Частное решение
typedef std::vector<TItems::const_iterator> TSolve;

/////////////////////////////////////////////////////////////////////////////////

//Получить размер файла из структуры FD
inline TFileSize GetSize( const WIN32_FIND_DATA &FD )
{
   TFileSize RetVal = FD.nFileSizeHigh;
   
   RetVal *= TFileSize(MAXDWORD) + 1;
   RetVal += FD.nFileSizeLow;
   return RetVal;
}
/////////////////////////////////////////////////////////////////////////////////

//Вычислить размер каталога в байтах
TFileSize CalcDirSize( const TString &Dir )
{
   TFileSize RetVal = 0;

   NWLib::CDirectoryFileList DFL(Dir);

   while( DFL.Next() )
      RetVal += GetSize( DFL.GetFindData() );

   return RetVal;
}

//Находим в каталоге Dir все файлы и каталоги, исключая вложенные и заполняем Items
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

//Печатаем частное решение
void PrintSolve( TFileSize FreeSpace, const TSolve &Solve)
{
   std::cout << "\n----- Найдено частное решение -----" << std::endl;
   std::cout << "Осталось не использованно: " << TFormat(FreeSpace) << std::endl;

   TFileSize TotalSize = 0;
   for( TSolve::const_iterator I = Solve.begin(); I != Solve.end(); ++I )
   {
      std::cout << (*I)->Name << std::endl;
      TotalSize += (*I)->Size;
   }
   
   std::cout << "Всего: " << TFormat(TotalSize) << std::endl;
}

//Основная функция выполняющая вычисления
void OptimalFillImpl( TItems::const_iterator Begin, TItems::const_iterator End, TSolve &CurSolve, TFileSize FreeSpace, TFileSize &BestFreeSpace, bool &StopFlag )
{  
   //На идею алгоритма меня натолкнула статья:
   //"Задача об одномерной оптимальной упаковке" 
   //http://www.wikiznanie.ru/ru-wz/index.php/Задача_об_одномерной_оптимальной_упаковке
   //Мой алгоритм отличается от того который описан в статье (я не потратил 
   //дастаточно времени на разбор того что там написано и поспешил реализовать то что у меня было в голове).
   //Задача о рюкзаке является экспоненциальной, и в худшем случае алгоритм работает именно так.
   //Алгоритм перебирает все варианты, так что сначала берётся некоторый файл, кладётся в каталог и для него
   //перебираются все возможные комбинации других файлов, после чего, файл можно отбросить.
   //Такая процедура повторяется рекурсивно.

   TFileSize CurFreeSpace;

   while( Begin != End && !StopFlag )
   {
      //Для того чтобы было возможно напечатать решение мы сохраняем текущие данные
      CurSolve.push_back( Begin ); 

      if( Begin->Size <= FreeSpace ) //Если файл поместится в каталоге
      {
         //Осталось свободного места после помещения файла
         CurFreeSpace = FreeSpace - Begin->Size;
         
         if( CurFreeSpace < BestFreeSpace )
         {
            //Нашли новое решение
            BestFreeSpace = CurFreeSpace;
            PrintSolve(BestFreeSpace, CurSolve);

            if( BestFreeSpace == 0 )
               StopFlag = true;
         }

         //Вниз по стеку мы передаём интервал не включающий [начало; текущий]
         //Поскольку этот интерывал мы обработали на предыдущих шагах
         ++Begin;

         OptimalFillImpl( Begin, End, CurSolve, CurFreeSpace, BestFreeSpace, StopFlag );
      }
      else
         ++Begin;

      CurSolve.pop_back();
   }
}
/////////////////////////////////////////////////////////////////////////////////

//Вычислить оптимальное заполнение директории
void OptimalFill( TItems &Items, TFileSize FreeSpace )
{
   //Мы сортируем файлы для того чтобы при поиске решений сначала учитывались самые 
   //большие файлы, а точная подгонка осуществлялась комбинированием маленькими файлами
   //Это позволяет быстрее находить решения
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

//Напечатать файлы/каталоги которые не поместятся в заданный размер
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
            std::cout << "Обнаружены файлы/каталоги которые не поместятся в заданный объём:" << std::endl;
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
   L"Дмитрий Шестеркин (c) 2010\n"
   L"Программа помогает оптимально заполнить емкость CD, DVD дисков или других\n"
   L"носителей при записи данных на них. Она сканирует каталог и вычисляет\n" 
   L"размер его файлов и вложенных каталогов, после этого находит такую\n" 
   L"комбинацию файлов и каталогов, при которой они займут максимальный размер,\n" 
   L"но не больше заданного.\n"
   L"\n"
   L"Использование:\n"
   L"      OptimalDiskFill \"<Каталог 1>\" \"[Каталог 2]\" \"[Каталог N]\"\n"
   L"                      \"<Желаемый объём>[Tb|Gb|Mb|Kb|b]\"\n"
   L"\n"
   L"Примечание:\n"
   L"      Поскольку в общем виде задача поиска оптимальной комбинации является\n" 
   L"      экспоненциально сложной, программа выводит частные решения по мере\n" 
   L"      их нахождения и МОЖЕТ РАБОТАТЬ ОЧЕНЬ ДОЛГО. Если вас уже устраивает\n" 
   L"      текущее решение, скопируйте его и закройте программу,\n" 
   L"      нажав CTRL+C или CTRL+BREAK.\n";
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
         APL_THROW( _T("Недопустимое количество аргументов. См. Использовние") );

      for( int curArg = 1; curArg < argc - 1; ++curArg )
      {
          Dirs.push_back(argv[curArg]);

          if( PathIsDirectory(Dirs.back().c_str()) == FALSE )
              APL_THROW( _T("Каталог '") << Dirs.back().c_str() << _T("' не найден") );
      }

      FreeSpace = TFormat::ConvertDims( argv[argc - 1] );
 
      if( FreeSpace <= 0 )
         APL_THROW( _T("Желаемый размер должен быть больше нуля") );
#else
      //Dirs.push_back(_T("c:\\_games\\_PSP\\_nead_burn") );
      Dirs.push_back(_T("c:\\!My_docs\\_Разобрать\\_NeadBurn\\Фильмы"));
      FreeSpace = TFileSize(4483) * 1024 * 1024;
#endif

      TItems Items;
      
      std::cout << "Желаемый объём:  " << TFormat(FreeSpace) << std::endl;

      for( TDirs::const_iterator it = Dirs.begin(); it != Dirs.end(); ++it )
      {
          std::basic_stringstream<TCHAR> FilePrefix;

          if( Dirs.size() > 1 )
            FilePrefix << it - Dirs.begin() + 1 << _T("\\");

          std::cout << "Чтение каталога " << FilePrefix.str() << ": " << *it << " ..." << std::endl;
          LoadItems( *it, Items, FilePrefix.str() ); 
      }
          
      //PrintFileSizes( Items );
      PrintNotFitToDestDirs( Items, FreeSpace );

      std::cout << "\nНачинаем поиск решений..." << std::endl;
      OptimalFill( Items, FreeSpace );

      std::cout << "\nПоиск закончен. Последнее решение оптимальное." << std::endl;
   }
   APL_CATCH()

   return 0;
}