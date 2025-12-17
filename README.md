# FreeRTOS Process Scheduler Simulation

![Language](https://img.shields.io/badge/language-C-blue)
![Platform](https://img.shields.io/badge/platform-FreeRTOS%20%7C%20Linux-green)
![License](https://img.shields.io/badge/license-MIT-orange)

Bu proje, **FreeRTOS** gerçek zamanlı işletim sistemi çekirdeği kullanılarak geliştirilmiş kapsamlı bir **Görev Zamanlayıcı (Process Scheduler)** simülasyonudur.  
Proje, **Çok Seviyeli Geri Beslemeli Kuyruk (MLFQ – Multi-Level Feedback Queue)** algoritmasını ve **Gerçek Zamanlı (Real-Time)** öncelik yönetimini simüle eder.

---

## 🚀 Özellikler

- **Hibrit Zamanlama Algoritması**
  - **Gerçek Zamanlı (RT) Görevler:** En yüksek öncelikte çalışır ve kesilmezler (Priority 0)
  - **Normal Görevler:** Dinamik öncelik yönetimi uygulanır  
    Zaman dilimini (Time Quantum) dolduran görevlerin önceliği düşürülür (Aging / Demotion)

- **Zaman Aşımı (Timeout) Kontrolü**  
  Belirli bir süre (20 sn) kuyrukta bekleyip çalışamayan görevler otomatik olarak sonlandırılır

- **Dosya Tabanlı Giriş**  
  Görev senaryoları `giris.txt` dosyasından dinamik olarak yüklenir

- **Renkli Konsol Çıktısı**  
  Her görev farklı bir renkle temsil edilir, izleme kolaylaşır

- **Thread-Safe Mimari**  
  FreeRTOS **Mutex** yapıları ile güvenli veri paylaşımı sağlanır

---

## 📂 Proje Yapısı

```text
.
├── main.c           # Ana giriş noktası ve dispatcher (dağıtıcı) görevi
├── scheduler.c      # Zamanlayıcı mantığı, kuyruk yönetimi ve algoritmalar
├── scheduler.h      # Veri yapıları, prototipler ve konfigürasyonlar
├── tasks.c          # Görevlerin (iş parçacıklarının) tanımı
├── FreeRTOSConfig.h # FreeRTOS yapılandırma ayarları
└── giris.txt        # Simülasyon senaryo dosyası

⚙️ Nasıl Çalışır? (Algoritma Mantığı)

Simülasyon aşağıdaki kurallara göre işler:

    Görev Yükleme
    Sistem giris.txt dosyasını okur ve görevleri varış zamanlarına göre kuyruğa ekler

    Dispatcher (Dağıtıcı)
    Her 1 saniyelik simülasyon adımında sistem durumu kontrol edilir

        Eğer RT görev varsa işlemci ona verilir

        Aksi halde normal görevler, öncelik sırasına göre seçilir

    Yürütme ve Geri Besleme

        Görev kendisine ayrılan sürede (1 sn) bitmezse önceliği düşürülür

        Kuyruğun sonuna eklenir

        Görev tamamlanırsa sistemden kaldırılır

    Zaman Aşımı (Starvation)

        Bir görev 20 saniye boyunca çalışamazsa sistemden atılır

        Konsola TIMEOUT uyarısı yazdırılır

🛠️ Kurulum ve Derleme
Gereksinimler

    GCC Compiler

    Make (opsiyonel ancak önerilir)

    FreeRTOS POSIX Port

Derleme (Makefile ile)

make

Manuel Derleme

gcc -o scheduler main.c scheduler.c tasks.c \
-I. -I/path/to/freertos/include -lpthread

▶️ Çalıştırma

./scheduler

📄 Giriş Dosyası Formatı (giris.txt)

Her satır aşağıdaki formatta olmalıdır:

VarışZamanı, Öncelik, ÇalışmaSüresi

Örnek

0, 1, 5
2, 0, 3
4, 2, 10

Alan Açıklamaları

    Varış Zamanı: Görevin sisteme giriş zamanı (saniye)

    Öncelik:

        0 → Real-Time

        1 → Yüksek

        2 → Orta

        3 → Düşük

    Çalışma Süresi: Görevin tamamlanması için gereken süre (Burst Time)

📊 Örnek Çıktı

0.0000 sn task1 başladı (id:0000 öncelik:1 kalan süre:5 sn)
1.0000 sn task1 yürütülüyor (id:0000 öncelik:1 kalan süre:4 sn)
2.0000 sn task2 başladı (id:0001 öncelik:0 kalan süre:3 sn) -> RT görev geldi!
2.0000 sn task1 askıda (id:0000 öncelik:1 -> 2)
...

👨‍💻 Katkıda Bulunma

Hata bildirmek veya yeni özellik eklemek için lütfen:

    Issues bölümünü kullanın

    veya Pull Request gönderin

📝 Lisans

Bu proje MIT Lisansı altında sunulmaktadır.
