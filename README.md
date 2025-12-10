# CamEsp32

Project for the **AI Thinker ESP32-CAM** with a Spanish web dashboard that serves MJPEG stream, live image controls, and basic chip telemetry. It spins up a server on port 80 with a stylized viewer, brightness/contrast/saturation sliders, AWB/AGC/AEC toggles, 180° rotation, resolution selector, and a client-side enhancement mode (denoise + micro-sharpen).

## Requirements
- PlatformIO with Arduino framework (`platformio.ini` already set for `esp32dev`).
- ESP32-CAM (AI Thinker) with PSRAM enabled and a USB-TTL programmer.
- 2.4 GHz WiFi and a stable 5V supply that can source enough current for streaming.

## Quick setup
1. Edit your credentials in `src/main.cpp`: set `WIFI_SSID`, `WIFI_PASS`, and optionally `HOSTNAME`.
2. Plug the board in programming mode.
3. Build and upload the firmware:
   - `pio run`
   - `pio run -t upload`
4. Open the serial monitor to see diagnostics and the assigned IP: `pio device monitor -b 115200`.
5. Browse to `http://<ip>` to open the HUD; the stream auto-starts and you can tweak parameters live.

## Useful endpoints
- `/` – HTML dashboard with stream, controls, and telemetry.
- `/stream` – Live MJPEG (integrated fallback if the dedicated server fails).
- `/control?var=<field>&val=<value>` – Sensor settings (framesize: UXGA/SXGA/SVGA/VGA or numeric code; quality, brightness, contrast, saturation, aec_value, awb, agc, aec, hmirror, vflip, rotate180).
- `/stats` – JSON with free heap/PSRAM and internal temperature.

## Usage notes
- The camera init tries multiple profiles (resolution/quality/frame buffers) to avoid memory issues on OV3660.
- WiFi reconnects if it drops; sleep is disabled to keep the stream stable.
- The dashboard shows heap/PSRAM bars and a temperature estimate; values depend on the chip and environment.

## License
This project is under the **MIT** license.

## Contact
jorge_villalobos — villa-lobos17@hotmail.com

---

# CamEsp32

Proyecto para la placa **AI Thinker ESP32-CAM** con dashboard web en español que muestra el stream MJPEG, controles de imagen en vivo y telemetría básica del chip. Arranca un servidor en el puerto 80 con un visor estilizado, ajustes de brillo/contraste/saturación, toggles de AWB/AGC/AEC, rotación 180°, selector de resolución y un modo de realce visual (denoise + micro-sharpen) ejecutado en el navegador.

## Requisitos
- PlatformIO con framework Arduino (`platformio.ini` ya configurado para `esp32dev`).
- Placa ESP32-CAM (AI Thinker) con PSRAM habilitada y programador/placa USB-TTL.
- Conexión WiFi 2.4 GHz y fuente estable de 5V que entregue suficiente corriente para el stream.

## Configuración rápida
1. Edita tus credenciales en `src/main.cpp`: ajusta `WIFI_SSID`, `WIFI_PASS` y, si quieres, `HOSTNAME`.
2. Conecta la placa en modo programación.
3. Compila y sube el firmware:
   - `pio run`
   - `pio run -t upload`
4. Abre el monitor serie para ver diagnóstico y la IP asignada: `pio device monitor -b 115200`.
5. Navega a `http://<ip>` para abrir el HUD; el stream inicia automáticamente y puedes cambiar parámetros en tiempo real.

## Endpoints útiles
- `/` – Dashboard HTML con stream, controles y telemetría.
- `/stream` – MJPEG en vivo (fallback integrado si el servidor dedicado no arranca).
- `/control?var=<campo>&val=<valor>` – Ajustes del sensor (framesize: UXGA/SXGA/SVGA/VGA o código numérico, quality, brightness, contrast, saturation, aec_value, awb, agc, aec, hmirror, vflip, rotate180).
- `/stats` – JSON con heap/psram libres y temperatura interna.

## Notas de uso
- El código intenta inicializar la cámara con perfiles escalonados (resolución/quality/frame buffers) para evitar fallos de memoria en el OV3660.
- El WiFi se reconecta si pierde la asociación; se desactiva el sleep para mantener estabilidad en el stream.
- El dashboard muestra barras de heap/PSRAM y estimación de temperatura; los valores dependen del chip y del entorno.

## Licencia
Este proyecto está bajo licencia **MIT**.

## Contacto
jorge_villalobos — villa-lobos17@hotmail.com
