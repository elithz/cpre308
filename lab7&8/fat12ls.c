/* fat12ls.c
*
*  Displays the files in the root sector of an MSDOS floppy disk
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>

#define SIZE 32      /* size of the read buffer */
#define ROOTSIZE 256 /* max size of the root directory */
#define DIR_ENTRY_SIZE 32 /* size of a directory entry */
#define DIR_NAME_OFFSET 0x00 /* name offset in directory entry */
#define DIR_ATTR_OFFSET 0x0b /* attributes offset in directory entry */
#define DIR_TIME_OFFSET 0x16 /* time offset in directory entry */
#define DIR_DATE_OFFSET 0x18 /* date offset in directory entry */
#define DIR_SIZE_OFFSET 0x1c /* size offset in directory entry */
//define PRINT_HEX   // un-comment this to print the values in hex for debugging

struct BootSector
{
    unsigned char  sName[9];            // The name of the volume
    unsigned short iBytesSector;        // Bytes per Sector

    unsigned char  iSectorsCluster;     // Sectors per Cluster
    unsigned short iReservedSectors;    // Reserved sectors
    unsigned char  iNumberFATs;         // Number of FATs

    unsigned short iRootEntries;        // Number of Root Directory entries
    unsigned short iLogicalSectors;     // Number of logical sectors
    unsigned char  xMediumDescriptor;   // Medium descriptor

    unsigned short iSectorsFAT;         // Sectors per FAT
    unsigned short iSectorsTrack;       // Sectors per Track
    unsigned short iHeads;              // Number of heads

    unsigned short iHiddenSectors;      // Number of hidden sectors
};

void parseDirectory(int iDirOff, int iEntries, unsigned char buffer[]);
//  Pre: Calculated directory offset and number of directory entries
// Post: Prints the filename, time, date, attributes and size of each entry

unsigned short endianSwap(unsigned char one, unsigned char two);
//  Pre: Two initialized characters
// Post: The characters are swapped (two, one) and returned as a short

void decodeBootSector(struct BootSector * pBootS, unsigned char buffer[]);
//  Pre: An initialized BootSector struct and a pointer to an array
//       of characters read from a BootSector
// Post: The BootSector struct is filled out from the buffer data

char * toDOSName(unsigned char string[], unsigned char buffer[], int offset);
//  Pre: String is initialized with at least 12 characters, buffer contains
//       the directory array, offset points to the location of the filename
// Post: fills and returns a string containing the filename in 8.3 format

char * parseAttributes(unsigned char string[], unsigned char buffer[], unsigned char key);
//  Pre: String is initialized with at least five characters, key contains
//       the byte containing the attribue from the directory buffer
// Post: fills the string with the attributes

char * parseTime(unsigned char string[], unsigned char buffer[], unsigned short usTime);
//  Pre: string is initialzied for at least 9 characters, usTime contains
//       the 16 bits used to store time
// Post: string contains the formatted time

char * parseDate(unsigned char string[], unsigned char buffer[], unsigned short usDate);
//  Pre: string is initialized for at least 13 characters, usDate contains
//       the 16 bits used to store the date
// Post: string contains the formatted date

char * parseSize(unsigned char string[], unsigned char buffer[], unsigned short size);
//  Pre: string is initialized for at least 13 characters
// Post: string contains the formatted size

int roundup512(int number);
// Pre: initialized integer
// Post: number rounded up to next increment of 512


