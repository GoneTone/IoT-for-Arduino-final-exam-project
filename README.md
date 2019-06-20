# IoT for Arduino final exam project
使用 Arduino 開發 IoT 物聯網專題 - 期末考專題

學校課程期末專題限定使用 Arduino Uno R3，所以我想到最後就開發了 IoT 物聯網專案。

## Arduino
### 硬體零件
- Arduino Uno R3
- ESP8266
- DHT22
- LED
- MAX7219 LED Matrix (8 * 8)
- LCD 2004A

### 使用的第三方 Libraries
- [DHT-sensor-library-1.2.3](https://github.com/adafruit/DHT-sensor-library)
- [NewLiquidCrystal_1.5.1](https://bitbucket.org/fmalpartida/new-liquidcrystal/)
- [NewPing_v1.9.1](https://bitbucket.org/teckel12/arduino-new-ping/)
- [WiFiEsp-2.2.2](https://github.com/bportaluri/WiFiEsp)

### `iot.ino` 需填寫的部分
```c
char ssid[] = ""; //WiFi SSID (名稱)
char pass[] = ""; //WiFi 密碼
```

### 照片
![](/images/arduino/01.jpg)
> 硬體架構

## 網頁
### 使用的第三方 Libraries 或資源
- [bootstrap-4.2.1](https://getbootstrap.com/)
- [jquery-3.4.1](https://jquery.com/)
- [fontawesome-5.8.2](https://fontawesome.com/)

### `index.html` 和  `getdata/index.html` 需修改的部分
```javascript
let requestUrl = "http://<您的 Arduino IP>:8080"; //請求網址
```

### 照片
![](/images/webpage/01.jpg)
> IoT Demo 網頁 - 控制頁

![](/images/webpage/02.jpg)
> IoT Demo 網頁 - 讀取資料頁

## Demo 影片
[前往 YouTube 觀看](https://youtu.be/Nbh7ozO356s)

## Licensing information
This project is licensed under GPL-3.0. Please see the [LICENSE](/LICENSE) file for details.