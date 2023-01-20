/**
 * SPDX-FileCopyrightText: (C) 2005 Dominik Seichter <domseichter@web.de>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <podofo/podofo.h>

using namespace std;
using namespace PoDoFo;

#include <cstdlib>
#include <cstdio>

void encrypt(const string_view& inputPath, const string_view& outputPath,
    const string_view& userPass, const string_view& ownerPass,
    const PdfEncryptAlgorithm algorithm, PdfPermissions permissions)
{
    PdfMemDocument doc;
    doc.Load(inputPath);

    PdfKeyLength keyLength;
    PdfVersion version;
    switch (algorithm)
    {
#ifndef PODOFO_HAVE_OPENSSL_NO_RC4
        case PdfEncryptAlgorithm::RC4V1:
            keyLength = PdfKeyLength::L40;
            version = PdfVersion::V1_3;
            break;
#endif // PODOFO_HAVE_OPENSSL_NO_RC4
#ifdef PODOFO_HAVE_LIBIDN
        case PdfEncryptAlgorithm::AESV3:;
            keyLength = PdfKeyLength::L256;
            version = PdfVersion::V1_3;
            break;
#endif // PODOFO_HAVE_LIBIDN
#ifndef PODOFO_HAVE_OPENSSL_NO_RC4
        case PdfEncryptAlgorithm::RC4V2:
#endif // PODOFO_HAVE_OPENSSL_NO_RC4
        case PdfEncryptAlgorithm::AESV2:
        default:
            keyLength = PdfKeyLength::L128;
            version = PdfVersion::V1_5;
            break;
    }

    doc.GetMetadata().SetPdfVersion(version);
    doc.SetEncrypted(userPass, ownerPass, permissions, algorithm, keyLength);
    doc.Save(outputPath);
}

void print_help()
{
    printf("Usage: podofoencrypt [--rc4v1] [--rc4v2] [--aesv2] [--aesv3] [-u <userpassword>]\n");
    printf("                     -o <ownerpassword> <inputfile> <outputfile>\n\n");
    printf("       This tool encrypts an existing PDF file.\n\n");
    printf("       --help        Display this help text\n");
    printf(" Algorithm:\n");
    printf("       --rc4v1       Use rc4v1 encryption\n");
    printf("       --rc4v2       Use rc4v2 encryption (Default value)\n");
    printf("       --aesv2       Use aes-128 encryption\n");
    printf("       --aesv3       Use aes-256 encryption\n");
    printf(" Passwords:\n");
    printf("       -u <password> An optional userpassword\n");
    printf("       -o <password> The required owner password\n");
    printf(" Permissions:\n");
    printf("       --print       Allow printing the document\n");
    printf("       --edit        Allow modifying the document besides annotations, form fields or changing pages\n");
    printf("       --copy        Allow text and graphic extraction\n");
    printf("       --editnotes   Add or modify text annoations or form fields (if PdfPermissions::Edit is set also allow the creation interactive form fields including signature)\n");
    printf("       --fillandsign Fill in existing form or signature fields\n");
    printf("       --accessible  Extract text and graphics to support user with disabillities\n");
    printf("       --assemble    Assemble the document: insert, create, rotate delete pages or add bookmarks\n");
    printf("       --highprint   Print a high resolution version of the document\n");
    printf("\n\n");
}

int main(int argc, char* argv[])
{
    const char* inputPath = NULL;
    const char* outputPath = NULL;
    PdfEncryptAlgorithm algorithm = PdfEncryptAlgorithm::AESV2;
    PdfPermissions permissions = PdfPermissions::None;
    string userPass;
    string ownerPass;

    if (argc < 3)
    {
        print_help();
        exit(-1);
    }

    // Parse the commandline options
    for (int i = 1; i < argc; i++)
    {
        if (argv[i][0] == '-')
        {
#ifndef PODOFO_HAVE_OPENSSL_NO_RC4
            if (strcmp(argv[i], "--rc4v1") == 0)
                algorithm = PdfEncryptAlgorithm::RC4V1;
            else if (strcmp(argv[i], "--rc4v2") == 0)
                algorithm = PdfEncryptAlgorithm::RC4V2;
            else
#endif // PODOFO_HAVE_OPENSSL_NO_RC4
                if (strcmp(argv[i], "--aesv2") == 0)
                    algorithm = PdfEncryptAlgorithm::AESV2;
#ifdef PODOFO_HAVE_LIBIDN
                else if (strcmp(argv[i], "--aesv3") == 0)
                    algorithm = PdfEncryptAlgorithm::AESV3;
#endif // PODOFO_HAVE_LIBIDN
                else if (strcmp(argv[i], "-u") == 0)
                {
                    i++;
                    if (i < argc)
                        userPass = argv[i];
                    else
                    {
                        fprintf(stderr, "ERROR: -u given on the commandline but no userpassword!\n");
                        exit(-1);
                    }
                }
                else if (strcmp(argv[i], "-o") == 0)
                {
                    i++;
                    if (i < argc)
                        ownerPass = argv[i];
                    else
                    {
                        fprintf(stderr, "ERROR: -o given on the commandline but no ownerpassword!\n");
                        exit(-1);
                    }
                }
                else if (strcmp(argv[i], "--help") == 0)
                {
                    print_help();
                    exit(-1);
                }
                else if (strcmp(argv[i], "--print") == 0)
                    permissions |= PdfPermissions::Print;
                else if (strcmp(argv[i], "--edit") == 0)
                    permissions |= PdfPermissions::Edit;
                else if (strcmp(argv[i], "--copy") == 0)
                    permissions |= PdfPermissions::Copy;
                else if (strcmp(argv[i], "--editnotes") == 0)
                    permissions |= PdfPermissions::EditNotes;
                else if (strcmp(argv[i], "--fillandsign") == 0)
                    permissions |= PdfPermissions::FillAndSign;
                else if (strcmp(argv[i], "--accessible") == 0)
                    permissions |= PdfPermissions::Accessible;
                else if (strcmp(argv[i], "--assemble") == 0)
                    permissions |= PdfPermissions::DocAssembly;
                else if (strcmp(argv[i], "--highprint") == 0)
                    permissions |= PdfPermissions::HighPrint;
                else
                {
                    fprintf(stderr, "WARNING: Do not know what to do with argument: %s\n", argv[i]);
                }
        }
        else
        {
            if (!inputPath)
            {
                inputPath = argv[i];
            }
            else if (!outputPath)
            {
                outputPath = argv[i];
            }
            else
            {
                fprintf(stderr, "WARNING: Do not know what to do with argument: %s\n", argv[i]);
            }

        }
    }

    // Check for errors in the commandline options
    if (!inputPath)
    {
        fprintf(stderr, "ERROR: No input file specified\n");
        exit(-1);
    }

    if (!outputPath)
    {
        fprintf(stderr, "ERROR: No output file specified\n");
        exit(-1);
    }

    if (!ownerPass.length())
    {
        fprintf(stderr, "ERROR: No owner password specified\n");
        exit(-1);
    }


    // Do the actual encryption
    try
    {
        encrypt(inputPath, outputPath, userPass, ownerPass, algorithm, permissions);
    }
    catch (PdfError& e)
    {
        fprintf(stderr, "Error: An error %i ocurred during encrypting the pdf file.\n", (int)e.GetCode());
        e.PrintErrorMsg();
        return (int)e.GetCode();
    }


    printf("%s was successfully encrypted to: %s\n", inputPath, outputPath);
    return 0;
}
