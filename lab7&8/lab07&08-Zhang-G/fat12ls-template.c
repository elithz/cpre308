/**
*		Filename:  fat12ls-template.c
*    Description:  FAT-12 FS lister
*        Version:  1.0
*        Created:  11.26.2017 22h05min23s
*         Author:  Ningyuan Zhang （狮子劫博丽）(elithz), elithz@iastate.edu 
*        Company:  NERVE Software
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>

#define SIZE 32          /* size of the read buffer */
#define ROOTSIZE 256     /* max size of the root directory */
#define ENTR_OFFSET 32   /* size of a directory entry */
#define NAME_OFFSET 0x00 /* name offset in directory entry */
#define ATBT_OFFSET 0x0b /* attributes offset in directory entry */
#define TIME_OFFSET 0x16 /* time offset in directory entry */
#define DATE_OFFSET 0x18 /* date offset in directory entry */
#define SIZE_OFFSET 0x1c /* size offset in directory entry */
//define PRINT_HEX   // un-comment this to print the values in hex for debugging

struct BootSector
{
    unsigned char sName[9];      // The name of the volume
    unsigned short iBytesSector; // Bytes per Sector

    unsigned char iSectorsCluster;   // Sectors per Cluster
    unsigned short iReservedSectors; // Reserved sectors
    unsigned char iNumberFATs;       // Number of FATs

    unsigned short iRootEntries;     // Number of Root Directory entries
    unsigned short iLogicalSectors;  // Number of logical sectors
    unsigned char xMediumDescriptor; // Medium descriptor

    unsigned short iSectorsFAT;   // Sectors per FAT
    unsigned short iSectorsTrack; // Sectors per Track
    unsigned short iHeads;        // Number of heads

    unsigned short iHiddenSectors; // Number of hidden sectors
};

void parseDirectory(int iDirOff, int iEntries, unsigned char buffer[]);
//  Pre: Calculated directory offset and number of directory entries
// Post: Prints the filename, time, date, attributes and size of each entry

unsigned short endianSwap(unsigned char one, unsigned char two);
//  Pre: Two initialized characters
// Post: The characters are swapped (two, one) and returned as a short

void decodeBootSector(struct BootSector *pBootS, unsigned char buffer[]);
//  Pre: An initialized BootSector struct and a pointer to an array
//       of characters read from a BootSector
// Post: The BootSector struct is filled out from the buffer data

char *toDOSName(unsigned char string[], unsigned char buffer[], int offset);
//  Pre: String is initialized with at least 12 characters, buffer contains
//       the directory array, offset points to the location of the filename
// Post: fills and returns a string containing the filename in 8.3 format

char *parseAttributes(char string[], unsigned char key, unsigned char buffer[]);
//  Pre: String is initialized with at least five characters, key contains
//       the byte containing the attribue from the directory buffer
// Post: fills the string with the attributes

char *parseTime(char string[], unsigned short usTime, unsigned char buffer[]);
//  Pre: string is initialzied for at least 9 characters, usTime contains
//       the 16 bits used to store time
// Post: string contains the formatted time

char *parseDate(char string[], unsigned short usDate, unsigned char buffer[]);
//  Pre: string is initialized for at least 13 characters, usDate contains
//       the 16 bits used to store the date
// Post: string contains the formatted date

//self-made size parser, used to pars size of file
char *parseSize(unsigned char string[], unsigned char buffer[], unsigned short size);

int roundup512(int number);
// Pre: initialized integer
// Post: number rounded up to next increment of 512

// reads the boot sector and root directory
int main(int argc, char *argv[])
{
    int pBootSector = 0;
    unsigned char buffer[SIZE];
    unsigned char rootBuffer[ROOTSIZE * 32];
    struct BootSector sector;
    int iRDOffset = 0;

    // Check for argument
    if (argc < 2)
    {
        printf("Specify boot sector\n");
        exit(1);
    }

    // Open the file and read the boot sector
    pBootSector = open(argv[1], O_RDONLY);
    read(pBootSector, buffer, SIZE);

    // Decode the boot Sector
    decodeBootSector(&sector, buffer);

    // Calculate the location of the root directory
    iRDOffset = (1 + (sector.iSectorsFAT * sector.iNumberFATs)) * sector.iBytesSector;

    // Read the root directory into buffer
    lseek(pBootSector, iRDOffset, SEEK_SET);
    read(pBootSector, rootBuffer, ROOTSIZE);
    close(pBootSector);

    // Parse the root directory
    parseDirectory(iRDOffset, sector.iRootEntries, rootBuffer);

} // end main

// Converts two characters to an unsigned short with two, one
unsigned short endianSwap(unsigned char one, unsigned char two)
{
    // swap
    return (two << 8) | one;
}

