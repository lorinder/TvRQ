/**     @file file_enc_dec.cpp
 *
 *      A tool that can be used to encode/decode a binary file using RaptorQ. InputESIs and
 *      OutputESIs are specified at the command line along with K and symbol size to be used
 *      for the encoding/decoding. Input and output filenames are also specified at the command
 *      line.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <string>
#include <fstream>
#include <sstream>

#include <getopt.h>

#include "rq_api.h"

using namespace std;

/* Returns number of ESIs in input string. Assumes the ESIs are enumerated and not in range format.
 */
int getESICnt (string esiStr) {

    istringstream ss(esiStr);
    int esiCnt = 0;
    string num;
    while (getline(ss, num, ',')) {
        esiCnt++;
    }

    return esiCnt;

}

/* Parses ESI string into an integer array of ESIs.
   Assumes the ESIs are enumerated and not in range format.
*/
int parseESIStr (string esiStr, int esiCnt, uint32_t *esiArr) {

    istringstream ss1(esiStr);
    string num;

    int i = 0;
    while (getline(ss1, num, ',')) {
        esiArr[i] = atoi(num.c_str());
        i++;
    }

    return 0;
}

/* Left trim */
inline std::string& ltrim(std::string& s, const char* t = " \t\n\r\f\v")
{
    s.erase(0, s.find_first_not_of(t));
    return s;
}

/* Right trim */
inline std::string& rtrim(std::string& s, const char* t = " \t\n\r\f\v")
{
    s.erase(s.find_last_not_of(t) + 1);
    return s;
}

/* Left and right trim */
inline std::string& trim(std::string& s, const char* t = " \t\n\r\f\v")
{
    return ltrim(rtrim(s, t), t);
}

/* Expands ESI string (possibly containing ranges) into a comma separated list of all ESIs */
string& expandEsiStr (string& esiStr) {

    istringstream ss1(esiStr);
    string num;
    esiStr = "";

    int i = 0;

    /* Look at all comma separated ESIs or ESI ranges */
    while (getline(ss1, num, ',')) {

        /* Remove whitespace if any */
        num = trim(num);

        /* Check if its a range */
        std::size_t found = num.find_first_of("-");

        /* If it is a range find start and end of range and enumerate the ESIs
           and append to ESI string */
        if (found!=std::string::npos) {

            std::string::size_type sz;
            int startR = std::stoi (num,&sz);
            num = num.substr(sz);
            num.erase(0, num.find_first_not_of("-"));
            int endR = std::stoi(num,&sz);

            for (int i = startR; i <= endR; i++)
                {
                    esiStr.append(to_string(i));
                    esiStr.append(",");
                }
        }
        /* If it is not a range, append ESI to ESI string */
        else {
            esiStr.append(num);
            esiStr.append(",");
        }
        i++;
    }

    /* Remove trailing comma */
    esiStr = rtrim(esiStr, ",");

    return esiStr;
}

