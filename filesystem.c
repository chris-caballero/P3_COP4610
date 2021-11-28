#include <stdio.h>
#include <ctype.h>
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

struct FILETABLE
{
    char name[32];
    char mode[8];
    unsigned int root_cluster;
    unsigned int offset;
    struct FILETABLE* next;
} __attribute__((packed));

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

unsigned int bytes_per_cluster;

char* UserInput[5]; //Longest Command is 4 long, so this will give us just enough space for 4 args and End line
struct FILETABLE* root = NULL;
 
//Shell Commands
void RunProgram(void);
char* DynStrPushBack(char* dest, char c);
void GetUserInput(void);

//Builtins
int loadBPB(const char *filename);
void print_info(void);
void ls(char *dirname);
int file_size(char* filename);
void cd(char *dirname);
//open -> write
int open_file(char* filename, char* mode);
int close_file(char* filename);
void seek_position(char * filename, int offset);
int read_file(char *filename, int size);
// int create_file(char *filename);
// int create_directory(char *dirname);

//Cluster Management
int get_first_sector(int cluster_num);
int get_next_cluster(int curr_cluster);
int is_last_cluster(int cluster);

//struct FILETABLE Management
void FTAdd(const char* fileName, const char* mode, int root_cluster);
void FTRemove(const char* fileName);
int FTIsOpen(const char* fileName);
void FTCleanup();
struct FILETABLE* get_entry_FT(int cluster_num);

//Helper functions
int find_dirname_cluster(char *dirname);
DIRENTRY find_filename_cluster(char *filename) ;
DIRENTRY new_file(char *filename);
DIRENTRY new_directory(char *dirname);
int find_and_allocate_empty_cluster(int last_cluster);
int allocate_cluster(int free_cluster, char *filename);
int find_last_cluster(void);
void create_standard_directories(int curr_cluster);
void format(char * name);
int isFile(int first_cluster);
int isValidMode(char * mode);
int isValidPermissions(int first_cluster, char * mode);


