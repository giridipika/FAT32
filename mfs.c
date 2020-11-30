/*
    Dipika Giri   ID: 1001440380
    
*/

// The MIT License (MIT)
// 
// Copyright (c) 2020 Trevor Bakker 
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#define _GNU_SOURCE

#include <stdio.h>
#include <unistd.h>
// #include <sys/wait.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>
#include <stdint.h>

#define MAX_NUM_ARGUMENTS 3

#define WHITESPACE " \t\n"      // We want to split our command line up into tokens
                                // so we need to define what delimits our tokens.
                                // In this case  white space
                                // will separate the tokens on our command line

#define MAX_COMMAND_SIZE 255    // The maximum command-line size


FILE *pFile; // pointer to fat32 file

struct DirectoryEntry
{
  char DIR_Name[11];
  uint8_t DIR_Attr;
  uint8_t Unused1[8];
  uint16_t DIR_FirstClusterHigh;
  uint8_t Unused2[4];
  uint16_t DIR_FirstClusterLow;
  uint32_t DIR_FileSize;
};
struct DirectoryEntry dir[16];

// Variables of FAT32
char BS_OEMName[8];      // OEM name
int16_t BPB_BytesPerSec; // bytes per sec
int8_t BPB_SecPerClus;   // sec per cluster
int16_t BPB_RsvdSecCnt;  // number of reserved sectors in reserved region
int8_t BPB_NumFATs;      // number of FAT data structures
int16_t BPB_RootEntCnt;  // number of 32 byte directories in the root 
char BS_VolLab[11];
int32_t BPB_FATSz32;     // number of sectors contained in one FAT
int32_t BPB_RootClus;    // number of the first cluster of RD

int32_t RootDirSectors = 0;
int32_t FirstDataSector = 0;
int32_t FistSectorofCluster = 0; 
 
int32_t CurrentDirectory = 0;


int opened = 0;

// opens FAT32 Image
void open_fat32_image(char* filename)
{
  pFile = fopen(filename, "r");
  if(pFile = NULL)
  {
    printf("Error: File system image not fount.\n");
    return;
  }
  else
  {
    opened = 1;
    fseek(pFile, 3, SEEK_SET);
    fread(&BS_OEMName, 8, 1, pFile);

    fseek(pFile, 11, SEEK_SET);
    fread(&BPB_BytesPerSec, 2, 1, pFile);
    fread(&BPB_SecPerClus, 1, 1, pFile);
    fread(&BPB_RsvdSecCnt, 2, 1, pFile);
    fread(&BPB_NumFATs, 1, 1, pFile);
    fread(&BPB_RootEntCnt, 2, 1, pFile);

    fseek(pFile, 36, SEEK_SET);
    fread(&BPB_FATSz32, 4, 1, pFile);

    fseek(pFile, 44, SEEK_SET);
    fread(&BPB_RootClus, 4, 1, pFile);
    CurrentDirectory = BPB_RootClus;
  }
}

int main()
{

  char * cmd_str = (char*) malloc( MAX_COMMAND_SIZE );

  while( 1 )
  {
    // Print out the mfs prompt
    printf ("mfs> ");

    // Read the command from the commandline.  The
    // maximum command that will be read is MAX_COMMAND_SIZE
    // This while command will wait here until the user
    // inputs something since fgets returns NULL when there
    // is no input
    while( !fgets (cmd_str, MAX_COMMAND_SIZE, stdin) );

    /* Parse input */
    char *token[MAX_NUM_ARGUMENTS];

    int   token_count = 0;                                 
                                                           
    // Pointer to point to the token
    // parsed by strsep
    char *arg_ptr;                                         
                                                           
    char *working_str  = strdup( cmd_str );                

    // we are going to move the working_str pointer so
    // keep track of its original value so we can deallocate
    // the correct amount at the end
    char *working_root = working_str;

    // Tokenize the input stringswith whitespace used as the delimiter
    while ( ( (arg_ptr = strsep(&working_str, WHITESPACE ) ) != NULL) && 
              (token_count<MAX_NUM_ARGUMENTS))
    {
      token[token_count] = strndup( arg_ptr, MAX_COMMAND_SIZE );
      if( strlen( token[token_count] ) == 0 )
      {
        token[token_count] = NULL;
      }
        token_count++;
    }

    // Now print the tokenized input as a debug check
    // \TODO Remove this code and replace with your FAT32 functionality

    // open file
    if(!strcmp(token[0], "open"))
    {
      // OPEN FAT32 IMAGE
      if(!opened)
      {
        open_fat32_image(token[1]); // function to open fat32 image. 
      }
      // FAT32 IMAGE ALREADY OPENED
      else
      {
        printf("Error: File system image already opened.\n");
      }
      continue;
    }
    else if(!strcmp(token[0],"close"))
    {

    }
    else if(!strcmp(token[0], "info"))
    {

    }
    else if(!strcmp(token[0],"stat"))
    {

    }
    else if(!strcmp(token[0], "cd"))
    {

    }
    else
    {
      printf("Entered command is not supported\n");
      continue;
    }
    
    free( working_root );

  }
  return 0;
}