/* Function to transcode input and output files using input ESIs and output ESIs specified
   at the command line.
*/
int transcode(int K, int symSize, string inputFname, string outputFname, string inputEsiStr,
              string outputEsiStr)
{
    int status = 0;

    RqInterWorkMem* interWork = NULL;
    RqInterProgram* interProgram = NULL;
    RqOutWorkMem*   outWork = NULL;
    RqOutProgram*   outProgram = NULL;

    /* Open input and output files */
    ifstream ifs(inputFname, ios::binary|ios::ate);
    int length = ifs.tellg();
    ifs.seekg(0, ios::beg);

    ofstream ofs(outputFname, ios::binary);

#define DONE()                                                  \
    do {                                                        \
        if (interProgram)    free(interProgram);                \
        if (interWork)       free(interWork);                   \
        if (outWork)         free(outWork);                     \
        if (outProgram)      free(outProgram);                  \
        ifs.close();                                            \
        ofs.close();                                            \
        if (status == 0) return true; else return false;        \
    } while(0);
#define RUN_NOFAIL(x)   RUN_NOFAIL__INT(x, RUN_NOFAIL__err)
#define RUN_NOFAIL__INT(x,           err)                       \
    do {                                                        \
        int err;                                                \
        if ((err = (x)) != 0) {                                 \
            fprintf(stderr, "Error:%s:%d:  %s failed: %d\n",    \
                    __FILE__, __LINE__, #x, err);               \
            status = err;                                       \
            DONE();                                             \
        }                                                       \
    } while(0)

    if (!ifs ) {
        fprintf(stderr,  "Error:%s:%d: Unable to open input file\n",
                __FILE__, __LINE__);

        status = 1;
        DONE();
    }

    if (!ofs ) {
        fprintf(stderr,  "Error:%s:%d: Unable to open output file\n",
                __FILE__, __LINE__);

        status = 1;
        DONE();
    }

    /* Setup src block */
    int inputEsiCnt = getESICnt(inputEsiStr);
    uint8_t src[inputEsiCnt * symSize];

    uint32_t ESIs_input[inputEsiCnt];
    parseESIStr(inputEsiStr, inputEsiCnt, ESIs_input);

    size_t interWorkSize, interProgSize, iblockSymCnt;

    /* Get various memory sizes from RQ API */
    const int nExtra = inputEsiCnt - K;
    RUN_NOFAIL(RqInterGetMemSizes(K,
                                nExtra,
                                &interWorkSize,
                                &interProgSize,
                                &iblockSymCnt));

    /* Create encoding interProgram */
    interWork = (RqInterWorkMem* )malloc(interWorkSize);
    RUN_NOFAIL(RqInterInit(K, nExtra, interWork, interWorkSize));
    for (int i = 0; i < inputEsiCnt; ++i) {
        RqInterAddIds(interWork, ESIs_input[i], 1);
    }

    interProgram = (RqInterProgram*)malloc(interProgSize);
    RUN_NOFAIL(RqInterCompile(interWork, interProgram, interProgSize));

    /* Setup enc block */
    int outputEsiCnt = getESICnt(outputEsiStr);
    uint32_t ESIs_wanted[outputEsiCnt];

    parseESIStr(outputEsiStr, outputEsiCnt, ESIs_wanted);
    uint8_t enc[outputEsiCnt * symSize];

    /* Create encoding output symbol Program */
    size_t outWorkSize, outProgSize;
    RUN_NOFAIL(RqOutGetMemSizes(outputEsiCnt, &outWorkSize, &outProgSize));
    outWork = (RqOutWorkMem*) malloc(outWorkSize);
    outProgram = (RqOutProgram*) malloc(outProgSize);
    RUN_NOFAIL(RqOutInit(K, outWork, outWorkSize));
    for (int i = 0; i < outputEsiCnt; ++i)
        RUN_NOFAIL(RqOutAddIds(outWork, ESIs_wanted[i], 1));
    RUN_NOFAIL(RqOutCompile(outWork, outProgram, outProgSize));

    /* Iterate through the input file and transcode the data */
    int count =  0;
    while (count < length) {
        for (int i = 0; i < inputEsiCnt * symSize; ++i) {
            src[i] = 0x00;
        }

        ifs.read((char*)src, inputEsiCnt * symSize);
        if (!ifs) {
            fprintf(stderr, "Error: %s:%d: Unable to read enough bytes from file: %ld\n",
                    __FILE__, __LINE__, ifs.gcount());

            status = 1;
            DONE();
        }

        count += inputEsiCnt * symSize;

        /* Create intermediate block */
        uint8_t iblock[iblockSymCnt * symSize];
        RUN_NOFAIL(RqInterExecute(interProgram,         // interProgram
				  symSize,            // symbol size
				  src,                // symbol data
				  sizeof(src),        // sizeof symbol data
				  iblock,             // i-block
				  sizeof(iblock))); // size thereof

        /* Compute encoding/decoding */
        memset(enc, 0, sizeof(enc));
        RUN_NOFAIL(RqOutExecute(outProgram,
                                    symSize,
                                    iblock,
                                    enc,
                                    sizeof(enc)));

        /* End of transcode */

        /* Write out data to output file */
        ofs.write ((char*)(&enc[0]), sizeof(enc));
        if (!ofs.good() ) {
            fprintf(stderr,  "Error:%s:%d: Error while writing to output file\n",
                    __FILE__, __LINE__);

            status = 1;
            DONE();
        }
    }

    DONE();
#undef RUN_NOFAIL__INT
#undef RUN_NOFAIL
#undef DONE
}


static void usage()
{
    puts(       "RQ File Encoder Decoder.\n"
                "\n"
                "   -h          display this help screen and exit\n"
                "   -i <str>    indicates the input filename to use for the operation\n"
                "   -o <str>    indicates the output filename to write output for the operation\n"
                "   -K <value>  the k value to use for encode/decode\n"
                "   -T <value>  the symbol size to use for encode/decode\n"
                "   -I <str>    input ESI string\n"
                "   -O <str>    output ESI string\n"
                );
    exit(0);
}

int main(int argc, char** argv)
{
    int Kval = 5;
    int symSize = 512;

    string inputFname = "input.bin";
    string outputFname = "output.bin";
    string inputEsiStr = "";
    string outputEsiStr = "";


    /* scan command lines */
    int c;
    while ((c = getopt(argc, argv, "hi:T:K:o:I:O:")) != -1) {
        switch (c) {
        case 'h':
            usage();
            break;
        case 'i':
            inputFname = optarg;
            break;
        case 'o':
            outputFname = optarg;
            break;
        case 'I':
            inputEsiStr = optarg;
            break;
        case 'O':
            outputEsiStr = optarg;
            break;
        case 'T':
            symSize = atoi(optarg);
            break;
        case 'K':
            Kval = atoi(optarg);
            break;
        case '?':
            exit(EXIT_FAILURE);
        };
    }

    /* Dump the input arguments */
    printf ("Settings used: \n");
    printf ("K: %d\n", Kval);
    printf ("Symbol size: %d\n", symSize);
    printf ("Input Filename: %s\n", inputFname.c_str());
    printf ("Output Filename: %s\n", outputFname.c_str());
    inputEsiStr = expandEsiStr(inputEsiStr);
    printf ("Input ESI string: %s\n", inputEsiStr.c_str());
    outputEsiStr = expandEsiStr(outputEsiStr);
    printf ("Output ESI string: %s\n", outputEsiStr.c_str());

    /* Encode/Decode */
    int status = 0;

    if (transcode(Kval, symSize, inputFname, outputFname, inputEsiStr, outputEsiStr)) {
        printf("--> Succeeded\n\n");
    } else {
        printf("--> Failed\n\n");
        status = 1;
    }

    return status;
}

// vim:et:ts=4
