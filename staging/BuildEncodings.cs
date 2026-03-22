using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;

class Program
{
    static void Main(string[] args)
    {
        string inputPath = @"D:\Temp\Enc\symbol.txt";
        string outputPath = @"D:\symbol.txt";

        // Dictionary: 1-byte code unit (0-255) -> list of (unicode code point, comment)
        var map = new Dictionary<int, List<(int codePoint, string comment)>>();

        foreach (string raw in File.ReadLines(inputPath))
        {
            string line = raw.Trim();
            if (string.IsNullOrEmpty(line) || line.StartsWith("#"))
                continue;

            string[] parts = line.Split(';');
            if (parts.Length < 3)
                continue;

            if (!int.TryParse(parts[0].Trim(), System.Globalization.NumberStyles.HexNumber, null, out int codePoint))
                continue;

            if (!int.TryParse(parts[1].Trim(), System.Globalization.NumberStyles.HexNumber, null, out int byteUnit))
                continue;

            string comment = FormatComment(parts[2].Trim());

            if (!map.ContainsKey(byteUnit))
                map[byteUnit] = new List<(int, string)>();

            map[byteUnit].Add((codePoint, comment));
        }

        using var writer = new StreamWriter(outputPath);

        for (int b = 0; b <= 255; b++)
        {
            if (!map.TryGetValue(b, out var entries) || entries.Count == 0)
            {
                writer.WriteLine("0x0000,");
                continue;
            }

            for (int i = 0; i < entries.Count; i++)
            {
                var (codePoint, comment) = entries[i];
                string entry = $"0x{codePoint:X4}, // {b:X2} {comment}";

                if (i % 2 == 0)
                    writer.WriteLine(entry);
                else
                    writer.WriteLine($"// {entry}");
            }
        }

        Console.WriteLine($"Done. Output written to: {outputPath}");
    }

    static string FormatComment(string comment)
    {
        // Normalize tabs to spaces
        comment = comment.Replace('\t', ' ');

        // Find the second '#' to split on
        int firstHash = comment.IndexOf('#');
        if (firstHash < 0)
            return comment;

        int secondHash = comment.IndexOf('#', firstHash + 1);
        if (secondHash < 0)
            return comment;

        string firstPart = comment[..secondHash].TrimEnd();
        string secondPart = comment[secondHash..];

        return firstPart.PadRight(40) + " " + secondPart;
    }
}