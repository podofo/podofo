using System;
using System.Collections.Generic;
using System.Collections.Specialized;
using System.IO;
using System.Reflection.Emit;
using System.Text;

var mappings = new OrderedDictionary<string, AglMapping>();
var ligatures = new List<AglLigature>();

foo(@"D:\glyphlist.txt", AglType.AdobeGlyphList);
foo(@"D:\aglfn.txt", AglType.AdobeGlyphListNewFonts);
foo(@"D:\zapfdingbats.txt", AglType.ZapfDingbatsGlyphList);

var builder = new StringBuilder();
foreach (var mapping in mappings)
{
    builder.AppendLine($"s_aglList.emplace_back(\"{mapping.Key}\"_n, AglMapping{{ (AglType){((int)mapping.Value.Type).ToString()}, {mapping.Value.CodePointCount}, {mapping.Value.CodeStr}}});");
}

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
}
