using System;
using System.Collections.Generic;
using System.IO;
using System.Text;

var mappings = new OrderedDictionary<string, AglMapping>();
var ligatures = new List<AglLigature>();
var inverseMap = new Dictionary<ushort, List<string>>();
var latinChars = new string[] {
    "A",
    "AE",
    "Aacute",
    "Acircumflex",
    "Adieresis",
    "Agrave",
    "Aring",
    "Atilde",
    "B",
    "C",
    "Ccedilla",
    "D",
    "E",
    "Eacute",
    "Ecircumflex",
    "Edieresis",
    "Egrave",
    "Eth",
    "Euro",
    "F",
    "G",
    "H",
    "I",
    "Iacute",
    "Icircumflex",
    "Idieresis",
    "Igrave",
    "J",
    "K",
    "L",
    "Lslash",
    "M",
    "N",
    "Ntilde",
    "O",
    "aring",
    "asciicircum",
    "asciitilde",
    "asterisk",
    "at",
    "atilde",
    "b",
    "backslash",
    "bar",
    "braceleft",
    "braceright",
    "bracketleft",
    "bracketright",
    "breve",
    "brokenbar",
    "bullet",
    "c",
    "caron",
    "ccedilla",
    "cedilla",
    "cent",
    "circumflex",
    "colon",
    "comma",
    "copyright",
    "currency",
    "d",
    "dagger",
    "daggerdbl",
    "degree",
    "dieresis",
    "divide",
    "dollar",
    "dotaccent",
    "dotlessi",
    "e",
    "eacute",
    "ecircumflex",
    "edieresis",
    "egrave",
    "eight",
    "ellipsis",
    "emdash",
    "endash",
    "equal",
    "oe",
    "ogonek",
    "ograve",
    "one",
    "onehalf",
    "onequarter",
    "onesuperior",
    "ordfeminine",
    "ordmasculine",
    "oslash",
    "otilde",
    "p",
    "paragraph",
    "parenleft",
    "parenright",
    "percent",
    "period",
    "periodcentered",
    "perthousand",
    "plus",
    "plusminus",
    "q",
    "question",
    "questiondown",
    "quotedbl",
    "quotedblbase",
    "quotedblleft",
    "quotedblright",
    "quoteleft",
    "quoteright",
    "quotesinglbase",
    "quotesingle",
    "r",
    "registered",
    "ring",
    "OE",
    "Oacute",
    "Ocircumflex",
    "Odieresis",
    "Ograve",
    "Oslash",
    "Otilde",
    "P",
    "Q",
    "R",
    "S",
    "Scaron",
    "T",
    "Thorn",
    "U",
    "Uacute",
    "Ucircumflex",
    "Udieresis",
    "Ugrave",
    "V",
    "W",
    "X",
    "Y",
    "Yacute",
    "Ydieresis",
    "Z",
    "Zcaron",
    "a",
    "aacute",
    "acircumflex",
    "acute",
    "adieresis",
    "ae",
    "agrave",
    "ampersand",
    "eth",
    "exclam",
    "exclamdown",
    "f",
    "fi",
    "five",
    "fl",
    "florin",
    "four",
    "fraction",
    "g",
    "germandbls",
    "grave",
    "greater",
    "guillemotleft",
    "guillemotright",
    "guilsinglleft",
    "guilsinglright",
    "h",
    "hungarumlaut",
    "hyphen",
    "i",
    "iacute",
    "icircumflex",
    "idieresis",
    "igrave",
    "j",
    "k",
    "l",
    "less",
    "logicalnot",
    "lslash",
    "m",
    "macron",
    "minus",
    "mu",
    "multiply",
    "n",
    "nine",
    "ntilde",
    "numbersign",
    "o",
    "oacute",
    "ocircumflex",
    "odieresis",
    "s",
    "scaron",
    "section",
    "semicolon",
    "seven",
    "six",
    "slash",
    "space",
    "sterling",
    "t",
    "thorn",
    "three",
    "threequarters",
    "threesuperior",
    "tilde",
    "trademark",
    "two",
    "twosuperior",
    "u",
    "uacute",
    "ucircumflex",
    "udieresis",
    "ugrave",
    "underscore",
    "v",
    "w",
    "x",
    "y",
    "yacute",
    "ydieresis",
    "yen",
    "z",
    "zcaron",
    "zero",
};

