using ICSharpCode.SharpZipLib.Zip.Compression;
using ICSharpCode.SharpZipLib.Zip.Compression.Streams;
using System.Text;

var stream = CompressFile(@"D:\GitHub\XMP-RNG-Schema\Merged_XMP_Packet.rng");
using StreamWriter output = new StreamWriter(@"D:\xmp.txt", false, Encoding.ASCII);

int bytesPerLine = 16;
byte[] buffer = new byte[bytesPerLine];
int bytesRead;

while ((bytesRead = stream.Read(buffer, 0, buffer.Length)) > 0)
{
    StringBuilder line = new StringBuilder();
    line.Append('"');
    for (int i = 0; i < bytesRead; i++)
    {
        line.Append('\\');
        line.Append(Convert.ToString(buffer[i], 8).PadLeft(3, '0'));
    }

    line.Append('"');

    output.WriteLine(line.ToString());
}

static Stream CompressFile(string path)
{
    using FileStream originalFileStream = File.Open(path, FileMode.Open);
    using var compressedMemStream = new MemoryStream();
    var deflater = new Deflater(-1, false);
    var compressor = new DeflaterOutputStream(compressedMemStream, deflater);
    originalFileStream.CopyTo(compressor);
    compressor.Flush();
    return new MemoryStream(compressedMemStream.ToArray());
}