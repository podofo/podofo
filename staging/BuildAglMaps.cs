using System;
using System.Collections.Generic;
using System.IO;
using System.Text;

var builder = new StringBuilder();
var ligatures = new List<Ligature>();

foo(@"D:\glyphlist.txt", AGLType.AdobeGlyphList);
foo(@"D:\aglfn.txt", AGLType.AdobeGlyphListNewFonts);
foo(@"D:\zapfdingbats.txt", AGLType.ZapfDingbatsGlyphList);

File.WriteAllText(@"D:\initagl.txt", builder.ToString());

builder.Clear();

foreach (var ligature in ligatures)
{
    builder.Append($"ligatures->emplace_back(aglMap->find(\"{ligature.Name}\")->first, initializer_list<codepoint>{{");
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

void foo(string filepath, AGLType type)
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
                case AGLType.AdobeGlyphList:
                case AGLType.ZapfDingbatsGlyphList:
                {
                    codeStr = splitted[1];
                    charName = splitted[0];
                    break;
                }
                case AGLType.AdobeGlyphListNewFonts:
                {
                    codeStr = splitted[0];
                    charName = splitted[1];
                    break;
                }
                default:
                    throw new NotImplementedException();
            }

            var codes = readCodes(codeStr);
            string code;
            if (codes.Length == 1)
            {
                code = $"0x{codes[0].ToString("X4")}";
            }
            else
            {
                code = ligatures.Count.ToString();
                ligatures.Add(new Ligature() { Codes = codes, Name = charName });
            }

            builder.AppendLine($"aglMap->emplace(\"{charName}\"_n, AGLMapping{{ AGLMapType::{type.ToString()}, {codes.Length}, {code}}});");
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

struct Ligature
{
    public string Name;
    public ushort[] Codes;
}

enum AGLType
{
    AdobeGlyphList = 1,
    AdobeGlyphListNewFonts,
    ZapfDingbatsGlyphList,
}
