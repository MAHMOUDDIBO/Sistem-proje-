#ifndef TARSAU_H
#define TARSAU_H

#define MAX_FILES_LIMIT 32
#define MAX_SIZE_LIMIT (200 * 1024 * 1024) 

typedef struct TarsauApp TarsauApp;


struct TarsauApp {
    
    int max_files;
    long max_total_size;
    
   
    void (*archive)(TarsauApp *self, int argc, char *argv[]);
    void (*extract)(TarsauApp *self, int argc, char *argv[]);
    void (*run)(TarsauApp *self, int argc, char *argv[]);
    
    
    int (*is_text_file)(const char *filename);
    void (*error_exit)(const char *msg);
};


TarsauApp* TarsauApp_create(void);
void TarsauApp_destroy(TarsauApp *app);

#endif 
