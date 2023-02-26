## Coding style

Coding style in PoDoFo follows the following general rules:
- Visual Studio formatting conventions (see for example [C# convetions](https://docs.microsoft.com/en-us/dotnet/csharp/fundamentals/coding-style/coding-conventions)), where they apply to C/C++ like constructs;
- Capitalized case for **public** `MethodNames()`;
- Camel case for variables, parameters and fields (example `variableName`);

Some language specific programming stylistic choices will be hard enforced, namely:
- C style programming (`malloc`/`free`, string formatting, etc.) shall be converted to equivalent modern C++ constructs/API calls, unless there's still no other modern C++ alternative (eg. `scanf`). Where possible `new`/`delete` semantics shall be replaced by use of shared pointers;
- `m_` and `s_` prefixes shall be used respectively for instance member and static fields/variables. For example `m_Value` or `s_instance`. No other Hungarian notation prefixes will be accepted;
- Implicit confersion from pointer types to bool (eg. `if (!ptr)` or `while (ptr)` conditionals) shall be converted to check for `nullptr`, eg. `if (ptr == nullptr)` or `while (ptr != nullptr)`;
- Increment/decrement operators shall always be postfixed in epxressions as in `it++`, unless the prefixed operator is necessary for the correctness of an algorithm or allows to shorten/optimize the code.

It follows other lenient rules that are not truly enforced in all the code base, but that it is still recommended to follow:

- Lower case method names for private methods used only in this class. Example `initOperation()`;
- Capitalized case for method for private or protected methods used outside of this class boundary, in case of class friendship or inheritance. For example `FriendMethod()` or `ProtectedMethod`;
- Lower case field names for private fields used only in this class boundary, for example `m_privateField`;
- Capitalized field names for private fields exposed by public getters/setters, example `m_Value`.

Some examples of expected coding style can be found at the following permalinks:

- [PdfEncoding](https://github.com/pdfmm/pdfmm/blob/588ee42ca16e0996c73a7d7887d189672ae4cc18/src/pdfmm/base/PdfEncoding.cpp)
- [PdfCharCodeMap](https://github.com/pdfmm/pdfmm/blob/588ee42ca16e0996c73a7d7887d189672ae4cc18/src/pdfmm/base/PdfCharCodeMap.cpp)
- [PdfFontFactory](https://github.com/pdfmm/pdfmm/blob/588ee42ca16e0996c73a7d7887d189672ae4cc18/src/pdfmm/base/PdfFontFactory.cpp)
- [PdfTrueTypeSubset](https://github.com/pdfmm/pdfmm/blob/588ee42ca16e0996c73a7d7887d189672ae4cc18/src/pdfmm/base/PdfFontTrueTypeSubset.cpp)
