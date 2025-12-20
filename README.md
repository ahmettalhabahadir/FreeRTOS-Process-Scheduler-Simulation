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
    Zaman dilimini dolduran görevlerin önceliği düşürülür (Aging / Demotion)

- **Zaman Aşımı (Timeout) Kontrolü**  
  20 saniye boyunca çalışamayan görevler otomatik olarak sonlandırılır

- **Dosya Tabanlı Giriş**  
  Görevler `giris.txt` dosyasından dinamik olarak okunur

- **Renkli Konsol Çıktısı**  
  Her görev farklı renkle gösterilir

- **Thread-Safe Mimari**  
  FreeRTOS Mutex yapıları kullanılır

---

## 📂 Proje Yapısı

```text
.
FreeRTOS_PC_Scheduler/
│
├── FreeRTOS/
│ ├── include/
│ ├── portable/ThirdParty/GCC/Posix/
│ └── source/
│
├── src/
│ ├── main.c
│ ├── scheduler.c
│ ├── scheduler.h
│ └── tasks.c
│
├── Makefile
├── giris.txt
└── README.md
```

---

## ⚙️ Algoritma Mantığı

1. Sistem `giris.txt` dosyasını okur  
2. Dispatcher her 1 saniyede sistemi kontrol eder  
3. RT görev varsa doğrudan çalıştırılır  
4. Normal görevler öncelik sırasına göre seçilir  
5. Süresi dolmayan görevlerin önceliği düşürülür  
6. 20 saniye çalışamayan görevler TIMEOUT ile sonlandırılır  

---

## 🛠️ Kurulum ve Derleme

### Gereksinimler
- GCC
- Make (önerilir)
- FreeRTOS POSIX Port

### Derleme
```bash
make
```


---

## ▶️ Çalıştırma

```bash
./freertos_sim giris.txt
```

---

## 📄 giris.txt Formatı

```text
VarışZamanı, Öncelik, ÇalışmaSüresi
```

### Örnek
```text
0, 1, 5
2, 0, 3
4, 2, 10
```

- Öncelik 0 → Real-Time  
- Öncelik 1 → Yüksek  
- Öncelik 2 → Orta  
- Öncelik 3 → Düşük  

---

## 📊 Örnek Çıktı

```text
0.0000 sn task1 başladı (id:0000 öncelik:1 kalan süre:5 sn)
1.0000 sn task1 yürütülüyor (id:0000 öncelik:1 kalan süre:4 sn)
2.0000 sn task2 başladı (id:0001 öncelik:0 kalan süre:3 sn) -> RT görev geldi!
2.0000 sn task1 askıda (id:0000 öncelik:1 -> 2)
```

---

