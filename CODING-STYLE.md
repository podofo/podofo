## Coding style

Coding style in PoDoFo follows the following general rules:
- Visual Studio formatting conventions (see for example [C# conventions](https://docs.microsoft.com/en-us/dotnet/csharp/fundamentals/coding-style/coding-conventions)), where they apply to C/C++ like constructs;
- Capitalized case for **public** `MethodNames()`, or private/protected methods names intended to be called outside the boundary of the current class through inheritance or class/method friendship (eg. `ProtectedMethod()`, `FriendMethod()`);
- Lower case for private/virtual protected methods intended to be called only within the current class, eg. `initOperation()`. It may be common to have a public outer method `MyMethod()` that performs some validations first and subsequently calls a private or virtual protected inner method `myMethod()` that actually performs the task;
- Camel case for variables, parameters and fields (example `variableName`).

Some language specific programming stylistic choices will be hard enforced, namely:
- C style programming (`malloc`/`free`, string formatting, etc.) shall be converted to equivalent modern C++ constructs/API calls, unless there's still no other modern C++ alternative (eg. `scanf`). Where possible `new`/`delete` semantics shall be replaced by use of smart pointers;
- `m_` and `s_` prefixes shall be used respectively for instance member and static fields/variables. For example `m_Value` or `s_instance`. No other Hungarian notation prefixes will be accepted;
- Implicit conversion from pointer types to bool (eg. `if (!ptr)` or `while (ptr)` conditionals) shall be converted to check for `nullptr`, eg. `if (ptr == nullptr)` or `while (ptr != nullptr)`;
- Increment/decrement operators shall always be postfixed in epxressions as in `it++`, unless the prefixed operator is necessary for the correctness of an algorithm or allows to shorten/optimize the code.

It follows other lenient rules that are not truly enforced in all the code base, but that it is still recommended to follow:
- Lower case field names for private fields used only in this class boundary, for example `m_privateField`;
- Capitalized field names for private fields exposed by public getters/setters, example `m_Value`.

### PoDoFo specifics

- `PdfDictionary` and other methods doing lookups on `PdfName` with keys known at compile time: `GetKey`, `FindKey`, `FindKeyHas`, `HasKey`, `RemoveKey` methods (and in general non addition lookup methods) use string literals or `string_view`. For `AddKey`, `AddKeyIndirect`, `AddKeyIndirectSafe` use the "_n" user literal
- `std::string_view` are always passed by const reference in the public API

Some examples of expected coding style can be found at the following permalinks:

- [PdfEncoding](https://github.com/podofo/podofo/blob/94145e4bd452cfd77b262dd672ec836da0c1530d/src/podofo/main/PdfEncoding.cpp)
- [PdfCharCodeMap](https://github.com/podofo/podofo/blob/94145e4bd452cfd77b262dd672ec836da0c1530d/src/podofo/main/PdfCharCodeMap.cpp)
- [PdfFontFactory](https://github.com/podofo/podofo/blob/94145e4bd452cfd77b262dd672ec836da0c1530d/src/podofo/main/PdfFontFactory.cpp)
- [PdfTrueTypeSubset](https://github.com/podofo/podofo/blob/94145e4bd452cfd77b262dd672ec836da0c1530d/src/podofo/private/FontTrueTypeSubset.cpp)