// Fills out the BootSector Struct from the buffer
void decodeBootSector(struct BootSector *pBootS, unsigned char buffer[])
{
    int i = 3; // Skip the a 3 bytes
    unsigned char a;
    unsigned char b;

    // Pull the name and put it in the struct (remember to null-terminate)
    memcpy(pBootS->sName, &(buffer[i]), 8);
    pBootS->sName[9] = '\0';

    // Read bytes/sector and convert to big endian
    i = 11; //offset for bytes/sector
    a = buffer[i++];
    b = buffer[i++];
    pBootS->iBytesSector = endianSwap(a, b);

    // Read sectors/cluster, Reserved sectors and Number of Fats
    a = buffer[i++];
    b = 0;
    pBootS->iSectorsCluster = endianSwap(a, b);

    a = buffer[i++];
    b = buffer[i++];
    pBootS->iReservedSectors = endianSwap(a, b);

    a = buffer[i++];
    b = 0;
    pBootS->iNumberFATs = endianSwap(a, b);

    // Read root entries, logicical sectors and medium descriptor
    a = buffer[i++];
    b = buffer[i++];
    pBootS->iRootEntries = endianSwap(a, b);

    a = buffer[i++];
    b = buffer[i++];
    pBootS->iLogicalSectors = endianSwap(a, b);

    a = buffer[i++];
    b = 0;
    pBootS->xMediumDescriptor = endianSwap(a, b);

    // Read and covert sectors/fat, sectors/track, and number of heads
    a = buffer[i++];
    b = buffer[i++];
    pBootS->iSectorsFAT = endianSwap(a, b);

    a = buffer[i++];
    b = buffer[i++];
    pBootS->iSectorsTrack = endianSwap(a, b);

    a = buffer[i++];
    b = buffer[i++];
    pBootS->iHeads = endianSwap(a, b);

    // Read hidden sectors
    a = buffer[i++];
    b = buffer[i++];
    pBootS->iHiddenSectors = endianSwap(a, b);

    return;
}

// iterates through the directory to display filename, time, date,
// attributes and size of each directory entry to the console
void parseDirectory(int iDirOff, int iEntries, unsigned char buffer[])
{
    int i = 0;
    unsigned char string[13];

    // Display table header with labels
    printf("Filename\tAttrib\tTime\t\tDate\t\tSize\n");

    // loop through directory entries to print information for each
    for (i = 0; i < (iEntries); i = i + ENTR_OFFSET)
    {
        if (buffer[i] != 0x00 && buffer[i] != 0xE5)
        {
            // Display filename
            printf("%s\t", toDOSName(string, buffer, i + NAME_OFFSET));
            // Display Attributes
            printf("%s\t", parseAttributes(string, i + ATBT_OFFSET, buffer));
            // Display Time
            printf("%s\t", parseTime(string, i + TIME_OFFSET, buffer));
            // Display Date
            printf("%s\t", parseDate(string, i + DATE_OFFSET, buffer));
            // Display Size
            printf("%s\n", parseSize(string, buffer, i + SIZE_OFFSET));
        }
    }

    // Display key
    printf("(R)ead Only (H)idden (S)ystem (A)rchive\n");
} // end parseDirectory()

// Parses the attributes bits of a file
char *parseAttributes(char string[], unsigned char key, unsigned char buffer[])
{
    //traversal index
    int index = 0;

    //traversal bitmask to find attributes
    if (buffer[key] & 0x01)
        string[index++] = 'R';
    if (buffer[key] & 0x02)
        string[index++] = 'H';
    if (buffer[key] & 0x04)
        string[index++] = 'S';
    if (buffer[key] & 0x20)
        string[index++] = 'A';

    //end string
    string[index] = '\0';

    return string;
} // end parseAttributes()

// Decodes the bits assigned to the time of each file
char *parseTime(char string[], unsigned short usTime, unsigned char buffer[])
{
    unsigned char hour = 0x00, min = 0x00, sec = 0x00;

    // use bitmasking and shifting to extract values
    hour = (unsigned char)(buffer[usTime + 1] >> 3);
    min = (unsigned char)(((buffer[usTime + 1] & 0x07) << 3) | (buffer[usTime] >> 5));
    sec = (unsigned char)(buffer[usTime] & 0x1F);

    sprintf(string, "%02i:%02i:%02i", hour, min, sec);

    return string;

} // end parseTime()

// Decodes the bits assigned to the date of each file
char *parseDate(char string[], unsigned short usDate, unsigned char buffer[])
{
    unsigned char month = 0x00, day = 0x00;
    unsigned short year = 0x0000;

    year = (unsigned short)(buffer[usDate + 1] >> 1);
    year += 1980;

    month = (unsigned char)(((buffer[usDate + 1] & 0x01) << 3) | (buffer[usDate] >> 5));
    day = (unsigned char)(buffer[usDate] & 0x1F);

    sprintf(string, "%d/%d/%d", year, month, day);

    return string;

} // end parseDate()

// Formats a filename string as DOS (adds the dot to 8-dot-3)
char *toDOSName(unsigned char string[], unsigned char buffer[], int offset)
{
    int i;

    //build up filename
    for (i = 0; i < 8; i++)
    {
        string[i] = buffer[offset + i];
        string[i + 1] = '\0';
    }
    //force index 7 to dot
    string[7] = '.';
    for (i = 8; i < 11; i++)
    {
        string[i] = buffer[offset + i];
    }
    //end string
    string[i] = '\0';

    return string;
} // end toDosNameRead-Only Bit

//a self made function to give parsed size of file
char *parseSize(unsigned char string[], unsigned char buffer[], unsigned short size)
{
    int i;
    int j = 3;

    // file size
    unsigned int fileSize = 0x0000;

    for (i = 0; i < 4; i++)
    {
        string[i] = buffer[j + size];
        j--;
    }

    //end string
    string[i] = '\0';

    //string -> int
    fileSize = (unsigned int)string[0] << 24 | string[1] << 16 | string[2] << 8 | string[3];

    //build returner
    sprintf(string, "%d bytes", fileSize);

    return string;
}
