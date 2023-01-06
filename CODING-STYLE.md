## Coding style

Coding style in PoDoFo follows the following general rules:
- Visual Studio formatting convetions (see for example [C# convetions](https://docs.microsoft.com/en-us/dotnet/csharp/fundamentals/coding-style/coding-conventions), where they apply for all C based languages;
- Capitalized case for **public** ```MethodNames()```;
- Camel case for variables, parameters and fields (example ```variableName```);
- ```m_``` and ```s_``` prefixes respectively for and static fields/variables. For example ```m_Value``` or ```s_instance```.

As some lenient rules, not truly enforced in all the code base, it's recommended to use:

- Lower case method names for private methods used only in this class. Example ```initOperation()```;
- Capitalized case for method for private or protected methods used outside of this class boundary, in case of class friendship or inheritance. For example ```FriendMethod()``` or ```ProtectedMethod```;
- Lower case field names for private fields used only in this class boundary, for example ```m_privateField```;
- Capitalized field names for private fields exposed by public getters/setters, example ```m_Value```.

Some examples of expected coding style can be found at the following permalinks:

- [PdfEncoding](https://github.com/pdfmm/pdfmm/blob/588ee42ca16e0996c73a7d7887d189672ae4cc18/src/pdfmm/base/PdfEncoding.cpp)
- [PdfCharCodeMap](https://github.com/pdfmm/pdfmm/blob/588ee42ca16e0996c73a7d7887d189672ae4cc18/src/pdfmm/base/PdfCharCodeMap.cpp)
- [PdfFontFactory](https://github.com/pdfmm/pdfmm/blob/588ee42ca16e0996c73a7d7887d189672ae4cc18/src/pdfmm/base/PdfFontFactory.cpp)
- [PdfTrueTypeSubset](https://github.com/pdfmm/pdfmm/blob/588ee42ca16e0996c73a7d7887d189672ae4cc18/src/pdfmm/base/PdfFontTrueTypeSubset.cpp)