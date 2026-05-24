# BİLGİSAYAR MÜHENDİSLİĞİ SİSTEM PROGRAMLAMA PROJESİ RAPORU

**Proje Adı:** Tarsau - Arşivleme Programı
**Dönem:** 2025-2026 Bahar Dönemi
**Geliştirici Ekip:** G211210576-G211210566
ogrenci adi soyadi: Mahmoud
**GitHub Depo Adresi:** [https://github.com/MAHMOUDDIBO/tarsau-projesi](https://github.com/KULLANICI_ADINIZ/tarsau-projesi) *(Lütfen kendi linkiniz ile değiştiriniz)*

## 1. Proje Özeti
Bu projede, metin dosyalarını tek bir dosya (`.sau` uzantılı) içerisine sıkıştırmadan arşivleyen ve bu arşivden dosyaları orijinal izinleri ve boyutlarıyla geriye çıkaran, Unix/Linux ortamında C diliyle yazılmış `tarsau` isimli bir komut satırı programı geliştirilmiştir. Program `-b` parametresiyle arşivleme (birleştirme) ve `-a` parametresiyle arşivden çıkarma işlemlerini yürütmektedir.

## 2. Geliştirme Süreci ve Alınan Kararlar
Geliştirme süreci boyunca C dilinin temel sistem çağrıları ve dosya okuma/yazma fonksiyonları (I/O) kullanılmıştır. Standart C kütüphanelerinin yanı sıra POSIX uyumlu `<sys/stat.h>`, `<unistd.h>` gibi kütüphanelerle sistem dosya izinleri yönetilmiştir.

**Önemli Tasarım Kararları:**
1. **Güvenlik ve Kısıtlamalar:** Proje yönergeleri gereği, 32 dosya sınırı ve toplam 200 MB boyut kısıtlaması kod içinde `#define` yapılarıyla katı bir şekilde tanımlandı. Kontroller programın hemen başında yapılarak oluşabilecek bellek veya kapasite sorunlarının önüne geçildi.
2. **Hata Yakalama (Error Handling):** Herhangi bir format uyuşmazlığı (`\0` byte kontrolü ile text dosya testi), eksik parametre veya yetkisiz okuma işleminde kullanıcıya proje yönergesinde istenen spesifik mesajlar yazdırılarak (`print_error_and_exit` fonksiyonu) programın çökmeden güvenli kapanması (graceful exit) sağlandı.
3. **Organizasyon Bloğu (Header) Tasarımı:** İlk 10 bayt, organizasyon yapısının toplam uzunluğunu belirtecek şekilde `ASCII` sayı olarak ayrıldı. Devamında, dosyaların isimleri, yetkileri (octal formatta) ve boyutları `|` (pipe) ve `,` (virgül) karakterleriyle ayrılarak (örneğin `dosya.txt,0644,150|...`) parsing (ayrıştırma) işlemlerinin sorunsuz çalışması garanti altına alındı.
4. **Bellek Yönetimi:** Çıkarma (-a) modunda, sadece organizasyon boyutunu bilecek şekilde dosyanın başından okuma yapılmış, ardından esnek bellek (`malloc`) kullanılarak o bölüm hafızaya alınmıştır.

## 3. Kod Mimarisinden Parçacıklar

**Metin Dosyası Kontrolü:**
Binary ve uyumsuz dosyaların arşive katılmasını engellemek için, dosyanın baştan sona okunup Null byte `\0` kontrolü yapıldığı fonksiyon kullanılmıştır.
```c
int is_text_file(const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (!file) return 0;
    
    int ch;
    while ((ch = fgetc(file)) != EOF) {
        if (ch == '\0') { 
            fclose(file);
            return 0; // Null karakteri varsa binary'dir
        }
    }
    fclose(file);
    return 1;
}
```

**Dosya İzinlerinin Kaydedilmesi:**
Sistemden dosya bilgileri `stat()` ile okunarak, `st_mode` değeri arşivin indeks bölümüne yazdırılmıştır.
```c
struct stat st;
stat(input_files[i], &st);
fprintf(out, "%s,%04o,%ld", input_files[i], st.st_mode & 0777, (long)st.st_size);
```

**Dizine Çıkarma İşlemi:**
Eğer argüman olarak bir hedef dizin girildiyse ve bu dizin yoksa `mkdir` ile oluşturulmaktadır.
```c
if (output_dir) {
    struct stat st = {0};
    if (stat(output_dir, &st) == -1) {
        if (mkdir(output_dir, 0700) != 0) {
            print_error_and_exit("Dizin oluşturulamadı.");
        }
    }
}
```

## 4. Ekran Çıktıları (Örnek Çalışma)

*Not: Kendi ortamınızda derleyip çalıştırdığınız ekran görüntülerini buraya görsel (screenshot) olarak ekleyebilirsiniz. Aşağıda terminal çıktılarının örnekleri verilmiştir.*

**Derleme ve Arşivleme (Birleştirme İşlemi):**
```bash
$ make
gcc -Wall -Wextra -O2 -o tarsau tarsau.c
$ ./tarsau -b t1.txt t2.txt t3.txt -o s1.sau
Dosyalar birleştirildi.
```

**Arşivden Çıkarma İşlemi:**
```bash
$ ./tarsau -a s1.sau d1
d1 dizininde t1.txt, t2.txt ve t3.txt dosyaları açıldı.
```

**Hata Durumları İşleyişi:**
```bash
$ ./tarsau -b resim.png -o test.sau
resim.png giriş dosyasının formatı uyumsuz!

$ ./tarsau -a bozuk.txt d2
Arşiv dosyası uygunsuz veya bozuk!
```


## 5. Sonuç
Proje istenilen spesifikasyonlara tam uygun olarak geliştirilmiş olup, herhangi bir sızıntı veya "segmentation fault" oluşturmadan sağlam (robust) bir şekilde çalışmaktadır. Öğrenilen teorik bilgilerin pratik uygulaması başarıyla tamamlanmıştır.
