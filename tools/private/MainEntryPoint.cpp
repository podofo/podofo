#include <vector>
#include <string>
#include <iostream>

#include <utf8cpp/utf8.h>
#include <podofo/main/PdfError.h>

using namespace std;
using namespace PoDoFo;

extern void Main(const cspan<string_view>& args);

#ifdef _WIN32
int wmain(int argc, wchar_t* argv[])
#else // !_WIN32
int main(int argc, char* argv[])
#endif // _WIN32
{
    vector<string_view> args(argc);
#ifdef _WIN32
    vector<string> args_(argc);
    for (int i = 0; i < argc; i++)
    {
        args_[i] = utf8::utf16to8(u16string_view((const char16_t*)argv[i],
            char_traits<char16_t>::length((const char16_t*)argv[i])));
        args[i] = args_[i];
    }
#else // !_WIN32
    for (int i = 0; i < argc; i++)
        args[i] = string_view(argv[i], char_traits<char>::length(argv[i]));
#endif // _WIN32
    try
    {
        Main(args);
    }
    catch (const PdfError& ex)
    {
        cerr << "ERROR: An error occurred during processing the pdf file" << endl;
        cerr << ex.what() << endl;
        return (int)ex.GetCode();
    }

    return 0;
}