int main(int argc, const char* argv[]) {
    if(argc != 2) {
        printf("Error, incorrect number of arguments\n");
        return 0;
    }

    const char* filename = argv[1];
    loadBPB(filename);

    bytes_per_cluster = BPB.BPB_BytsPerSec * BPB.BPB_SecPerClus;

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

        // cd("blue");
        // cd("blue1");
        // cd("blue2");
        // cd("blue3");
        // cd("blue4");
        // cd("blue5");
        // cd("blue6");
        // cd("blue7");
        // cd("blue8");
        // cd("blue9");
        // cd("blue10");
        // cd("blue11");
        // cd("blue12");
        // cd("blue13");
        // cd("blue14");
        // cd("blue15");
        // cd("blue16");
        // cd("blue17");
        // cd("blue18");
        // cd("blue19");
        // cd("blue20");

        if (!strcmp(command, "exit")) {
            //release resources 
            FTCleanup();
            break;
        }
        else if (!strcmp(command, "info")) {
            print_info();
        }
        else if (!strcmp(command, "size")) {
            char *filename = UserInput[1];
            int size = file_size(filename);
            if(size >= 0) {
                printf("The size of the file is %d bytes\n", size);
            }
            //printf("The size of the file is %d bytes\n", get_file_size(filename));
        }   
        else if (!strcmp(command, "ls")) {
            char *dirname = UserInput[1];
            ls(dirname);  
        }
        else if (!strcmp(command, "cd")) {
            char *dirname = UserInput[1];
            cd(dirname);
        }
        else if (!strcmp(command, "creat")) {
            // char *filename = UserInput[1];
            // create_file(filename);
        }
        else if (!strcmp(command, "mkdir")) {
            // char *dirname = UserInput[1];
            // create_directory(dirname);
        }
        else if (!strcmp(command, "mv")) {

        }
        else if (!strcmp(command, "open")) {
            char *filename = UserInput[1];
            char *mode = UserInput[2];
            open_file(filename, mode);
        }
        else if (!strcmp(command, "close")) {
            char *filename = UserInput[1];
            close_file(filename);
        }
        else if (!strcmp(command, "lseek")) {
            char *filename = UserInput[1];
            int offset = atoi(UserInput[2]);
            seek_position(filename, offset);
        }
        else if (!strcmp(command, "read")) {
            char *filename = UserInput[1];
            int size = atoi(UserInput[2]);
            read_file(filename, size);
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

    return next_cluster;
}

void ls(char *dirname) {
    DIRENTRY curr_dir;
    int i, offset;
    int curr_cluster;

    if(strcmp(dirname, "") == 0) {
        curr_cluster = ENV.current_cluster;
    } else if((curr_cluster = find_dirname_cluster(dirname)) == -1) {
        return;
    }

    while(!is_last_cluster(curr_cluster)) {
        for(i = 0; i*sizeof(curr_dir) < BPB.BPB_BytsPerSec; i++) {
            offset = get_first_sector(curr_cluster) * BPB.BPB_BytsPerSec + i*sizeof(curr_dir);
            fseek(img_file, offset, SEEK_SET);
            fread(&curr_dir, sizeof(curr_dir), 1, img_file);

            if(curr_dir.DIR_Name[0] != '\0' && !(curr_dir.DIR_Attributes & ATTR_LONG_NAME)) {
                //printf("...%d, %d...", curr_dir.DIR_FirstClusterHI,curr_dir.DIR_FirstClusterLO);
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

void cd(char *dirname) {
    int new_cluster;
    if((new_cluster = find_dirname_cluster(dirname)) == -1) {
        return;
    } else if(new_cluster == 0) {
        new_cluster = BPB.BPB_RootClus;
    }
    ENV.current_cluster = new_cluster;
    
    int offset = get_first_sector(ENV.current_cluster) * BPB.BPB_BytsPerSec;
    fseek(img_file, offset, SEEK_SET);
    fread(&ENV.current_dir, sizeof(ENV.current_dir), 1, img_file);
}

DIRENTRY new_file(char *filename) {
    DIRENTRY new_dir;

    for(int i = 0; i < 8; i++) {
        new_dir.DIR_Name[i] = toupper(filename[i]);
    } 
    for(int i = 8; i < 11; i++) {
        new_dir.DIR_Name[i] = ' ';
    }
    new_dir.DIR_Attributes = 0;
    new_dir.DIR_NTRes = 0;
    new_dir.DIR_FileSize = 0;
    new_dir.DIR_FirstClusterHI = 0;
    new_dir.DIR_FirstClusterLO = 0;

    return new_dir;
}

DIRENTRY new_directory(char *dirname) {
    DIRENTRY new_dir = new_file(dirname);

    new_dir.DIR_Attributes = ATTR_DIRECTORY;

    return new_dir;
}

DIRENTRY find_filename_cluster(char *filename) {
    DIRENTRY curr_dir;
    int i, j, offset;
    int curr_cluster = ENV.current_cluster;

    format(filename);

    while(!is_last_cluster(curr_cluster)) {
        for(i = 0; i*sizeof(curr_dir) < BPB.BPB_BytsPerSec; i++) {
            offset = get_first_sector(curr_cluster) * BPB.BPB_BytsPerSec + i*sizeof(curr_dir);
            fseek(img_file, offset, SEEK_SET);
            fread(&curr_dir, sizeof(curr_dir), 1, img_file);

            if(!(curr_dir.DIR_Attributes & ATTR_DIRECTORY) && curr_dir.DIR_Name[0] != '\0' && !(curr_dir.DIR_Attributes & ATTR_LONG_NAME)) {
                //printf("{[%s], [%s]}", (char *) curr_dir.DIR_Name, filename);
                int cont = 0;
                for(int j = 0; j < 11; j++) {
                    if(curr_dir.DIR_Name[j] != filename[j]) {
                        cont = 1;
                        break;
                    }
                }
                if(!cont) {
                    return curr_dir;
                }
            }
        }
        curr_cluster = get_next_cluster(curr_cluster);
    }
    printf("Error: file %s not found\n", filename);
    curr_dir.DIR_Name[0] = 0x0;

    return curr_dir;
}

int find_dirname_cluster(char * dirname) {
    DIRENTRY curr_dir;
    int i, j, offset;
    int curr_cluster = ENV.current_cluster;

    format(dirname);

    while(!is_last_cluster(curr_cluster)) {
        for(i = 0; i*sizeof(curr_dir) < BPB.BPB_BytsPerSec; i++) {
            offset = get_first_sector(curr_cluster) * BPB.BPB_BytsPerSec + i*sizeof(curr_dir);
            fseek(img_file, offset, SEEK_SET);
            fread(&curr_dir, sizeof(curr_dir), 1, img_file);
            
            if((curr_dir.DIR_Attributes & ATTR_DIRECTORY) && curr_dir.DIR_Name[0] != '\0' && !(curr_dir.DIR_Attributes & ATTR_LONG_NAME)) {
                int cont = 0;
                for(int j = 0; j < 11; j++) {
                    if(curr_dir.DIR_Name[j] != dirname[j]) {
                        cont = 1;
                        break;
                    }
                }
                if(!cont) {
                    return curr_dir.DIR_FirstClusterHI * 0x100 + curr_dir.DIR_FirstClusterLO;
                }
            }
        }
        curr_cluster = get_next_cluster(curr_cluster);
    }
    printf("Error: directory %s not found\n", dirname);
    return -1;
}

int file_size(char *filename) {
    DIRENTRY curr_dir;
    int i, j, offset;
    int curr_cluster = ENV.current_cluster;

    format(filename);

    while(!is_last_cluster(curr_cluster)) {
        for(i = 0; i*sizeof(curr_dir) < BPB.BPB_BytsPerSec; i++) {
            offset = get_first_sector(curr_cluster) * BPB.BPB_BytsPerSec + i*sizeof(curr_dir);
            fseek(img_file, offset, SEEK_SET);
            fread(&curr_dir, sizeof(curr_dir), 1, img_file);

            if(curr_dir.DIR_Name[0] != '\0' && !(curr_dir.DIR_Attributes & ATTR_LONG_NAME)) {
                if((curr_dir.DIR_Attributes & ATTR_DIRECTORY) == 0) {
                    int cont = 0;
                    for(int j = 0; j < 11; j++) {
                        if(curr_dir.DIR_Name[j] != filename[j]) {
                            cont = 1;
                            break;
                        }
                    }
                    if(!cont) {
                        return curr_dir.DIR_FileSize;
                    }
                    //printf("Size of file %s is %d bytes", filename, curr_dir.DIR_FileSize);
                    //return curr_dir.DIR_FileSize;
                } else if(strstr((char *) curr_dir.DIR_Name, filename) != NULL) {
                    printf("Error: %s is a directory\n", filename);
                    return -1;
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

void format(char * name) {
    int i;
    for(i = 0; i < strlen(name); i++) {
        name[i] = toupper(name[i]);
    } 
    for(; i < 11; i++) {
        name[i] = ' ';
    }
    name[11] = '\0';
}

int open_file(char* filename, char* mode) {
    int cluster;
    DIRENTRY file = find_filename_cluster(filename);
    cluster = file.DIR_FirstClusterHI * 0x100 + file.DIR_FirstClusterLO;
   //Mode Check
    if(file.DIR_Name[0] == 0x0) {
        printf("ERROR: File not Found\n");
    } else if (!isValidMode(mode)) {
        printf("ERROR: Invalid mode, options are: r w rw wr\n");
    } else if(!isValidPermissions(cluster, mode)) {
        printf("ERROR: Invalid permissions, file %s is read-only\n", filename);
    }
    //Check if File exists
    else if (get_entry_FT(cluster) != NULL)
    {
        printf("ERROR: File already Open\n");
    } else {
        //printf("%d", cluster);
        FTAdd(filename, mode, cluster);
    }

    return 0;
}

int isValidPermissions(int first_cluster, char * mode) {
    DIRENTRY dir;
    int offset = get_first_sector(first_cluster) * BPB.BPB_BytsPerSec;
    fseek(img_file, offset, SEEK_SET);
    fread(&dir, sizeof(dir), 1, img_file);
    if(((dir.DIR_Attributes & ATTR_READ_ONLY) != 0) && 
        (strcmp(mode, "w") == 0 || strcmp(mode, "rw") == 0 || strcmp(mode, "wr") == 0)) {
        return 0;
    }
    return 1;
}

int isValidMode(char * mode) {
    return !(strcmp(mode, "r") != 0 && strcmp(mode, "w") != 0 && 
            strcmp(mode, "rw") != 0 && strcmp(mode, "wr") != 0);
}

int close_file(char* filename)
{
    DIRENTRY file = find_filename_cluster(filename);
    int cluster = file.DIR_FirstClusterHI * 0x100 + file.DIR_FirstClusterLO;
    if (get_entry_FT(cluster) != NULL) 
    {
        FTRemove(filename);
    }
    else
    {
        printf("ERROR: File not open.\n");
        return -1;
    }
    return 0;
}

void FTAdd(const char* fileName, const char* mode, int root_cluster) {
    if (FTIsOpen(fileName)) {
        printf("ERROR: File already open.\n");
    }
    else {
        struct FILETABLE* tmp = calloc(1, sizeof(struct FILETABLE));
        strcpy(tmp->name, fileName);
        strcpy(tmp->mode, mode);
        tmp->offset = 0;
        tmp->next = NULL;
        tmp->root_cluster = root_cluster;
        if (root == NULL) {
            root = tmp;
        }
        else {
            struct FILETABLE* itr = root;
            while (itr->next != NULL) {
                itr = itr->next;
            }
            itr->next = tmp;
        }
    }
}

void FTRemove(const char* fileName) {
    struct FILETABLE* itr1;
    struct FILETABLE* itr2 = NULL;
    for (itr1 = root; itr1 != NULL; itr2 = itr1, itr1 = itr1->next) {
        if (strcmp(itr1->name, fileName) == 0) {
            if (itr2 == NULL) {
                root = itr1->next;
            }
            else {
                itr2->next = itr1->next;
            }
            free(itr1);
            return;
        }
    }
}

int FTIsOpen(const char* fileName) {
    struct FILETABLE* itr;
    for (itr = root; itr != NULL; itr = itr->next) {
        if (strcmp(itr->name, fileName) == 0) {
            return 1;
        }
    }
    return 0;
}

struct FILETABLE* get_entry_FT(int cluster_num) {
    struct FILETABLE* itr;
    for (itr = root; itr != NULL; itr = itr->next) {
        if (itr->root_cluster == cluster_num) {
            return itr;
        }
    }
    return NULL;
}

void FTCleanup() {
    struct FILETABLE* itr1 = root;
    struct FILETABLE* itr2;
    while (itr1 != NULL) {
        itr2 = itr1->next;
        free(itr1);
        itr1 = itr2;
    }
    root = NULL;
}

void seek_position(char * filename, int offset) {
    struct FILETABLE* itr;
    DIRENTRY file = find_filename_cluster(filename);
    int clus = file.DIR_FirstClusterHI * 0x100 + file.DIR_FirstClusterLO;

    if((itr = get_entry_FT(clus)) != NULL) {
        if(offset > file.DIR_FileSize) {
            printf("Error: Offset out of bounds\n");
        } else {
            itr->offset = offset;
        }
    } else {
        printf("%s is not in the open file table", filename);
    }
}

int read_file(char *filename, int size) {
    struct FILETABLE* itr;
    int temp_offset, sz, curr_cluster;
    DIRENTRY file = find_filename_cluster(filename);

    int first_cluster = file.DIR_FirstClusterHI * 0x100 + file.DIR_FirstClusterLO;
    curr_cluster = first_cluster;

    if((itr = get_entry_FT(first_cluster)) != NULL) {
        if(itr->offset + size > file.DIR_FileSize) {
            size = file.DIR_FileSize - itr->offset;
        }

        temp_offset = itr->offset;
        while (temp_offset > bytes_per_cluster) {
            temp_offset = itr->offset - bytes_per_cluster;
        }
        //if overflow we only read to end, otherwise read the entire size
        int position = get_first_sector(curr_cluster) * BPB.BPB_BytsPerSec + itr->offset;
        //printf("position %d", position);
        do {
            sz = (temp_offset + size > bytes_per_cluster) ? bytes_per_cluster - temp_offset : size;
            char buff[sz];

            fseek(img_file, position, SEEK_SET);
            fread(&buff, sizeof(buff), 1, img_file);

            buff[sz] = '\0';
            printf("%s", buff);

            size -= (bytes_per_cluster - temp_offset);
            temp_offset = 0;
            curr_cluster = get_next_cluster(curr_cluster);

            position = get_first_sector(curr_cluster) * BPB.BPB_BytsPerSec;
        } while(size > 0 && !is_last_cluster(curr_cluster));


    } else {
        printf("Error: %s is not in the open file table", filename);
        return -1;
    }
    return 0;
}

int isFile(int first_cluster) {
    DIRENTRY dir;
    int offset = get_first_sector(first_cluster) * BPB.BPB_BytsPerSec;
    fseek(img_file, offset, SEEK_SET);
    fread(&dir, sizeof(dir), 1, img_file);
    if(!(dir.DIR_Attributes & ATTR_DIRECTORY)) {
        return 1;
    }
    return 0;
}

/*
int create_file(char *filename) {
    DIRENTRY curr_dir;
    int i, offset, free;

    DIRENTRY new_dir = new_file(filename);
    int curr_cluster = ENV.current_cluster;

    while(!is_last_cluster(curr_cluster)) {
        for(i = 0; i*sizeof(curr_dir) < BPB.BPB_BytsPerSec; i++) {
            offset = get_first_sector(curr_cluster) * BPB.BPB_BytsPerSec + i*sizeof(curr_dir);
            fseek(img_file, offset, SEEK_SET);
            fread(&curr_dir, sizeof(curr_dir), 1, img_file);

            if(curr_dir.DIR_Name[0] == 0x0 || curr_dir.DIR_Name[0] == 0xE5) {
                // free = find_and_allocate_empty_cluster(curr_cluster);
                fseek(img_file, offset, SEEK_SET);
                fwrite(&new_dir, sizeof(DIRENTRY), 1, img_file);
                return 0;
            }
        }
        curr_cluster = get_next_cluster(curr_cluster);
    }

    if((free = find_and_allocate_empty_cluster(curr_cluster)) == -1) {
        return -1;
    }
    offset = get_first_sector(free) * BPB.BPB_BytsPerSec;
    fseek(img_file, offset, SEEK_SET);
    fwrite(&new_dir, sizeof(DIRENTRY), 1, img_file);;

    return 0;
}

int create_directory(char *dirname) {
    DIRENTRY curr_dir;
    int i, offset, free;

    DIRENTRY new_dir = new_directory(dirname);
    int curr_cluster = ENV.current_cluster;

    while(!is_last_cluster(curr_cluster)) {
        for(i = 0; i*sizeof(curr_dir) < BPB.BPB_BytsPerSec; i++) {
            offset = get_first_sector(curr_cluster) * BPB.BPB_BytsPerSec + i*sizeof(curr_dir);
            fseek(img_file, offset, SEEK_SET);
            fread(&curr_dir, sizeof(curr_dir), 1, img_file);

            if(curr_dir.DIR_Name[0] == 0x0 || curr_dir.DIR_Name[0] == 0xE5) {
                //free is 4B while Lo, Hi are 2B, so we need to split into two halfs [byte 4 - byte 3] [byte 2 - byte 1]
                if((free = find_and_allocate_empty_cluster(curr_cluster)) == -1) {
                    return -1;
                }
                new_dir.DIR_FirstClusterLO = free % 0x100;
                new_dir.DIR_FirstClusterHI = free / 0x100;

                // new_dir.DIR_FirstClusterLO = curr_cluster % 0x100;
                // new_dir.DIR_FirstClusterHI = curr_cluster / 0x100;

                fseek(img_file, offset, SEEK_SET);
                fwrite(&new_dir, sizeof(DIRENTRY), 1, img_file);

                create_standard_directories(free);

                return 0;
            }
        }
        curr_cluster = get_next_cluster(curr_cluster);
    }
    if((free = find_and_allocate_empty_cluster(curr_cluster)) == -1) {
        return -1;
    }

    new_dir.DIR_FirstClusterLO = free % 0x100;
    new_dir.DIR_FirstClusterHI = free / 0x100;

    offset = get_first_sector(free) * BPB.BPB_BytsPerSec;
    fseek(img_file, offset, SEEK_SET);
    fwrite(&new_dir, sizeof(DIRENTRY), 1, img_file);

    create_standard_directories(free);

    return 0;
}

void create_standard_directories(int curr_cluster) {
    //printf("%d", HI * 0x100 + LO);
    int offset;
    DIRENTRY dir = new_directory("..");
    if(ENV.current_cluster != BPB.BPB_RootClus) {
        dir.DIR_FirstClusterHI = ENV.current_dir.DIR_FirstClusterHI;
        dir.DIR_FirstClusterLO = ENV.current_dir.DIR_FirstClusterLO;
    }
    offset = get_first_sector(curr_cluster) * BPB.BPB_BytsPerSec;
    fseek(img_file, offset, SEEK_SET);
    fwrite(&dir, sizeof(DIRENTRY), 1, img_file);

    dir = new_directory(".");
    offset += sizeof(DIRENTRY);
    fseek(img_file, offset, SEEK_SET);
    fwrite(&dir, sizeof(DIRENTRY), 1, img_file);

    char end = 0x0;

    fwrite(&end, 1, 1, img_file);
}

int find_last_cluster(void) {
    int i, j, offset;
    int curr_cluster = BPB.BPB_RootClus;

    while(!is_last_cluster(curr_cluster)) {
        curr_cluster = get_next_cluster(curr_cluster);
    }
    
    return curr_cluster;
}

int find_and_allocate_empty_cluster(int last_cluster) {
    int current_cluster;
    //int last_cluster = find_last_cluster();
    int i = 0;
    int base = FirstFATSector * BPB.BPB_BytsPerSec;
    long position = base + i*4;

    while(position < FirstDataSector * BPB.BPB_BytsPerSec) {
        fseek(img_file, position, SEEK_SET);
        fread(&current_cluster, 4, 1, img_file);
        
        if(current_cluster == 0x0) {
            fseek(img_file, base + last_cluster * 4, SEEK_SET);
            fwrite(&i, 1, 4, img_file);
            int new_value = 0xFFFFFFFF;
            fseek(img_file, position, SEEK_SET);
            fwrite(&new_value, 1, 4, img_file);
            //printf("\n\nHIHIHI\n\n");
            return i;
        }

        i += 1;
        position = base + i*4;
    }

    return -1;
}
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