foo(@"D:\glyphlist.txt", AglType.AdobeGlyphList);
foo(@"D:\aglfn.txt", AglType.AdobeGlyphListNewFonts);
foo(@"D:\zapfdingbats.txt", AglType.ZapfDingbatsGlyphList);

foreach (var latin in latinChars)
    mappings[latin].Type |= AglType.LatinTextEncodings;

var builder = new StringBuilder();
foreach (var mapping in mappings)
    builder.AppendLine($"s_aglList.emplace_back(\"{mapping.Key}\"_n, AglMapping{{ (AglType){((int)mapping.Value.Type).ToString()}, {mapping.Value.CodePointCount}, {mapping.Value.CodeStr}}});");

File.WriteAllText(@"D:\initagl.txt", builder.ToString());

builder.Clear();
foreach (var ligature in ligatures)
{
    builder.Append($"s_ligatures.emplace_back(s_aglMap[\"{ligature.Name}\"], initializer_list<codepoint>{{");
    bool first = true;
    for (int i = 0; i < ligature.Codes.Length; i++)
    {
        if (first)
            first = false;
        else
            builder.Append(", ");

        builder.Append($"0x{ligature.Codes[i]:X4}");
    }

    builder.AppendLine(" });");
}

File.WriteAllText(@"D:\ligatures.txt", builder.ToString());

void foo(string filepath, AglType type)
{
    using (var stream = new FileStream(filepath, FileMode.Open))
    using (var reader = new StreamReader(stream))
    {
        while (!reader.EndOfStream)
        {
            var line = reader.ReadLine()!;
            if (line.Length == 0 || line.StartsWith("#"))
                continue;

            var splitted = line.Split(';');
            string codeStr;
            string charName;
            switch (type)
            {
                case AglType.AdobeGlyphList:
                case AglType.ZapfDingbatsGlyphList:
                {
                    codeStr = splitted[1];
                    charName = splitted[0];
                    break;
                }
                case AglType.AdobeGlyphListNewFonts:
                {
                    codeStr = splitted[0];
                    charName = splitted[1];
                    break;
                }
                default:
                    throw new NotImplementedException();
            }

            var codes = readCodes(codeStr);
            string codeOrIndex;
            if (codes.Length == 1)
            {
                codeOrIndex = $"0x{codes[0].ToString("X4")}";
            }
            else
            {
                codeOrIndex = ligatures.Count.ToString();
                ligatures.Add(new AglLigature() { Codes = codes, Name = charName });
            }

            if (mappings.TryGetValue(charName, out var mapping))
            {
                mapping.Type |= type;
            }
            else
            {
                mapping = new AglMapping() { Type = type, CodePointCount = codes.Length, CodeStr = codeOrIndex };
                mappings.Add(charName, mapping);
            }

            if (codes.Length == 1)
            {
                if (inverseMap.TryGetValue(codes[0], out var names))
                {
                    if (names.Contains(charName))
                        continue;

                    names.Add(charName);
                }
                else
                {
                    names = [charName];
                    inverseMap.Add(codes[0], names);
                }
            }
        }
    }
}

ushort[] readCodes(string codeStr)
{
    var splitted = codeStr.Split(' ');
    var ret = new ushort[splitted.Length];
    for (int i = 0; i < splitted.Length; i++)
        ret[i] = ushort.Parse(splitted[i], System.Globalization.NumberStyles.HexNumber);

    return ret;
}

class AglMapping
{
    public AglType Type;
    public int CodePointCount;
    public string CodeStr = null!;
}

class AglLigature
{
    public string Name = null!;
    public ushort[] Codes = null!;
}

[Flags]
enum AglType
{
    AdobeGlyphList = 1,
    AdobeGlyphListNewFonts = 2,
    ZapfDingbatsGlyphList = 4,
    LatinTextEncodings = 8,
}
