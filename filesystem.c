#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <fcntl.h>

#include "FAT32.h"

typedef struct {
    unsigned char DIR_Name[11];             // offset: 0
    unsigned char DIR_Attributes;           // offset: 11
    unsigned char DIR_NTRes;                // offset: 12
    unsigned char DIR_CreatTimeTenth;       // offset: 13
    unsigned short DIR_CreatTime;           // offset: 14 
    unsigned short DIR_CreatDate;           // offset: 16
    unsigned short DIR_LastAccessDate;      // offset: 18
    unsigned short DIR_FirstClusterHI;      // offset: 20
    unsigned short DIR_WriteTime;           // offset: 22
    unsigned short DIR_WriteDate;           // offset: 24
    unsigned short DIR_FirstClusterLO;      // offset: 26
    unsigned int DIR_FileSize;            // offset: 28
} __attribute__((packed)) DIRENTRY;

typedef struct {
    unsigned int current_cluster;
    DIRENTRY current_dir;
} __attribute__((packed)) ENV_Info;



FILE *  img_file;
BPB_Info BPB;
ENV_Info ENV;

int FirstDataSector;
int FirstFATSector;
// FSInfo FSI;
unsigned char ATTR_READ_ONLY = 0x01;
unsigned char ATTR_HIDDEN = 0x02;
unsigned char ATTR_SYSTEM = 0x04;
unsigned char ATTR_VOLUME_ID = 0x08;
unsigned char ATTR_DIRECTORY = 0x10;
unsigned char ATTR_ARCHIVE = 0x20;

unsigned char ATTR_LONG_NAME;



char* UserInput[5]; //Longest Command is 4 long, so this will give us just enough space for 4 args and End line
 
//Shell Commands
void RunProgram(void);
char* DynStrPushBack(char* dest, char c);
void GetUserInput(void);

//Builtins
int loadBPB(const char *filename);
void print_info(void);
void ls(int curr_cluster, char *dirname);
int file_size(int curr_cluster, char* filename);
void cd(int curr_cluster, char *dirname);

//Cluster Management
int get_first_sector(int cluster_num);
int get_next_cluster(int curr_cluster);
int is_last_cluster(int cluster);

//Helper functions
int find_dirname_cluster(int curr_cluster, char *dirname);



int open_file(char* filename, char* mode);


int main(int argc, const char* argv[]) {
    if(argc != 2) {
        printf("Error, incorrect number of arguments\n");
        return 0;
    }

    const char* filename = argv[1];
    loadBPB(filename);

    FirstDataSector = BPB.BPB_RsvdSecCnt + BPB.BPB_NumFATs * BPB.BPB_FATSz32;
    FirstFATSector = BPB.BPB_RsvdSecCnt;

    ATTR_LONG_NAME = ATTR_READ_ONLY | ATTR_HIDDEN | ATTR_SYSTEM | ATTR_VOLUME_ID;

    ENV.current_cluster = BPB.BPB_RootClus;

    RunProgram();

    return 0;
}

void RunProgram(void) {
    while (1) {
        printf("$ ");
        GetUserInput();

        char *command = UserInput[0];

        if (!strcmp(command, "exit")) {
            //release resources 
            break;
        }
        else if (!strcmp(command, "info")) {
            print_info();
        }
        else if (!strcmp(command, "size")) {
            char *filename = UserInput[1];
            int size = file_size(ENV.current_cluster, filename);
            if(size > 0) {
                printf("The size of the file is %d bytes\n", size);
            }
            //printf("The size of the file is %d bytes\n", get_file_size(filename));
        }   
        else if (!strcmp(command, "ls")) {
            char *dirname = UserInput[1];
            ls(ENV.current_cluster, dirname);  
        }
        else if (!strcmp(command, "cd")) {
            char *dirname = UserInput[1];
            cd(ENV.current_cluster, dirname);
        }
        else if (!strcmp(command, "creat")) {

        }
        else if (!strcmp(command, "mkdir")) {

        }
        else if (!strcmp(command, "mv")) {

        }
        else if (!strcmp(command, "open")) {
            // if (openFile() == -1) {
            //     printf("Error");
            // }

        }
        else if (!strcmp(command, "close")) {

        }
        else if (!strcmp(command, "lseek")) {

        }
        else if (!strcmp(command, "read")) {

        }
        else if (!strcmp(command, "write")) {

        }
        else if (!strcmp(command, "rm")) {

        }
        else if (!strcmp(command, "cp")) {

        }
        else if (!strcmp(command, "rmdir")) {

        }

    }
}

