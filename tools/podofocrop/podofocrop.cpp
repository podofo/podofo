/**
 * SPDX-FileCopyrightText: (C) 2010 Dominik Seichter <domseichter@web.de>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include <podofo/podofo.h>

#include <cstdlib>
#include <cstdio>

#include <sstream>
#include <vector>

#ifdef _WIN32
#include <podofo/private/WindowsLeanMean.h>
#else
# include <unistd.h>
# include <sys/types.h> 
# include <sys/wait.h> 
#endif // _WIN32

using namespace std;
using namespace PoDoFo;

void print_help()
{
    printf("Usage: podofocrop input.pdf output.pdf\n");
    printf("       This tool will crop all pages.\n");
    printf("       It requires ghostscript to be in your PATH\n");
    printf("\nPoDoFo Version: %s\n\n", PODOFO_VERSION_STRING);
}

void crop_page(PdfPage& page, const Rect& cropBox)
{
    PdfArray arr;
    /*
    printf("%f %f %f %f\n",
           rCropBox.X,
           rCropBox.Y,
           rCropBox.Width,
           rCropBox.Height);
    */
    cropBox.ToArray(arr);
    page.GetDictionary().AddKey("MediaBox", arr);
}

string get_ghostscript_output(const string_view& inputPath)
{
    string output;
    constexpr unsigned BufferLen = 256;
    char buffer[BufferLen];

#ifdef _WIN32
    DWORD count;
    char cmd[BufferLen];

    STARTUPINFOA si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    // Fenster nicht sichtbar
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    // Ausgabe umleiten
    HANDLE pipe;
    CreatePipe(&pipe, 0, 0, 0);
    si.dwFlags |= STARTF_USESTDHANDLES;
    //si.hStdOutput=pipe_wr;
    si.hStdError = pipe;

    _snprintf(cmd, BufferLen, "gs -dSAFER -sDEVICE=bbox -sNOPAUSE -q %s -c quit", inputPath.data());
    printf("Running %s\n", cmd);
    if (!CreateProcessA(NULL, cmd, NULL, NULL, TRUE, CREATE_DEFAULT_ERROR_MODE, NULL, NULL, &si, &pi))
    {
        printf("CreateProcess failed.");
        exit(1);
    }

    while (ReadFile(pipe, buffer, BufferLen, &count, NULL) && GetLastError() != ERROR_BROKEN_PIPE && count > 0)
    {
        printf("%s", buffer);
        output.append(buffer, count);
    }

    // eigenes Handle schliessen
    CloseHandle(pipe);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
#else
    pid_t pid;
    int p[2];
    int count;

    pipe(p);

    if ((pid = fork()) == 0)
    {
        // Child, launch ghostscript
        close(p[0]); // Close unused read end

        dup2(p[1], 2); // redirect stderr to stdout
        dup2(p[1], 1);

        //printf("HELLO\n");
        execlp("gs", "gs", "-dSAFER", "-sDEVICE=bbox",
            "-sNOPAUSE", "-q", inputPath.data(), "-c", "quit", NULL);
        printf("Fatal error, cannot launch ghostscript\n");
        exit(0);
    }
    else
    {
        close(p[1]); // Close unused write end
        while ((count = read(p[0], buffer, BufferLen)) > 0)
        {
            output.append(buffer, count);
        }
        wait(NULL);
    }
#endif // _WIN32
    return output;
}

vector<Rect> get_crop_boxes(const string_view& input)
{
    vector<Rect> rects;
    string output = get_ghostscript_output(input);

    stringstream ss(output);
    string line;
    Rect curRect;
    bool haveRect = false;
    while (std::getline(ss, line))
    {
        if (strncmp("%%BoundingBox: ", line.c_str(), 15) == 0)
        {
            int x, y, w, h;
            if (sscanf(line.c_str() + 15, "%i %i %i %i\n", &x, &y, &w, &h) != 4)
            {
                printf("Failed to read bounding box's four numbers from '%s'\n", line.c_str() + 15);
                exit(1);
            }
            curRect = Rect(static_cast<double>(x),
                static_cast<double>(y),
                static_cast<double>(w - x),
                static_cast<double>(h - y));
            haveRect = true;
        }
        else if (strncmp("%%HiResBoundingBox: ", line.c_str(), 17) == 0)
        {
            if (haveRect)
            {
                // I have no idea, while gs writes BoundingBoxes twice to stdout ..
                printf("Using bounding box: [ %f %f %f %f ]\n",
                    curRect.X,
                    curRect.Y,
                    curRect.Width,
                    curRect.Height);
                rects.push_back(curRect);
                haveRect = false;
            }
        }
    }

    return rects;
}

void Main(const cspan<string_view>& args)
{
    PdfCommon::SetMaxLoggingSeverity(PdfLogSeverity::None);

    if (args.size() != 3)
    {
        print_help();
        exit(-1);
    }

    auto inputPath = args[1];
    auto outputPath = args[2];

    printf("Cropping file:\t%s\n", inputPath.data());
    printf("Writing to   :\t%s\n", outputPath.data());

    vector<Rect> cropBoxes = get_crop_boxes(inputPath);

    PdfMemDocument doc;
    doc.Load(inputPath);

    if (cropBoxes.size() != doc.GetPages().GetCount())
    {
        printf("Number of cropboxes obtained form ghostscript does not match with page count (%u, %u)\n",
            static_cast<unsigned>(cropBoxes.size()), doc.GetPages().GetCount());
        PODOFO_RAISE_ERROR(PdfErrorCode::InvalidHandle);
    }

    for (unsigned i = 0; i < doc.GetPages().GetCount(); i++)
    {
        auto& page = doc.GetPages().GetPageAt(i);
        crop_page(page, cropBoxes[i]);
    }

    doc.Save(outputPath);
}
