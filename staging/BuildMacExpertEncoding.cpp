#include <podofo/private/PdfDeclarationsPrivate.h>
#include <podofo/podofo.h>


// All PoDoFo classes are member of the PoDoFo namespace.
using namespace std;
using namespace PoDoFo;

int main(int argc, char* argv[])
{
    fstream file(R"(D:\OneDrive\ShareWork\Podofo\MacExpertEncoding.txt)", ios::in);
    CodePointSpan codePoints;
    string name;
    string encoding;
    string temp;
    std::getline(file, name);
    while (file)
    {
        if (name == ".notdef")
        {
            encoding.append("0x0000,\n");
            std::getline(file, name);
            continue;
        }

        if (!PdfDifferenceEncoding::TryGetCodePointsFromCharName(name, codePoints))
            throw runtime_error(name);

        utls::FormatTo(temp, "0x{:04X},\n", (unsigned)*codePoints);
        encoding.append(temp);
        std::getline(file, name);
    }

    return 0;
}
