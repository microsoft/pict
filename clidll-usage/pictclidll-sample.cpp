#include <string>
#include <iostream>

using namespace std;

// pict.dll exports this method, make sure you have pict.dll next to this executable when running
extern int execute( int argc, wchar_t* args[], wstring& output );

int main()
{
    // The _execute_ method is the entry point to the command-line PICT and it behaves accordingly.
    // To use it, we need to mimic the runtime's handling of the console apps.  The first argument 
    // is the name of the program but in this case, the actual value doesn't matter. The second
    // argument is a path to the model file.
    const wchar_t* input[] = { L"", L"test.txt" };
    
    wstring output;

    int ret = execute( sizeof( input ) / sizeof( wchar_t* ),
                       const_cast<wchar_t**>( input ), 
                       output );
    wcout << output;
    
    return ret;
}
