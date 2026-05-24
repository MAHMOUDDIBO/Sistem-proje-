
#define _CRT_SECURE_NO_WARNINGS 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#ifdef _WIN32
    #include <io.h>
    #include <direct.h>
    #define COMPAT_MKDIR(dir, mode) _mkdir(dir)
    #define COMPAT_CHMOD(path, mode) _chmod(path, mode)
#else
    #include <unistd.h>
    #define COMPAT_MKDIR(dir, mode) mkdir(dir, mode)
    #define COMPAT_CHMOD(path, mode) chmod(path, mode)
#endif

#include "tarsau.h" 
typedef struct {
    char name[256];
    int permissions;
    long size;
} FileData;



static void TarsauApp_error_exit(const char *msg) {
    printf("%s\n", msg);
    exit(1);
}

static int TarsauApp_is_text_file(const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (!file) return 0;
    
    int ch;
    while ((ch = fgetc(file)) != EOF) {
        if (ch == '\0') { 
            fclose(file);
            return 0; 
        }
    }
    fclose(file);
    return 1;
}


static void TarsauApp_archive(TarsauApp *self, int argc, char *argv[]) {
    char *input_files[MAX_FILES_LIMIT];
    int num_files = 0;
    char *output_file = "a.sau";
    
    // Parametre Ayrıştırma
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0) {
            if (i + 1 < argc) {
                output_file = argv[i + 1];
                break;
            } else {
                self->error_exit("Çıktı dosyası belirtilmedi!");
            }
        } else {
            if (num_files >= self->max_files) {
                self->error_exit("En fazla 32 dosya girilebilir.");
            }
            input_files[num_files++] = argv[i];
        }
    }
    
    if (num_files == 0) self->error_exit("Arşivlenecek dosya belirtilmedi.");
    
    long total_size = 0;
    FileData files_info[MAX_FILES_LIMIT];
    
    // Veri toplama ve geçerlilik kontrolleri
    for (int i = 0; i < num_files; i++) {
        struct stat st;
        if (stat(input_files[i], &st) != 0) {
            char err[256];
            sprintf(err, "%s dosyası bulunamadı!", input_files[i]);
            self->error_exit(err);
        }
        
        if (((st.st_mode & S_IFMT) != S_IFREG) || !self->is_text_file(input_files[i])) {
            char err[256];
            sprintf(err, "%s giriş dosyasının formatı uyumsuz!", input_files[i]);
            self->error_exit(err);
        }
        
        total_size += st.st_size;
        
        
        strncpy(files_info[i].name, input_files[i], 255);
        files_info[i].name[255] = '\0';
        files_info[i].permissions = st.st_mode & 0777;
        files_info[i].size = st.st_size;
    }
    
    if (total_size > self->max_total_size) {
        self->error_exit("Giriş dosyalarının toplam boyutu 200 MB'ı geçemez.");
    }
  
    long org_size = 0;
    for (int i = 0; i < num_files; i++) {
        char buf[512];
        sprintf(buf, "%s,%04o,%ld", files_info[i].name, files_info[i].permissions, files_info[i].size);
        org_size += strlen(buf);
        if (i < num_files - 1) org_size += 1; // '|' karakteri için alan ayır
    }
    
    FILE *out = fopen(output_file, "wb");
    if (!out) self->error_exit("Çıktı dosyası oluşturulamadı.");
    
    // Header'ı Yaz
    fprintf(out, "%010ld", org_size);
    for (int i = 0; i < num_files; i++) {
        if (i > 0) fprintf(out, "|");
        fprintf(out, "%s,%04o,%ld", files_info[i].name, files_info[i].permissions, files_info[i].size);
    }
    
    for (int i = 0; i < num_files; i++) {
        FILE *in = fopen(input_files[i], "rb");
        if (!in) continue;
        
        char buffer[4096];
        size_t bytes;
        while ((bytes = fread(buffer, 1, sizeof(buffer), in)) > 0) {
            fwrite(buffer, 1, bytes, out);
        }
        fclose(in);
    }
    
    fclose(out);
    printf("Dosyalar birleştirildi.\n");
}