char* DynStrPushBack(char* dest, char c) //TODO: Move to a seperate file with other commonly used functions
{
    size_t total_len = strlen(dest) + 2;
    char* new_str = (char*)calloc(total_len, sizeof(char));
    strcpy(new_str, dest);
    new_str[total_len - 2] = c;
    new_str[total_len - 1] = '\0';
    free(dest);
    return new_str;
}

void GetUserInput(void) 
{
    char temp;
    int i = 0;
    while (i < 5) //Clear Array
    {
        if (UserInput[i] != NULL) {
            free(UserInput[i]);
        }
        UserInput[i] = (char*)calloc(1, sizeof(char));
        UserInput[i][0] = '\0';
        i++;
    }
    unsigned int userInputIndex = 0;
    do {
        temp = fgetc(stdin); //fgetc grabs 1 char at a time and casts it to an int.
        
        if (userInputIndex == 4)
        {
            if (temp != '\"')
            {
                printf("Error: Expected quotes around last argument");
            }
            
            temp = fgetc(stdin);
            while (temp != '\"') //if slash, append to the UserInput index, then keep grabbing
            {
                UserInput[userInputIndex] = DynStrPushBack(UserInput[userInputIndex], temp);
                temp = fgetc(stdin);
            }
            //If not end, grab keep grabbing more chars
            while (temp != '\n' && temp != '\0')
            {
                temp = fgetc(stdin);
            }
        }

        else
        {
            while (temp != ' ' && temp != '\n' && temp != '\0') //If not space, new line or end, Append.
            {
                UserInput[userInputIndex] = DynStrPushBack(UserInput[userInputIndex], temp);
                temp = fgetc(stdin);
            }
        }
        userInputIndex++;
    } while (temp != '\n' && temp != '\0' && userInputIndex < 5);

    int j = userInputIndex;
    while (j < 5) {
        free(UserInput[j]);
        UserInput[j] = calloc(10, sizeof(char));
        strcpy(UserInput[j], "");
        j++;
    }
}

void print_info(void) {
    printf("Bytes per Sector: %d\n", BPB.BPB_BytsPerSec);
	printf("Sectors per Cluster: %d\n", BPB.BPB_SecPerClus);
    printf("Reserved Sector Count: %d\n", BPB.BPB_RsvdSecCnt);
	printf("Number of FATs: %d\n", BPB.BPB_NumFATs);
    printf("Total Sectors: %d\n", BPB.BPB_TotSec32);  
    printf("FAT Size: %d\n", BPB.BPB_FATSz32);
    printf("Root Cluster: %d\n", BPB.BPB_RootClus);
}

int loadBPB(const char *filename) {
    img_file = fopen(filename, "rb+");
    fread(&BPB, sizeof(BPB), 1, img_file);
    return 0;
}

int is_last_cluster(int cluster) {
    if(cluster == 0xFFFFFFFF || cluster == 0x0FFFFFF8 || cluster == 0x0FFFFFFE) {
        return 1;
    }
    return 0;
}

int get_next_cluster(int curr_cluster) {
    int next_cluster = FirstFATSector * BPB.BPB_BytsPerSec + curr_cluster * 4;

    fseek(img_file, next_cluster, SEEK_SET);
    fread(&next_cluster, 4, 1, img_file);

    if(next_cluster == 0xFFFFFFFF || next_cluster == 0x0FFFFFF8 || next_cluster == 0x0FFFFFFE) {
        return -1;
    }
    return next_cluster;
}

void ls(int curr_cluster, char *dirname) {
    DIRENTRY curr_dir;
    int i, offset;

    if(strcmp(dirname, "") == 0) {
        curr_cluster = ENV.current_cluster;
    } else if((curr_cluster = find_dirname_cluster(BPB.BPB_RootClus, dirname)) == -1) {
        return;
    }

    while(!is_last_cluster(curr_cluster)) {
        for(i = 0; i*sizeof(curr_dir) < BPB.BPB_BytsPerSec; i++) {
            offset = get_first_sector(curr_cluster) * BPB.BPB_BytsPerSec + i*sizeof(curr_dir);
            fseek(img_file, offset, SEEK_SET);
            fread(&curr_dir, sizeof(curr_dir), 1, img_file);

            if(curr_dir.DIR_Name[0] != '\0' && !(curr_dir.DIR_Attributes & ATTR_LONG_NAME)) {
                for(int j = 0; j < 11; j++) {
                    if(curr_dir.DIR_Name[j] == ' ') {
                        break;
                    }
                    printf("%c", curr_dir.DIR_Name[j]);
                }
                printf(" ");
            }
        }
        curr_cluster = get_next_cluster(curr_cluster);
        if(curr_dir.DIR_Name[0] != '\0' && !(curr_dir.DIR_Attributes & ATTR_LONG_NAME)) {
            printf("\n");
        }
    }
    printf("\n");
}

