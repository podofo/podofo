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
    const PdfEncryptionAlgorithm algorithm, PdfPermissions permissions)
{
    PdfMemDocument doc;
    doc.Load(inputPath);

    PdfKeyLength keyLength;
    PdfVersion version;
    switch (algorithm)
    {
#ifndef PODOFO_HAVE_OPENSSL_NO_RC4
        case PdfEncryptionAlgorithm::RC4V1:
            keyLength = PdfKeyLength::L40;
            version = PdfVersion::V1_3;
            break;
#endif // PODOFO_HAVE_OPENSSL_NO_RC4
        case PdfEncryptionAlgorithm::AESV3R5:;
            keyLength = PdfKeyLength::L256;
            version = PdfVersion::V1_3;
            break;
#ifndef PODOFO_HAVE_OPENSSL_NO_RC4
        case PdfEncryptionAlgorithm::RC4V2:
#endif // PODOFO_HAVE_OPENSSL_NO_RC4
        case PdfEncryptionAlgorithm::AESV2:
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
    printf("       --editnotes   Add or modify text annotations or form fields (if PdfPermissions::Edit is set also allow the creation interactive form fields including signature)\n");
    printf("       --fillandsign Fill in existing form or signature fields\n");
    printf("       --accessible  Extract text and graphics to support user with disabillities\n");
    printf("       --assemble    Assemble the document: insert, create, rotate delete pages or add bookmarks\n");
    printf("       --highprint   Print a high resolution version of the document\n");
    printf("\n\n");
}

void Main(const cspan<string_view>& args)
{
    string_view inputPath;
    string_view outputPath;
    PdfEncryptionAlgorithm algorithm = PdfEncryptionAlgorithm::AESV2;
    PdfPermissions permissions = PdfPermissions::None;
    string userPass;
    string ownerPass;

    if (args.size() < 3)
    {
        print_help();
        exit(-1);
    }

    // Parse the commandline options
    for (unsigned i = 1; i < args.size(); i++)
    {
        if (args[i][0] == '-')
        {
#ifndef PODOFO_HAVE_OPENSSL_NO_RC4
            if (args[i] == "--rc4v1")
                algorithm = PdfEncryptionAlgorithm::RC4V1;
            else if (args[i] == "--rc4v2")
                algorithm = PdfEncryptionAlgorithm::RC4V2;
            else
#endif // PODOFO_HAVE_OPENSSL_NO_RC4
                if (args[i] == "--aesv2")
                    algorithm = PdfEncryptionAlgorithm::AESV2;
                else if (args[i] == "--aesv3")
                    algorithm = PdfEncryptionAlgorithm::AESV3R5;
                else if (args[i] == "-u")
                {
                    i++;
                    if (i < args.size())
                        userPass = args[i];
                    else
                    {
                        fprintf(stderr, "ERROR: -u given on the commandline but no userpassword!\n");
                        exit(-1);
                    }
                }
                else if (args[i] == "-o")
                {
                    i++;
                    if (i < args.size())
                        ownerPass = args[i];
                    else
                    {
                        fprintf(stderr, "ERROR: -o given on the commandline but no ownerpassword!\n");
                        exit(-1);
                    }
                }
                else if (args[i] == "--help")
                {
                    print_help();
                    exit(-1);
                }
                else if (args[i] == "--print")
                    permissions |= PdfPermissions::Print;
                else if (args[i] == "--edit")
                    permissions |= PdfPermissions::Edit;
                else if (args[i] == "--copy")
                    permissions |= PdfPermissions::Copy;
                else if (args[i] == "--editnotes")
                    permissions |= PdfPermissions::EditNotes;
                else if (args[i] == "--fillandsign")
                    permissions |= PdfPermissions::FillAndSign;
                else if (args[i] == "--accessible")
                    permissions |= PdfPermissions::Accessible;
                else if (args[i] == "--assemble")
                    permissions |= PdfPermissions::DocAssembly;
                else if (args[i] == "--highprint")
                    permissions |= PdfPermissions::HighPrint;
                else
                {
                    fprintf(stderr, "WARNING: Do not know what to do with argument: %s\n", args[i].data());
                }
        }
        else
        {
            if (inputPath.empty())
            {
                inputPath = args[i];
            }
            else if (outputPath.empty())
            {
                outputPath = args[i];
            }
            else
            {
                fprintf(stderr, "WARNING: Do not know what to do with argument: %s\n", args[i].data());
            }

        }
    }

    // Check for errors in the commandline options
    if (inputPath.empty())
    {
        fprintf(stderr, "ERROR: No input file specified\n");
        exit(-1);
    }

    if (outputPath.empty())
    {
        fprintf(stderr, "ERROR: No output file specified\n");
        exit(-1);
    }

    if (ownerPass.empty())
    {
        fprintf(stderr, "ERROR: No owner password specified\n");
        exit(-1);
    }


    // Do the actual encryption
    encrypt(inputPath, outputPath, userPass, ownerPass, algorithm, permissions);

    printf("%s was successfully encrypted to: %s\n", inputPath.data(), outputPath.data());
}
