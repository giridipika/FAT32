/*
    Dipika Giri   ID: 1001440380
    Dhruv Patel   ID: 1001307326
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
#include <sys/wait.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>
#include <stdint.h>

#define MAX_NUM_ARGUMENTS 4
#define NUM_ENTRIES 16

#define WHITESPACE " \t\n" // We want to split our command line up into tokens \
                           // so we need to define what delimits our tokens.   \
                           // In this case  white space                        \
                           // will separate the tokens on our command line

#define MAX_COMMAND_SIZE 255 // The maximum command-line size

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
int32_t BPB_FATSz32;  // number of sectors contained in one FAT
int32_t BPB_RootClus; // number of the first cluster of RD

int32_t RootDirSectors = 0;
int32_t FirstDataSector = 0;
int32_t FistSectorofCluster = 0;

int32_t CurrentDirectory = 0;

int opened = 0;

// Figure out where root dir starts in data region
int FirstSectorofCluster(int32_t sector)
{
  return (((sector - 2) * BPB_BytesPerSec) + (BPB_NumFATs * BPB_FATSz32 * BPB_BytesPerSec) +
          (BPB_RsvdSecCnt * BPB_BytesPerSec));
}

int16_t NextLB(int32_t sector)
{
  uint32_t FATAddress = (BPB_BytesPerSec * BPB_RsvdSecCnt) + (sector * 4);
  int16_t value;
  fseek(pFile, FATAddress, SEEK_SET);
  fread(&value, 2, 1, pFile);
  return value;
}

int compare(char *userString, char *directoryString)
{
  char *dots = "..";
  if (strncmp(dots, userString, 2) == 0)
  {
    if (strncmp(userString, directoryString, 2) == 0)
    {
      return 1;
    }
    return 0;
  }
  char IMG_Name[13];
  strncpy(IMG_Name, directoryString, 12);
  IMG_Name[12] = '\0';
  char input[12];
  memset(input, 0, 12);
  strncpy(input, userString, strlen(userString));
  char expanded_name[12];
  memset(expanded_name, ' ', 12);
  char *token = strtok(input, ".");
  strncpy(expanded_name, token, strlen(token));
  token = strtok(NULL, ".");
  if (token) // If it is a file
  {
    strncpy((char *)(expanded_name + 8), token, strlen(token));
  }
  expanded_name[11] = '\0';
  int i;
  for (i = 0; i < 11; i++)
  {
    expanded_name[i] = toupper(expanded_name[i]);
  }
  if (strncmp(expanded_name, IMG_Name, 11) == 0)
  {
    //printf("They matched\n");
    return 1;
  }
  return 0;
}

void decToHex(int n)
{
  char hex[100];
  int i = 1;
  int j;
  int temp;
  while (n != 0)
  {
    temp = n % 16;
    if (temp < 10)
    {
      temp += 48;
    }
    else
    {
      temp += 55;
    }
    hex[i++] = temp;
    n /= 16;
  }
  printf("0x");
  for (j = i - 1; j > 0; j--)
  {
    printf("%c", hex[j]);
  }
}
// Get function
int get(char *filename, char *newfilename)
{
  FILE *opFile;
  if (newfilename == NULL)
  {
    opFile = fopen(filename, "w");
    if (opFile == NULL)
    {
      printf("%s could not be opened\n", filename);
      perror("ERROR Could not open file\n");
    }
  }
  else
  {
    opFile = fopen(newfilename, "w");
    if (opFile == NULL)
    {
      printf("%s could not be opened\n", newfilename);
      perror("ERROR Could not open file\n");
    }
  }
  int i;
  int found = 0;

  for (i = 0; i < NUM_ENTRIES; i++)
  {
    if (compare(filename, dir[i].DIR_Name))
    {
      int cluster = dir[i].DIR_FirstClusterLow;
      found = 1;

      int bytes_remaining = dir[i].DIR_FileSize;
      int offset = 0;
      unsigned char buffer[512];
      while (bytes_remaining >= BPB_BytesPerSec)
      {
        offset = FirstSectorofCluster(cluster);
        fseek(pFile, offset, SEEK_SET);
        fread(buffer, 1, BPB_BytesPerSec, pFile);

        fwrite(buffer, 1, 512, opFile);
        cluster = NextLB(cluster);
        bytes_remaining = bytes_remaining - BPB_BytesPerSec;
      }
      if (bytes_remaining)
      {
        offset = FirstSectorofCluster(cluster);
        fseek(pFile, offset, SEEK_SET);
        fread(buffer, 1, bytes_remaining, pFile);

        fwrite(buffer, 1, bytes_remaining, opFile);
      }
      fclose(opFile);
    }
  }
  if (!found)
  {
    printf("Error: File Not Found!\n");
    return -1;
  }
  return 0;
}
// Reads from the given file at the position, in bytes, specified by the position parameter and output
// the number of bytes specified.
int read_image(char *dirname, int position, int numofbytes)
{
  int i;
  int found = 0;
  int bytes_remaining = numofbytes;
  if (position < 0)
  {
    printf("Error: Offset can not be less than zero.\n");
  }
  for (i = 0; i < NUM_ENTRIES; i++)
  {
    if (compare(dirname, dir[i].DIR_Name))
    {
      int cluster = dir[i].DIR_FirstClusterLow;
      found = 1;
      int search_size = position;

      while (search_size >= BPB_BytesPerSec)
      {
        cluster = NextLB(cluster);
        search_size = search_size - BPB_BytesPerSec;
      }

      int offset = FirstSectorofCluster(cluster);
      int byteOffset = (position % BPB_BytesPerSec);
      fseek(pFile, offset + byteOffset, SEEK_SET);

      unsigned char buffer[BPB_BytesPerSec];
      int first_block_bytes = BPB_BytesPerSec - position;
      fread(buffer, 1, first_block_bytes, pFile);

      for (i = 0; i < first_block_bytes; i++)
      {
        printf("%x ", buffer[i]);
      }

      bytes_remaining = bytes_remaining - first_block_bytes;

      while (bytes_remaining >= 512)
      {
        cluster = NextLB(cluster);
        offset = FirstSectorofCluster(cluster);
        fseek(pFile, offset, SEEK_SET);
        fread(buffer, 1, BPB_BytesPerSec, pFile);
        for (i = 0; i < first_block_bytes; i++)
        {
          printf("%x ", buffer[i]);
        }
        bytes_remaining = bytes_remaining - BPB_BytesPerSec;
      }
      if (bytes_remaining)
      {
        cluster = NextLB(cluster);
        offset = FirstSectorofCluster(cluster);
        fseek(pFile, offset, SEEK_SET);
        fread(buffer, 1, bytes_remaining, pFile);

        for (i = 0; i < bytes_remaining; i++)
        {
          printf("%x ", buffer[i]);
        }
      }
      printf("\n");
    }
  }
  if (!found)
  {
    printf("Error: File Not Found!\n");
    return -1;
  }
  return 0;
}
int change_directory(char *directoryName)
{
  //loop over the curretn directory and search for the wanted one
  //set the lower cluster number to 2 if its found and its 0
  //now we use LBAToOffset to get offset of directory and then fseek to it
  //then read the directory information in the directory array

  int i;
  int found = 0;
  for (i = 0; i < NUM_ENTRIES; i++)
  {
    if (compare(directoryName, dir[i].DIR_Name))
    {
      int cluster = dir[i].DIR_FirstClusterLow;
      if (cluster == 0)
      {
        cluster = 2;
      }
      int offset = FirstSectorofCluster(cluster);
      fseek(pFile, offset, SEEK_SET);
      fread(dir, sizeof(struct DirectoryEntry), NUM_ENTRIES, pFile);

      found = 1;
      break;
    }
  }
  if (!found)
  {
    printf("Error: Directory not found\n");
    return -1;
  }
  return 0;
}
//Prints the attributes and starting cluster number of the file or directory name
int stat(char *fileName)
{
  int i;
  int found = 0;
  for (i = 0; i < NUM_ENTRIES; i++)
  {
    if (compare(fileName, dir[i].DIR_Name))
    {
      printf("%s Attribute: %d Size: %d Cluster: %d\n", fileName, dir[i].DIR_Attr,
             dir[i].DIR_FileSize, dir[i].DIR_FirstClusterLow);
      found = 1;
      break;
    }
  }
  if (!found)
  {
    printf("Error: File is not found!\n");
  }
  return 0;
}
// Lists the directory contents.
int print_directory()
{
  for (int i = 0; i < 16; i++)
  {
    char direc[12];
    strncpy(direc, dir[i].DIR_Name, 11);
    direc[11] = '\0';
    if ((dir[i].DIR_Name[0] != 0xffffffe5) &&
        (dir[i].DIR_Attr == 0x01 || dir[i].DIR_Attr == 0x10 || dir[i].DIR_Attr == 0x20))
    {
      printf("%s\n", direc);
    }
  }
  return 0;
}
// Displays info about file system in hex and base 10
void show_info()
{
  printf("BPB_BytesPerSec: %d - ", BPB_BytesPerSec);
  decToHex(BPB_BytesPerSec);
  printf("\n");
  printf("BPB_SecPerClus: %d - ", BPB_SecPerClus);
  decToHex(BPB_SecPerClus);
  printf("\n");
  printf("BPB_RsvdSecCnt: %d - ", BPB_RsvdSecCnt);
  decToHex(BPB_RsvdSecCnt);
  printf("\n");
  printf("BPB_NumFATs: %d - ", BPB_NumFATs);
  decToHex(BPB_NumFATs);
  printf("\n");
  printf("BPB_FATSz32: %d - ", BPB_FATSz32);
  decToHex(BPB_FATSz32);
  printf("\n");
}
// Opens FAT32 Image
void open_fat32_image(char *filename)
{
  pFile = fopen(filename, "r");
  if (pFile == NULL)
  {
    printf("Error: File system image not found.\n");
    return;
  }
  else
  {
    opened = 1;
  }
  //fseek(pFile, 3, SEEK_SET);
  //fread(&BS_OEMName, 8, 1, pFile);

  fseek(pFile, 11, SEEK_SET);
  fread(&BPB_BytesPerSec, 1, 2, pFile);
  fseek(pFile, 13, SEEK_SET);
  fread(&BPB_SecPerClus, 1, 1, pFile);
  fseek(pFile, 14, SEEK_SET);
  fread(&BPB_RsvdSecCnt, 1, 2, pFile);
  fseek(pFile, 16, SEEK_SET);
  fread(&BPB_NumFATs, 1, 2, pFile);
  fseek(pFile, 36, SEEK_SET);
  fread(&BPB_FATSz32, 1, 4, pFile);

  //rood directory address
  int rootAddress = (BPB_RsvdSecCnt * BPB_BytesPerSec) + (BPB_NumFATs * BPB_FATSz32 * BPB_BytesPerSec);

  fseek(pFile, rootAddress, SEEK_SET);
  fread(dir, sizeof(struct DirectoryEntry), 16, pFile);

  fread(&BPB_RootEntCnt, 2, 1, pFile);

  fseek(pFile, 44, SEEK_SET);
  fread(&BPB_RootClus, 4, 1, pFile);
  CurrentDirectory = BPB_RootClus;

  int offset = FirstSectorofCluster(CurrentDirectory);
  fseek(pFile, offset, SEEK_SET);
  fread(&dir[0], 32, 16, pFile);
}
int main()
{
  char *cmd_str = (char *)malloc(MAX_COMMAND_SIZE);
  while (1)
  {
    // Print out the mfs prompt
    printf("mfs> ");

    // Read the command from the commandline.  The
    // maximum command that will be read is MAX_COMMAND_SIZE
    // This while command will wait here until the user
    // inputs something since fgets returns NULL when there
    // is no input
    while (!fgets(cmd_str, MAX_COMMAND_SIZE, stdin))
      ;

    /* Parse input */
    char *token[MAX_NUM_ARGUMENTS];

    int token_count = 0;

    // Pointer to point to the token
    // parsed by strsep
    char *arg_ptr;

    char *working_str = strdup(cmd_str);

    // we are going to move the working_str pointer so
    // keep track of its original value so we can deallocate
    // the correct amount at the end
    char *working_root = working_str;

    // Tokenize the input stringswith whitespace used as the delimiter
    while (((arg_ptr = strsep(&working_str, WHITESPACE)) != NULL) &&
           (token_count < MAX_NUM_ARGUMENTS))
    {
      token[token_count] = strndup(arg_ptr, MAX_COMMAND_SIZE);
      if (strlen(token[token_count]) == 0)
      {
        token[token_count] = NULL;
      }
      token_count++;
    }

    // Now print the tokenized input as a debug check
    // \TODO Remove this code and replace with your FAT32 functionality
    if (token[0] == NULL)
    {
      continue;
    }
    // open file
    if (!strcmp(token[0], "open"))
    {
      // OPEN FAT32 IMAGE
      if (!opened)
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
    else if (pFile == NULL)
    {
      printf("Error: File system image mush be opened first.\n");
    }
    else if (!strcmp(token[0], "close"))
    {
      // Close FAT32 image
      // Error: not opened
      if (!opened)
      {
        printf("Error: File System image not opened.\n");
      }
      else
      {
        fclose(pFile);
        opened = 0;
      }
      continue;
    }
    else if (!strcmp(token[0], "bpb"))
    {
      if (!opened)
      {
        printf("Error: File System image not opened.\n");
      }
      else
      {
        show_info();
      }
      continue;
    }
    else if (!strcmp(token[0], "stat"))
    {
      if (!opened)
      {
        printf("Error: File System image not opened.\n");
      }
      else
      {
        if (token[1] == NULL)
        {
          printf("Enter valid argument.\n");
        }
        else
        {
          stat(token[1]);
        }
      }
      continue;
    }
    else if (!strcmp(token[0], "ls"))
    {
      if (!opened)
      {
        printf("Error: File System image not opened.\n");
      }
      else
      {
        print_directory();
      }
      continue;
    }
    else if (!strcmp(token[0], "cd"))
    {
      if (!opened)
      {
        printf("Error: File System image not opened.\n");
      }
      else
      {
        change_directory(token[1]);
      }
      continue;
    }
    else if (!strcmp(token[0], "read"))
    {
      if (!opened)
      {
        printf("Error: File system image not opened.\n");
      }
      else
      {
        read_image(token[1], atoi(token[2]), atoi(token[3]));
      }
      continue;
    }
    else if (!strcmp(token[0], "get"))
    {
      if (!opened)
      {
        printf("Error: File system image not opened.\n");
      }
      else
      {
        get(token[1], token[2]);
      }
      continue;
    }
    else
    {
      printf("Entered command is not supported\n");
      continue;
    }
    free(working_root);
  }
  return 0;
}