void cd(int curr_cluster, char *dirname) {
    int new_cluster;
    if((new_cluster = find_dirname_cluster(curr_cluster, dirname)) == -1) {
        return;
    }
    ENV.current_cluster = new_cluster;

    int offset = get_first_sector(ENV.current_cluster) * BPB.BPB_BytsPerSec;
    fseek(img_file, offset, SEEK_SET);
    fread(&ENV.current_dir, sizeof(ENV.current_dir), 1, img_file);
}

int find_dirname_cluster(int curr_cluster, char * dirname) {
    DIRENTRY curr_dir;
    int i, j, offset;

    while(!is_last_cluster(curr_cluster)) {
        for(i = 0; i*sizeof(curr_dir) < BPB.BPB_BytsPerSec; i++) {
            offset = get_first_sector(curr_cluster) * BPB.BPB_BytsPerSec + i*sizeof(curr_dir);
            fseek(img_file, offset, SEEK_SET);
            fread(&curr_dir, sizeof(curr_dir), 1, img_file);

            if((curr_dir.DIR_Attributes & ATTR_DIRECTORY) && curr_dir.DIR_Name[0] != '\0' && !(curr_dir.DIR_Attributes & ATTR_LONG_NAME)) {
                if(strstr((char *) curr_dir.DIR_Name, dirname) != NULL) {
                    return curr_dir.DIR_FirstClusterHI * 0x100 + curr_dir.DIR_FirstClusterLO;
                }
            }
        }
        curr_cluster = get_next_cluster(curr_cluster);
    }
    printf("Error: directory %s not found\n", dirname);
    return -1;
}

int file_size(int curr_cluster, char *filename) {
    DIRENTRY curr_dir;
    int i, j, offset;

    while(!is_last_cluster(curr_cluster)) {
        for(i = 0; i*sizeof(curr_dir) < BPB.BPB_BytsPerSec; i++) {
            offset = get_first_sector(curr_cluster) * BPB.BPB_BytsPerSec + i*sizeof(curr_dir);
            fseek(img_file, offset, SEEK_SET);
            fread(&curr_dir, sizeof(curr_dir), 1, img_file);

            if(curr_dir.DIR_Name[0] != '\0' && !(curr_dir.DIR_Attributes & ATTR_LONG_NAME)) {
                if(!(curr_dir.DIR_Attributes & ATTR_DIRECTORY) && strstr((char *) curr_dir.DIR_Name, filename) != NULL) {
                    //printf("Size of file %s is %d bytes", filename, curr_dir.DIR_FileSize);
                    return curr_dir.DIR_FileSize;
                } else if(strstr((char *) curr_dir.DIR_Name, filename) != NULL) {
                    printf("Error: %s is a directory\n", filename);
                    return 0;
                }
            }
        }
        curr_cluster = get_next_cluster(curr_cluster);
    }
    printf("Error: file %s not found\n", filename);
    return -1;
}

int get_first_sector(int cluster_num) {
    int offset = (cluster_num - 2) * BPB.BPB_SecPerClus;
    return FirstDataSector + offset;
}



int open_file(char* filename, char* mode) {
    return 0;
}




/*
    DIRENTRY curr_dir;
    int i, j, offset;

    while(!is_last_cluster(curr_cluster)) {
        for(i = 0; i < BPB.BPB_BytsPerSec / sizeof(curr_dir); i++) {
            offset = get_first_sector(curr_cluster) * BPB.BPB_BytsPerSec + i*sizeof(curr_dir);
            fseek(img_file, offset, SEEK_SET);
            fread(&curr_dir, sizeof(curr_dir), 1, img_file);

            if(curr_dir.DIR_Name[0] != '\0') {
                for(j = 0; j < 11; j++) {
                    printf("%c", curr_dir.DIR_Name[j]);
                }
                printf(" ");
            }
        }
        curr_cluster = get_next_cluster(curr_cluster);
        break;
    }
    printf("\n");

    */