// reads the boot sector and root directory
int main(int argc, char * argv[])
{
    int pBootSector = 0;
    unsigned char buffer[SIZE];
    unsigned char rootBuffer[ROOTSIZE * 32];
    struct BootSector sector;
    int iRDOffset = 0;

    // Check for argument
    if (argc < 2) {
    	printf("Specify boot sector\n");
    	exit(1);
    }

    // Open the file and read the boot sector
    pBootSector = open(argv[1], O_RDONLY);
    read(pBootSector, buffer, SIZE);

    // Decode the boot Sector
    decodeBootSector(&sector, buffer);

    // Calculate the location of the root directory
    iRDOffset = (1 + (sector.iSectorsFAT * sector.iNumberFATs) )
                 * sector.iBytesSector;

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
void decodeBootSector(struct BootSector * pBootS, unsigned char buffer[])
{
    int i = 3;  // Skip the first 3 bytes
    unsigned char first;
    unsigned char second;
    unsigned short value;

    // Pull the name and put it in the struct (remember to null-terminate)
    memcpy(pBootS->sName, &(buffer[i]), 8);
    pBootS->sName[9] = '\0';

    // Read bytes/sector and convert to big endian
    i = 11; //offset for bytes/sector
    first = buffer[i++];
    second = buffer[i++];
    pBootS->iBytesSector = endianSwap(first, second);

    // Read sectors/cluster, Reserved sectors and Number of Fats
    first = buffer[i++];
    second = 0;
    pBootS->iSectorsCluster = endianSwap(first, second);

    first = buffer[i++];
    second = buffer[i++];
    pBootS->iReservedSectors = endianSwap(first, second);

    first = buffer[i++];
    second = 0;
    pBootS->iNumberFATs = endianSwap(first, second);

    // Read root entries, logicical sectors and medium descriptor
    first = buffer[i++];
    second = buffer[i++];
    pBootS->iRootEntries = endianSwap(first, second);

    first = buffer[i++];
    second = buffer[i++];
    pBootS->iLogicalSectors = endianSwap(first, second);

    first = buffer[i++];
    second = 0;
    pBootS->xMediumDescriptor = endianSwap(first, second);

    // Read and covert sectors/fat, sectors/track, and number of heads
    first = buffer[i++];
    second = buffer[i++];
    pBootS->iSectorsFAT = endianSwap(first, second);

    first = buffer[i++];
    second = buffer[i++];
    pBootS->iSectorsTrack = endianSwap(first, second);

    first = buffer[i++];
    second = buffer[i++];
    pBootS->iHeads = endianSwap(first, second);

    // Read hidden sectors
    first = buffer[i++];
    second = buffer[i++];
    pBootS->iHiddenSectors = endianSwap(first, second);

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
    for(i = 0; i < (iEntries); i = i + DIR_ENTRY_SIZE)   {
    	if (  buffer[i] != 0x00 && buffer[i] != 0xE5 ) {
    		// Display filename
    		printf("%s\t", toDOSName(string, buffer, i + DIR_NAME_OFFSET)  );
    		// Display Attributes
    		printf("%s\t", parseAttributes(string, buffer, i + DIR_ATTR_OFFSET)  );
    		// Display Time
    		printf("%s\t", parseTime(string, buffer, i + DIR_TIME_OFFSET)  );
    		// Display Date
    		printf("%s\t", parseDate(string, buffer, i + DIR_DATE_OFFSET)  );
    		// Display Size
    		printf("%s\n", parseSize(string, buffer, i + DIR_SIZE_OFFSET) );
    	}
    }

    // Display key
    printf("(R)ead Only (H)idden (S)ystem (A)rchive\n");
} // end parseDirectory()


// Parses the attributes bits of a file
char * parseAttributes(unsigned char string[], unsigned char buffer[], unsigned char key)
{
    // count active attributes
    int attr_count = 0;

    // parse attributes via bitmask
    if(buffer[key] & 0x01)
        string[attr_count++] = 'R';
    if(buffer[key] & 0x02)
        string[attr_count++] = 'H';
    if(buffer[key] & 0x04)
        string[attr_count++] = 'S';
    if(buffer[key] & 0x20)
        string[attr_count++] = 'A';

    // null terminate
    string[attr_count] = '\0';

    return string;
} // end parseAttributes()


// Decodes the bits assigned to the time of each file
char * parseTime(unsigned char string[], unsigned char buffer[], unsigned short usTime)
{
    unsigned char hour = 0x00, min = 0x00, sec = 0x00;

    // use bitmasking and shifting to extract values
    hour = (unsigned char) (buffer[usTime+1] >> 3);
    min = (unsigned char) (((buffer[usTime+1] & 0x07) << 3) | (buffer[usTime] >> 5));
    sec = (unsigned char) (buffer[usTime] & 0x1F);

    sprintf(string, "%02i:%02i:%02i", hour, min, sec);

    return string;


} // end parseTime()


// Decodes the bits assigned to the date of each file
char * parseDate(unsigned char string[], unsigned char buffer[], unsigned short usDate)
{
    unsigned char month = 0x00, day = 0x00;
    unsigned short year = 0x0000;

    year = (unsigned short) (buffer[usDate+1] >> 1);
    year += 1980;


    month = (unsigned char) (((buffer[usDate+1] & 0x01) << 3) | (buffer[usDate] >> 5));
    day = (unsigned char) (buffer[usDate] & 0x1F);

    sprintf(string, "%d/%d/%d", year, month, day);

    return string;

} // end parseDate()

// Decodes the bits assigned to the size of each file
char * parseSize(unsigned char string[], unsigned char buffer[], unsigned short size)
{
    // counter
    int i;
    int j = 3;

    // file size
    unsigned int file_size = 0x0000;

    // fill in size (swap byte order)
    for(i = 0; i < 4; i++)
    {
        string[i] = buffer[j+size];
        j--;
    }

    // null terminate
    string[i] = '\0';

    // convert string to unsigned integer
    file_size = (unsigned int) string[0] << 24 | string[1] << 16 | string[2] << 8 | string[3];

    // write file size into return string
    sprintf(string, "%d bytes", file_size);

    return string;
} // end parseSize()


// Formats a filename string as DOS (adds the dot to 8-dot-3)
char * toDOSName(unsigned char string[], unsigned char buffer[], int offset)
{
    // counter
    int i;

    // loop through and write file name
    for(i = 0; i < 8; i++)
    {
        string[i] = buffer[offset+i];
        string[i+1] = '\0';
        if(!string[i])
        {
            string[i] = '.';
            break;
        }
    }

    for(i = 8; i < 11; i++)
    {
        string[i] = buffer[offset+i];
    }

    string[i] = '\0';

    return string;
} // end toDosNameRead-Only Bit
