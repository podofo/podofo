## Coding style

Coding style in PoDoFo follows the following general rules:

- Visual Studio formatting conventions (see for example [C# conventions](https://docs.microsoft.com/en-us/dotnet/csharp/fundamentals/coding-style/coding-conventions)), where they apply to C/C++ like constructs
- Capitalized case for **public** `MethodNames()`, or private/protected methods names intended to be called outside the boundary of the current class through inheritance or class/method friendship (eg. `ProtectedMethod()`, `FriendMethod()`)
- Lower case for private/virtual protected methods intended to be called only within the current class, eg. `initOperation()`. It may be common to have a public outer method `MyMethod()` that performs some validations first and subsequently calls a private or virtual protected inner method `myMethod()` that actually performs the task
- Camel case for variables, parameters and fields (example `variableName`)

Some language specific programming stylistic choices will be hard enforced, namely:

- C style programming (`malloc`/`free`, string formatting, etc.) shall be converted to equivalent modern C++ constructs/API calls, unless there's still no other modern C++ alternative (eg. `scanf`). Where possible `new`/`delete` semantics shall be replaced by use of smart pointers
- `m_` and `s_` prefixes shall be used respectively for instance member and static fields/variables. For example `m_Value` or `s_instance`. No other Hungarian notation prefixes will be accepted
- Implicit conversion from pointer types to bool (eg. `if (!ptr)` or `while (ptr)` conditionals) shall be converted to check for `nullptr`, eg. `if (ptr == nullptr)` or `while (ptr != nullptr)`
- Increment/decrement operators shall always be postfixed in expressions as in `it++`, unless the prefixed operator is necessary for the correctness of an algorithm or allows to shorten/optimize the code

The following rules are lenient and are not truly enforced in all the code base, but they are still recommended to follow:

- Lower case field names for private fields used only in this class boundary, for example `m_privateField`
- Capitalized field names for private fields exposed by public getters/setters, example `m_Value`

All other coding conventions, covered in detail, are organized by category in the sections below.

Some examples of expected coding style can be found at the following permalinks:

- [PdfEncoding](https://github.com/podofo/podofo/blob/94145e4bd452cfd77b262dd672ec836da0c1530d/src/podofo/main/PdfEncoding.cpp)
- [PdfCharCodeMap](https://github.com/podofo/podofo/blob/94145e4bd452cfd77b262dd672ec836da0c1530d/src/podofo/main/PdfCharCodeMap.cpp)
- [PdfFontFactory](https://github.com/podofo/podofo/blob/94145e4bd452cfd77b262dd672ec836da0c1530d/src/podofo/main/PdfFontFactory.cpp)
- [PdfTrueTypeSubset](https://github.com/podofo/podofo/blob/94145e4bd452cfd77b262dd672ec836da0c1530d/src/podofo/private/FontTrueTypeSubset.cpp)

### Whitespace
- 4-space indentation, LF line endings, UTF-8, trailing whitespace trimmed (see `.editorconfig`)

### SPDX license headers
- All source files should begin with SPDX copyright and license headers:
  ```cpp
  // SPDX-FileCopyrightText: YEAR Author <email>
  // SPDX-License-Identifier: LGPL-2.0-or-later OR MPL-2.0
  ```

### Comments and documentation
- `///` with at-sign `@` tags is used for Doxygen-style documentation comments on public methods
- Don't indent at-sign tags, put just one space after `///`
- Comments should be concise and avoid repetition

### Brace and statement style
- Allman braces: opening brace on its own line for namespaces, classes, functions, lambdas and control-flow blocks
- `else`/`else if` go on a new line after the previous closing brace
- Single-statement `if`/`while`/`for` bodies without braces are preferred

### Access specifiers and method organization
- Access specifiers should be organized in this order: `public`, `protected`, `private`
- Static factory methods and creation methods/constructors should appear early in the `public` section
- Access specifiers can be repeated to separate groups of functionalities
- Private methods may have more permissive preconditions and skip validation that public methods perform
- Trivial one-line/simple expression getter methods are allowed to be implemented inline in header files

### Method naming patterns
- `Try*` prefix for methods that return `bool` to indicate success/failure (e.g., `TryGetGID()`, `TryConvertToUtf8()`)
- `Must*` prefix for methods that throw on failure instead of returning bool (e.g., `MustGetKey()`, `MustFindKey()`)
- `Get*` for simple accessor methods that may return nullptr or throw
- `Find*` for lookup methods that search through collections or hierarchies
- `Add*` for methods that insert/append items
- `Remove*` for methods that delete items
- `Set*` for setter methods
- `Is*` or `Has*` for boolean query methods
- `*Unsafe` suffix for private methods that bypass safety checks or assume preconditions
- `*NoDirtySet` suffix for internal methods that modify state without setting dirty flags

### Const correctness
- Const correctness is generally enforced everywhere (an exception is accessing the document `PdfIndirectObjectList` instance)
- Both const and non-const overloads are provided for accessor methods that return references or pointers (e.g., `GetKey()`, `FindKey()`)
- Const methods should not modify object state, except for mutable members used for caching/lazy loading

### Namespace usage
- `using namespace std;` and `using namespace PoDoFo;` are used in implementation files (.cpp) for brevity
- `using namespace` is not used in header files

### PoDoFo specifics

- `PdfElement` inheritors shall not have a public constructor accepting `PdfDocument` as an argument. Such constructors are infrastructural only
- `PdfDictionary` and other methods doing lookups on `PdfName` with keys known at compile time: `GetKey`, `FindKey`, `FindKeyHas`, `HasKey`, `RemoveKey` methods (and in general non addition lookup methods) take string literals or `string_view`. For `AddKey`, `AddKeyIndirect`, `AddKeyIndirectSafe` the `_n` user literal is used
- `std::string_view` are always passed by const reference in the public API

### Friend declarations
- `friend` declarations are used to grant access to related classes (e.g., `friend class PdfDocument;`)
- The `PODOFO_PRIVATE_FRIEND` macro is used for private (not part of the public API) implementation classes

### Constructors and move semantics
- Constructor arguments are generally stored as class fields. Static `Create*()` factory methods are preferred when member fields need to be synthesized from constructor arguments
- `shared_ptr<T>` are generally passed by value in public constructors and by r-value `shared_ptr<T>&&` references in private constructors/methods
- `noexcept` is used on move operations

### Header conventions
- Include guards take the `#ifndef PDF_*_H` / `#define ... / #endif` form
- Public classes and free functions intended to be exported are annotated with `PODOFO_API`
- `final` is applied to classes and helper types (iterators, parameter structs) that are not designed for inheritance
- PoDoFo classes are forward-declared inside `namespace PoDoFo { ... }` at the top of a header when a full include can be avoided

### `.cpp` file layout
- Include order: `<podofo/private/PdfDeclarationsPrivate.h>` first, then the matching own header, then system, 3rd-party and other PoDoFo headers, generally in that order
- Right after `using namespace std;` / `using namespace PoDoFo;`, `static` forward prototypes for translation-unit-local helpers are declared, with their definitions placed later in the file
- Translation-unit-local types are placed in an anonymous `namespace { ... }`; translation-unit-local free functions are marked `static`

### Enums
- An explicit underlying type is always declared, e.g. `enum class PdfXxx : uint8_t`. The minimum size required should be used for regular enums, while flags shall use `uint32_t` 
- The first enumerator is conventionally a sentinel: `Unknown = 0` for value enums, `None = 0` for flag enums
- For bitmask-style enums, the operators are declared by placing `ENABLE_BITMASK_OPERATORS(PoDoFo::PdfXxxFlags);` outside the `PoDoFo` namespace, near the bottom of the header

### Errors, asserts, logging
- Throwing shall be done with the following macros:
  - `PODOFO_RAISE_ERROR(PdfErrorCode::X)` for plain throws
  - `PODOFO_RAISE_ERROR_INFO(PdfErrorCode::X, "msg {}", ...)` when a formatted message is needed
  - `PODOFO_RAISE_LOGIC_IF(cond, "msg")` for guarded `InternalLogic` throws
- `PODOFO_PUSH_FRAME` / `PODOFO_PUSH_FRAME_INFO` are used to add frames to an existing `PdfError`
- `PODOFO_ASSERT(cond)` is used for debug-only sanity checks and `PODOFO_INVARIANT(cond)` to document invariants without runtime cost
- Logging goes through `PoDoFo::LogMessage(PdfLogSeverity::..., "msg {}", ...)`

### Comment markers
- `// NOTE:` explanatory notes worthy of attention
- `// TODO:` for planned work
- `// FIXME:` for known defects
- `// CHECK-ME:` for code that should be reviewed/validated

 They should be kept short (one or two lines).