static void TarsauApp_extract(TarsauApp *self, int argc, char *argv[]) {
    if (argc < 3 || argc > 4) {
        self->error_exit("-a parametresinden sonra en fazla 2 parametre almalıdır.");
    }
    
    char *archive_file = argv[2];
    char *output_dir = NULL;
    
    if (argc == 4) output_dir = argv[3];
    
   
    int len = strlen(archive_file);
    if (len < 4 || strcmp(archive_file + len - 4, ".sau") != 0) {
        self->error_exit("Arşiv dosyası uygunsuz veya bozuk!");
    }
    
    FILE *in = fopen(archive_file, "rb");
    if (!in) self->error_exit("Arşiv dosyası uygunsuz veya bozuk!");
    
   
    char size_buf[11];
    if (fread(size_buf, 1, 10, in) != 10) {
        fclose(in);
        self->error_exit("Arşiv dosyası uygunsuz veya bozuk!");
    }
    size_buf[10] = '\0';
    long org_size = atol(size_buf);
    
    if (org_size <= 0 || org_size > self->max_total_size) {
        fclose(in);
        self->error_exit("Arşiv dosyası uygunsuz veya bozuk!");
    }
    
    char *org_data = malloc(org_size + 1);
    if (!org_data) {
        fclose(in);
        self->error_exit("Bellek tahsis hatası!");
    }
    
    if (fread(org_data, 1, org_size, in) != (size_t)org_size) {
        free(org_data);
        fclose(in);
        self->error_exit("Arşiv dosyası uygunsuz veya bozuk!");
    }
    org_data[org_size] = '\0';
    
    
    if (output_dir) {
        struct stat st = {0};
        if (stat(output_dir, &st) == -1) {
            
            if (COMPAT_MKDIR(output_dir, 0700) != 0) {
                free(org_data);
                fclose(in);
                self->error_exit("Dizin oluşturulamadı.");
            }
        }
    }
    
    char opened_files[4096] = "";
    int file_count = 0;
    
    
    char *record = strtok(org_data, "|");
    
    while (record != NULL) {
        FileData f; 
        if (sscanf(record, "%255[^,],%o,%ld", f.name, &f.permissions, &f.size) != 3) {
            free(org_data);
            fclose(in);
            self->error_exit("Arşiv dosyası uygunsuz veya bozuk!");
        }
        
        char out_path[512];
        if (output_dir) {
            sprintf(out_path, "%s/%s", output_dir, f.name);
        } else {
            strcpy(out_path, f.name);
        }
        
        FILE *out = fopen(out_path, "wb");
        if (!out) {
            free(org_data);
            fclose(in);
            self->error_exit("Dosya oluşturulamadı.");
        }
        
        // Veri Akışı
        char buffer[4096];
        long remaining = f.size;
        while (remaining > 0) {
            size_t to_read = remaining < sizeof(buffer) ? remaining : sizeof(buffer);
            size_t bytes = fread(buffer, 1, to_read, in);
            if (bytes == 0) break;
            fwrite(buffer, 1, bytes, out);
            remaining -= bytes;
        }
        fclose(out);
        
       
        COMPAT_CHMOD(out_path, f.permissions);
        
        if (file_count > 0) strcat(opened_files, "|");
        strcat(opened_files, f.name);
        file_count++;
        
        record = strtok(NULL, "|");
    }
    
    free(org_data);
    fclose(in);
    
   
    if (file_count == 1) {
        if (output_dir) printf("%s dizininde %s dosyası açıldı.\n", output_dir, opened_files);
        else printf("Geçerli dizinde %s dosyası açıldı.\n", opened_files);
    } else if (file_count > 1) {
        char final_str[8192] = "";
        char *token = strtok(opened_files, "|");
        int i = 0;
        while (token != NULL) {
            strcat(final_str, token);
            i++;
            token = strtok(NULL, "|");
            if (token != NULL) {
                if (i == file_count - 1) strcat(final_str, " ve ");
                else strcat(final_str, ", ");
            }
        }
        if (output_dir) printf("%s dizininde %s dosyaları açıldı.\n", output_dir, final_str);
        else printf("Geçerli dizinde %s dosyaları açıldı.\n", final_str);
    }
}


static void TarsauApp_run(TarsauApp *self, int argc, char *argv[]) {
    if (argc < 2) {
        printf("Kullanım:\n");
        printf("  Arşivleme: tarsau -b dosya1 dosya2 ... -o arsiv.sau\n");
        printf("  Açma:      tarsau -a arsiv.sau [dizin]\n");
        exit(1);
    }
    
    if (strcmp(argv[1], "-b") == 0) {
        self->archive(self, argc, argv);
    } else if (strcmp(argv[1], "-a") == 0) {
        self->extract(self, argc, argv);
    } else {
        printf("Geçersiz argüman: %s\n", argv[1]);
        exit(1);
    }
}

TarsauApp* TarsauApp_create() {
    TarsauApp *app = (TarsauApp*)malloc(sizeof(TarsauApp));
    if (!app) {
        printf("Kritik Hata: TarsauApp nesnesi oluşturulamadı (Bellek Hatası)!\n");
        exit(1);
    }
    
   
    app->max_files = MAX_FILES_LIMIT;
    app->max_total_size = MAX_SIZE_LIMIT;
    
    
    app->archive = TarsauApp_archive;
    app->extract = TarsauApp_extract;
    app->run = TarsauApp_run;
    app->is_text_file = TarsauApp_is_text_file;
    app->error_exit = TarsauApp_error_exit;
    
    return app;
}


void TarsauApp_destroy(TarsauApp *app) {
    if (app) {
        free(app);
    }
}
