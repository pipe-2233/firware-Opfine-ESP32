"""
ESP32 Opfine Flasher v1.0
Herramienta gráfica para configurar y flashear el firmware ESP32 Opfine.
Compatible con Windows. Requiere Python 3.8+.

Distribución: empaquetar con build.bat → genera ESP32-Opfine-Flasher.exe
"""

import tkinter as tk
from tkinter import messagebox
import customtkinter as ctk
import serial.tools.list_ports
import threading
import subprocess
import json
import os
import sys
import tempfile
import shutil
import webbrowser
import time
import re
import urllib.request
from pathlib import Path

# URL base de los binarios en GitHub Releases
# Actualizar con la URL real al subir a GitHub
GITHUB_RELEASE_URL = "https://github.com/pipe-2233/firware-Opfine-ESP32/releases/latest/download"
FIRMWARE_FILES = [
    "firmware.bin",
    "bootloader.bin",
    "partitions.bin",
    "boot_app0.bin",
]


# ─────────────────────────────────────────────────────────────────────────────
# Helpers de recursos (compatibles con PyInstaller y modo script)
# ─────────────────────────────────────────────────────────────────────────────

def resource(rel: str) -> str:
    """Devuelve ruta absoluta al recurso, sea .py o .exe empaquetado."""
    base = getattr(sys, "_MEIPASS", Path(__file__).parent)
    return str(Path(base) / rel)


BIN_DIR    = resource("bins")

def _resolve_bin(filename: str) -> str:
    """
    Busca un binario en este orden:
      1. flasher/bins/  (distribución .exe)
      2. ../.pio/build/esp32dev/  (compilación local con PlatformIO)
    """
    # 1. Junto al ejecutable / carpeta bins
    p = Path(BIN_DIR) / filename
    if p.exists():
        return str(p)
    # 2. Salida de PlatformIO relativa al proyecto
    project_root = Path(__file__).parent.parent
    pio_out = project_root / ".pio" / "build" / "esp32dev" / filename
    if pio_out.exists():
        return str(pio_out)
    return str(p)   # devuelve la ruta bins/ aunque no exista (para el mensaje de error)

def _resolve_app0() -> str:
    """boot_app0.bin viene con el framework de Arduino para ESP32."""
    p = Path(BIN_DIR) / "boot_app0.bin"
    if p.exists():
        return str(p)
    pio_pkg = Path.home() / ".platformio" / "packages"
    for fw in pio_pkg.glob("framework-arduinoespressif32*"):
        candidate = fw / "tools" / "partitions" / "boot_app0.bin"
        if candidate.exists():
            return str(candidate)
    return str(p)

FW_BIN   = _resolve_bin("firmware.bin")
BOOT_BIN = _resolve_bin("bootloader.bin")
PART_BIN = _resolve_bin("partitions.bin")
APP0_BIN = _resolve_app0()

# Dirección y tamaño del SPIFFS en la partición del ESP32 (4MB flash, default)
SPIFFS_ADDR = 0x290000
SPIFFS_SIZE = 0x160000   # 1.4 MB


# ─────────────────────────────────────────────────────────────────────────────
# Diálogo para agregar / editar un sensor
# ─────────────────────────────────────────────────────────────────────────────

class SensorDialog(ctk.CTkToplevel):
    TYPES = ["analog", "digital", "i2c", "uart"]
    MODES = ["input", "input_pullup", "input_pulldown"]

    def __init__(self, parent):
        super().__init__(parent)
        self.title("Agregar Sensor")
        self.geometry("420x540")
        self.resizable(False, False)
        self.grab_set()
        self.result = None
        self._build()

    def _build(self):
        ctk.CTkLabel(self, text="Nuevo Sensor",
                     font=ctk.CTkFont(size=16, weight="bold")).pack(pady=(18, 4))

        # ID
        ctk.CTkLabel(self, text="ID del sensor  (ej: temp_01, puerta, gps)").pack(
            anchor="w", padx=20)
        self.id_var = tk.StringVar(value="mi_sensor")
        ctk.CTkEntry(self, textvariable=self.id_var).pack(
            fill="x", padx=20, pady=(2, 10))

        # Tipo
        ctk.CTkLabel(self, text="Tipo de sensor").pack(anchor="w", padx=20)
        self.type_var = tk.StringVar(value="analog")
        ctk.CTkOptionMenu(self, variable=self.type_var, values=self.TYPES,
                          command=self._refresh_fields,
                          width=380).pack(padx=20, pady=(2, 10))

        # Pin GPIO
        ctk.CTkLabel(self, text="Pin GPIO  (ej: 34, 23, 21)").pack(anchor="w", padx=20)
        self.pin_var = tk.StringVar(value="34")
        ctk.CTkEntry(self, textvariable=self.pin_var).pack(
            fill="x", padx=20, pady=(2, 10))

        # ── Campos condicionales ──

        # Digital: modo del pin
        self.mode_frame = ctk.CTkFrame(self, fg_color="transparent")
        ctk.CTkLabel(self.mode_frame, text="Modo del pin").pack(anchor="w")
        self.mode_var = tk.StringVar(value="input_pullup")
        ctk.CTkOptionMenu(self.mode_frame, variable=self.mode_var,
                          values=self.MODES, width=380).pack()

        # I2C: dirección
        self.i2c_frame = ctk.CTkFrame(self, fg_color="transparent")
        ctk.CTkLabel(self.i2c_frame,
                     text="Dirección I2C en decimal  (ej: 118 = 0x76 para BME280)"
                     ).pack(anchor="w")
        self.i2c_addr_var = tk.StringVar(value="118")
        ctk.CTkEntry(self.i2c_frame, textvariable=self.i2c_addr_var, width=380).pack()

        # UART: rx, tx, baudrate
        self.uart_frame = ctk.CTkFrame(self, fg_color="transparent")
        for lbl, attr, default in [
            ("Pin RX", "rx_var", "16"),
            ("Pin TX", "tx_var", "17"),
        ]:
            ctk.CTkLabel(self.uart_frame, text=lbl).pack(anchor="w")
            var = tk.StringVar(value=default)
            setattr(self, attr, var)
            ctk.CTkEntry(self.uart_frame, textvariable=var, width=380).pack(pady=(0, 4))
        ctk.CTkLabel(self.uart_frame, text="Baudrate").pack(anchor="w")
        self.baud_var = tk.StringVar(value="9600")
        ctk.CTkOptionMenu(self.uart_frame, variable=self.baud_var,
                          values=["9600", "19200", "38400", "57600", "115200"],
                          width=380).pack()

        # Sample rate
        ctk.CTkLabel(self, text="Intervalo de lectura (ms)").pack(anchor="w", padx=20)
        self.rate_var = tk.StringVar(value="1000")
        ctk.CTkEntry(self, textvariable=self.rate_var).pack(
            fill="x", padx=20, pady=(2, 10))

        ctk.CTkFrame(self, height=1, fg_color="gray30").pack(fill="x", padx=16, pady=6)

        # Botones
        row = ctk.CTkFrame(self, fg_color="transparent")
        row.pack(pady=6)
        ctk.CTkButton(row, text="Cancelar", width=160, fg_color="gray40",
                      command=self.destroy).pack(side="left", padx=8)
        ctk.CTkButton(row, text="✓  Agregar sensor", width=160,
                      command=self._save).pack(side="left", padx=8)

        self._refresh_fields("analog")

    def _refresh_fields(self, sensor_type: str):
        for frame in (self.mode_frame, self.i2c_frame, self.uart_frame):
            frame.pack_forget()
        if sensor_type == "digital":
            self.mode_frame.pack(fill="x", padx=20, pady=4)
        elif sensor_type == "i2c":
            self.i2c_frame.pack(fill="x", padx=20, pady=4)
        elif sensor_type == "uart":
            self.uart_frame.pack(fill="x", padx=20, pady=4)

    def _save(self):
        try:
            sid  = self.id_var.get().strip()
            pin  = int(self.pin_var.get())
            rate = int(self.rate_var.get())
            assert sid, "El ID no puede estar vacío"
        except (ValueError, AssertionError) as e:
            messagebox.showwarning(
                "Datos inválidos",
                str(e) or "Pin e intervalo deben ser números enteros.",
                parent=self)
            return

        sensor_type = self.type_var.get()
        self.result = {
            "id": sid,
            "type": sensor_type,
            "pin": pin,
            "mode": self.mode_var.get(),
            "sample_rate": rate,
            "enabled": True,
        }
        if sensor_type == "i2c":
            self.result["i2c_address"] = int(self.i2c_addr_var.get())
        if sensor_type == "uart":
            self.result.update({
                "uart_rx": int(self.rx_var.get()),
                "uart_tx": int(self.tx_var.get()),
                "uart_baudrate": int(self.baud_var.get()),
            })
        self.destroy()


# ─────────────────────────────────────────────────────────────────────────────
# Aplicación principal
# ─────────────────────────────────────────────────────────────────────────────

class Flasher(ctk.CTk):

    def __init__(self):
        super().__init__()
        ctk.set_appearance_mode("dark")
        ctk.set_default_color_theme("blue")

        self.title("⚡  ESP32 Opfine Flasher")
        self.geometry("960x700")
        self.minsize(880, 600)

        self.sensors: list = []
        self.found_ip: str = ""
        self.flashing: bool = False

        self._build_ui()
        self._scan_ports()

    # ═══════════════════════════════════════════════════════════════
    # Construcción de la interfaz
    # ═══════════════════════════════════════════════════════════════

    def _build_ui(self):
        self.grid_columnconfigure(0, weight=5)
        self.grid_columnconfigure(1, weight=6)
        self.grid_rowconfigure(0, weight=1)
        self._left_panel()
        self._right_panel()

    # ── Panel izquierdo: configuración ───────────────────────────

    def _left_panel(self):
        left = ctk.CTkScrollableFrame(self, corner_radius=0, fg_color="gray14")
        left.grid(row=0, column=0, sticky="nsew")
        left.grid_columnconfigure(0, weight=1)

        # Título
        ctk.CTkLabel(left, text="⚡  ESP32 Opfine Flasher",
                     font=ctk.CTkFont(size=17, weight="bold")).pack(pady=(16, 2))
        ctk.CTkLabel(left, text="Configura y flashea tu ESP32 sin complicaciones",
                     text_color="gray55",
                     font=ctk.CTkFont(size=11)).pack(pady=(0, 10))

        # ── Puerto COM ──────────────────────────
        self._section(left, "📌  Puerto COM")
        row = ctk.CTkFrame(left, fg_color="transparent")
        row.pack(fill="x", padx=14, pady=4)
        self.port_var = tk.StringVar()
        self.port_menu = ctk.CTkOptionMenu(row, variable=self.port_var,
                                           values=["Buscando..."])
        self.port_menu.pack(side="left", fill="x", expand=True, padx=(0, 8))
        ctk.CTkButton(row, text="🔄  Actualizar", width=110,
                      command=self._scan_ports).pack(side="left")

        # ── WiFi ────────────────────────────────
        self._section(left, "📶  Red WiFi")
        ctk.CTkLabel(
            left,
            text="ℹ️  Puede ser tu red de casa o el hotspot de tu celular.\n"
                 "    Solo necesitas que tu PC y la ESP32 estén en la misma red.\n"
                 "    NO necesita acceso a internet.",
            justify="left",
            text_color="gray55",
            font=ctk.CTkFont(size=10),
        ).pack(anchor="w", padx=16, pady=(0, 8))

        ctk.CTkLabel(left, text="Nombre de la red (SSID)").pack(anchor="w", padx=16)
        self.ssid_var = tk.StringVar()
        ctk.CTkEntry(left, textvariable=self.ssid_var,
                     placeholder_text="Mi_Red_WiFi o nombre de hotspot"
                     ).pack(fill="x", padx=16, pady=(2, 8))

        ctk.CTkLabel(left, text="Contraseña de la red").pack(anchor="w", padx=16)
        pw_row = ctk.CTkFrame(left, fg_color="transparent")
        pw_row.pack(fill="x", padx=16, pady=(2, 8))
        self.pass_var = tk.StringVar()
        self.pw_entry = ctk.CTkEntry(pw_row, textvariable=self.pass_var,
                                     show="•", placeholder_text="Contraseña")
        self.pw_entry.pack(side="left", fill="x", expand=True, padx=(0, 6))
        self._show_pw = False
        ctk.CTkButton(pw_row, text="👁", width=36, fg_color="gray30",
                      hover_color="gray25",
                      command=self._toggle_pw).pack(side="left")

        # ── Sensores ────────────────────────────
        self._section(left, "🔌  Sensores")
        self.sensor_box = ctk.CTkFrame(left, fg_color="gray18", corner_radius=8)
        self.sensor_box.pack(fill="x", padx=14, pady=4)
        self._refresh_sensors()

        ctk.CTkButton(left, text="＋  Agregar sensor",
                      height=32, fg_color="gray30", hover_color="gray25",
                      command=self._add_sensor).pack(fill="x", padx=14, pady=4)

        # ── Protocolo ───────────────────────────
        self._section(left, "📡  Protocolo de comunicación")
        self.proto_var = tk.StringVar(value="mqtt")
        proto_row = ctk.CTkFrame(left, fg_color="transparent")
        proto_row.pack(fill="x", padx=14, pady=4)
        for p in ["mqtt", "tcp", "http"]:
            ctk.CTkRadioButton(proto_row, text=p.upper(),
                               variable=self.proto_var, value=p,
                               command=self._refresh_proto).pack(side="left", padx=12)

        # Frames de cada protocolo (se muestran/ocultan según selección)
        self.f_mqtt = ctk.CTkFrame(left, fg_color="gray18", corner_radius=8)
        self._fe(self.f_mqtt, "Broker MQTT  (IP o hostname)", "mqtt_broker", "192.168.1.100")
        self._fe(self.f_mqtt, "Puerto", "mqtt_port", "1883")
        self._fe(self.f_mqtt, "Topic prefix", "mqtt_topic", "sensores/")
        self._fe(self.f_mqtt, "Usuario MQTT  (dejar vacío si no aplica)", "mqtt_user", "")
        self._fe(self.f_mqtt, "Contraseña MQTT  (dejar vacío si no aplica)", "mqtt_pass", "")

        self.f_tcp = ctk.CTkFrame(left, fg_color="gray18", corner_radius=8)
        self._fe(self.f_tcp, "Servidor TCP  (IP)", "tcp_server", "192.168.1.50")
        self._fe(self.f_tcp, "Puerto TCP", "tcp_port", "5000")

        self.f_http = ctk.CTkFrame(left, fg_color="gray18", corner_radius=8)
        self._fe(self.f_http, "Endpoint  (URL completa)", "http_url",
                 "http://192.168.1.50/api/data")
        self._fe(self.f_http, "Token de autenticación  (opcional)", "http_token", "")

        self._refresh_proto()

        # ── Botones flashear / descargar ────────
        ctk.CTkFrame(left, height=2, fg_color="gray30").pack(
            fill="x", padx=10, pady=14)

        self.dl_btn = ctk.CTkButton(
            left,
            text="📥   Descargar firmware desde GitHub",
            height=38,
            font=ctk.CTkFont(size=12),
            fg_color="#37474f",
            hover_color="#263238",
            command=self._start_download,
        )
        self.dl_btn.pack(fill="x", padx=14, pady=(0, 6))

        self.flash_btn = ctk.CTkButton(
            left,
            text="⚡   FLASHEAR ESP32",
            height=50,
            font=ctk.CTkFont(size=15, weight="bold"),
            fg_color="#1565c0",
            hover_color="#0d47a1",
            command=self._start_flash,
        )
        self.flash_btn.pack(fill="x", padx=14, pady=(0, 20))

    # ── Panel derecho: log y banner de IP ────────────────────────

    def _right_panel(self):
        right = ctk.CTkFrame(self, corner_radius=0, fg_color="gray12")
        right.grid(row=0, column=1, sticky="nsew")
        right.grid_rowconfigure(1, weight=1)
        right.grid_columnconfigure(0, weight=1)

        hdr = ctk.CTkFrame(right, fg_color="transparent")
        hdr.grid(row=0, column=0, sticky="ew", padx=14, pady=(14, 4))
        ctk.CTkLabel(hdr, text="📋  Registro en tiempo real",
                     font=ctk.CTkFont(size=13, weight="bold")).pack(side="left")
        ctk.CTkButton(hdr, text="Limpiar", width=70, height=26,
                      fg_color="gray30", hover_color="gray25",
                      command=self._clear_log).pack(side="right")

        self.log = ctk.CTkTextbox(
            right, wrap="word", state="disabled",
            font=ctk.CTkFont(family="Consolas", size=12),
            fg_color="gray10",
        )
        self.log.grid(row=1, column=0, sticky="nsew", padx=14, pady=(0, 8))

        # Banner de IP (oculto hasta que la ESP32 se conecta)
        self.ip_banner = ctk.CTkFrame(right, fg_color="#1b3a1b", corner_radius=8)
        self.ip_banner.grid(row=2, column=0, sticky="ew", padx=14, pady=(0, 14))
        self.ip_banner.grid_remove()

        self.ip_label = ctk.CTkLabel(
            self.ip_banner, text="",
            font=ctk.CTkFont(size=12, weight="bold"),
            text_color="#66bb6a",
        )
        self.ip_label.pack(side="left", padx=14, pady=10)
        ctk.CTkButton(
            self.ip_banner,
            text="🌐  Abrir en navegador",
            width=165,
            fg_color="#2e7d32",
            hover_color="#1b5e20",
            command=self._open_browser,
        ).pack(side="right", padx=14, pady=8)

    # ═══════════════════════════════════════════════════════════════
    # Helpers de UI
    # ═══════════════════════════════════════════════════════════════

    def _section(self, parent, text: str):
        ctk.CTkLabel(parent, text=text,
                     font=ctk.CTkFont(size=12, weight="bold")).pack(
            anchor="w", padx=14, pady=(12, 2))

    def _fe(self, parent, label: str, attr: str, default: str):
        """Etiqueta + campo de texto dentro de un frame de protocolo."""
        ctk.CTkLabel(parent, text=label,
                     font=ctk.CTkFont(size=11)).pack(
            anchor="w", padx=10, pady=(6, 0))
        var = tk.StringVar(value=default)
        setattr(self, attr, var)
        ctk.CTkEntry(parent, textvariable=var).pack(fill="x", padx=10, pady=(2, 4))

    def _toggle_pw(self):
        self._show_pw = not self._show_pw
        self.pw_entry.configure(show="" if self._show_pw else "•")

    def _refresh_proto(self):
        for f in (self.f_mqtt, self.f_tcp, self.f_http):
            f.pack_forget()
        frames = {"mqtt": self.f_mqtt, "tcp": self.f_tcp, "http": self.f_http}
        frames[self.proto_var.get()].pack(fill="x", padx=14, pady=4)

    def _clear_log(self):
        self.log.configure(state="normal")
        self.log.delete("1.0", "end")
        self.log.configure(state="disabled")

    def _open_browser(self):
        if self.found_ip:
            webbrowser.open(f"http://{self.found_ip}")

    # ═══════════════════════════════════════════════════════════════
    # Descarga de firmware desde GitHub Releases
    # ═══════════════════════════════════════════════════════════════

    def _start_download(self):
        if self.flashing:
            return
        self.dl_btn.configure(state="disabled", text="⏳  Descargando...")
        self._clear_log()
        threading.Thread(target=self._download_worker, daemon=True).start()

    def _download_worker(self):
        bins_dir = Path(__file__).parent / "bins"
        bins_dir.mkdir(exist_ok=True)
        ok = True
        self._log("📥  Descargando firmware desde GitHub Releases...")
        self._log(f"    {GITHUB_RELEASE_URL}")
        self._log("")
        for fname in FIRMWARE_FILES:
            url  = f"{GITHUB_RELEASE_URL}/{fname}"
            dest = bins_dir / fname
            try:
                self._log(f"    ⬇  {fname}...")
                urllib.request.urlretrieve(url, str(dest))
                size_kb = dest.stat().st_size // 1024
                self._log(f"       ✅ {fname}  ({size_kb} KB)")
            except Exception as ex:
                self._log(f"       ❌ Error descargando {fname}: {ex}")
                ok = False
        if ok:
            self._log("\n✅  ¡Firmware descargado correctamente!")
            self._log("    Ahora conecta tu ESP32 y presiona ⚡ FLASHEAR.")
        else:
            self._log("\n⚠️   Algunos archivos no se pudieron descargar.")
            self._log("    Verifica tu conexión a internet e inténtalo de nuevo.")
            self._log(f"    URL base: {GITHUB_RELEASE_URL}")
        self.after(0, lambda: self.dl_btn.configure(
            state="normal", text="📥   Descargar firmware desde GitHub"))

    # ═══════════════════════════════════════════════════════════════
    # Gestión de sensores
    # ═══════════════════════════════════════════════════════════════

    def _add_sensor(self):
        d = SensorDialog(self)
        self.wait_window(d)
        if d.result:
            self.sensors.append(d.result)
            self._refresh_sensors()

    def _refresh_sensors(self):
        for w in self.sensor_box.winfo_children():
            w.destroy()
        if not self.sensors:
            ctk.CTkLabel(
                self.sensor_box,
                text="Sin sensores. Presiona ＋ para agregar.",
                text_color="gray50",
                font=ctk.CTkFont(size=11),
            ).pack(pady=14)
            return
        icons = {"analog": "〰", "digital": "◼", "i2c": "🔗", "uart": "↔"}
        for i, s in enumerate(self.sensors):
            row = ctk.CTkFrame(self.sensor_box, fg_color="gray22", corner_radius=6)
            row.pack(fill="x", padx=6, pady=3)
            ctk.CTkLabel(
                row,
                text=f"  {icons.get(s['type'], '•')}  {s['id']}   "
                     f"{s['type'].upper()} · GPIO{s['pin']} · {s['sample_rate']} ms",
                font=ctk.CTkFont(size=11),
            ).pack(side="left", padx=4, pady=7)
            ctk.CTkButton(
                row, text="✕", width=28, height=24,
                fg_color="gray35", hover_color="#c62828",
                command=lambda idx=i: self._remove_sensor(idx),
            ).pack(side="right", padx=6)

    def _remove_sensor(self, idx: int):
        self.sensors.pop(idx)
        self._refresh_sensors()

    # ═══════════════════════════════════════════════════════════════
    # Puertos COM
    # ═══════════════════════════════════════════════════════════════

    def _scan_ports(self):
        ports = [p.device for p in serial.tools.list_ports.comports()]
        if not ports:
            self.port_menu.configure(values=["— Sin puertos COM —"])
            self.port_var.set("— Sin puertos COM —")
            self._log("🔍  No se encontraron puertos COM. Conecta tu ESP32.")
        else:
            self.port_menu.configure(values=ports)
            self.port_var.set(ports[0])
            self._log(f"🔍  Puertos disponibles: {', '.join(ports)}")

    # ═══════════════════════════════════════════════════════════════
    # Log
    # ═══════════════════════════════════════════════════════════════

    def _log(self, msg: str):
        def _do():
            self.log.configure(state="normal")
            self.log.insert("end", msg + "\n")
            self.log.see("end")
            self.log.configure(state="disabled")
        self.after(0, _do)

    # ═══════════════════════════════════════════════════════════════
    # Construcción del config.json
    # ═══════════════════════════════════════════════════════════════

    def _make_config(self) -> dict:
        p = self.proto_var.get()
        return {
            "system": {
                "device_name": "ESP32-Opfine",
                "core0_frequency": 240,
                "core1_frequency": 240,
            },
            "wifi": {
                "ssid": self.ssid_var.get().strip(),
                "password": self.pass_var.get(),
                "ap_mode": False,
                "ap_ssid": "ESP32-Config",
                "ap_password": "config123",
            },
            "sensors": self.sensors,
            "communication": {
                "protocol": p,
                "mqtt": {
                    "broker": self.mqtt_broker.get().strip(),
                    "port": int(self.mqtt_port.get() or 1883),
                    "user": self.mqtt_user.get().strip(),
                    "password": self.mqtt_pass.get(),
                    "topic_prefix": self.mqtt_topic.get().strip(),
                    "client_id": "ESP32-Opfine",
                },
                "tcp": {
                    "server": self.tcp_server.get().strip(),
                    "port": int(self.tcp_port.get() or 5000),
                    "use_ssl": False,
                },
                "http": {
                    "endpoint": self.http_url.get().strip(),
                    "method": "POST",
                    "auth_token": self.http_token.get().strip(),
                },
            },
        }

    # ═══════════════════════════════════════════════════════════════
    # Flasheo — hilo principal
    # ═══════════════════════════════════════════════════════════════

    def _start_flash(self):
        if self.flashing:
            return

        port = self.port_var.get()
        if "—" in port or not port:
            messagebox.showwarning(
                "Sin puerto",
                "Conecta tu ESP32 por USB y pulsa 🔄 Actualizar.",
                parent=self)
            return

        if not self.ssid_var.get().strip():
            messagebox.showwarning(
                "WiFi requerido",
                "Ingresa el nombre (SSID) de tu red WiFi o hotspot.",
                parent=self)
            return

        self.flashing = True
        self.flash_btn.configure(state="disabled", text="⏳  Flasheando...")
        self.ip_banner.grid_remove()
        self.found_ip = ""
        self._clear_log()
        threading.Thread(target=self._flash_worker, args=(port,), daemon=True).start()

    def _flash_worker(self, port: str):
        tmpdir = tempfile.mkdtemp()
        try:
            self._log("═" * 46)
            self._log("⚡  Iniciando proceso de flasheo")
            self._log("═" * 46)

            # ── 1. Generar config.json ──────────────
            self._log("\n📝  Generando configuración...")
            cfg  = self._make_config()
            data_dir = Path(tmpdir) / "data"
            data_dir.mkdir()
            (data_dir / "config.json").write_text(json.dumps(cfg, indent=2))
            spiffs_bin = str(Path(tmpdir) / "spiffs.bin")

            self._log(f"    SSID      : {cfg['wifi']['ssid']}")
            self._log(f"    Sensores  : {len(self.sensors)}")
            self._log(f"    Protocolo : {cfg['communication']['protocol'].upper()}")

            # ── 2. Imagen SPIFFS ─────────────────────
            self._log("\n📦  Creando imagen SPIFFS (sistema de archivos)...")
            mkspiffs = self._find_mkspiffs()
            r = subprocess.run(
                [mkspiffs, "-c", str(data_dir),
                 "-b", "4096", "-p", "256",
                 "-s", str(SPIFFS_SIZE), spiffs_bin],
                capture_output=True, text=True,
            )
            if r.returncode != 0:
                raise RuntimeError(f"mkspiffs falló:\n{r.stderr.strip()}")
            self._log("    ✅ Imagen SPIFFS lista")

            # ── 3. Validar binarios del firmware ─────
            fw_bin   = _resolve_bin("firmware.bin")
            boot_bin = _resolve_bin("bootloader.bin")
            part_bin = _resolve_bin("partitions.bin")
            app0_bin = _resolve_app0()

            missing = []
            for label, path in [
                ("Bootloader",  boot_bin),
                ("Particiones", part_bin),
                ("boot_app0",   app0_bin),
                ("Firmware",    fw_bin),
            ]:
                if not Path(path).exists():
                    missing.append(label)

            if missing:
                raise FileNotFoundError(
                    f"Faltan archivos compilados: {', '.join(missing)}\n\n"
                    "Ejecuta 'pio run' en el proyecto para compilar el firmware,\n"
                    "luego vuelve a presionar ⚡ FLASHEAR."
                )
            self._log(f"    ✅ Binarios del firmware encontrados")

            # ── 4. Flashear con esptool ───────────────
            self._log(f"\n⬆️   Flasheando en {port}...")
            self._log("─" * 46)
            self._log("  👆  IMPORTANTE: Si la ESP32 no conecta,")
            self._log("      mantén presionado el botón  [ BOOT ]")
            self._log("      de la placa mientras esto carga.")
            self._log("─" * 46)
            self._log("    Conectando con la ESP32...")

            esptool_cmd = self._find_esptool()
            cmd = esptool_cmd + [
                "--chip", "esp32",
                "--port", port,
                "--baud", "921600",
                "--before", "default_reset",
                "--after", "hard_reset",
                "write_flash", "-z",
                "--flash_mode", "dio",
                "--flash_freq", "40m",
                "--flash_size", "detect",
                "0x1000",         boot_bin,
                "0x8000",         part_bin,
                "0xe000",         app0_bin,
                "0x10000",        fw_bin,
                hex(SPIFFS_ADDR), spiffs_bin,
            ]

            proc = subprocess.Popen(
                cmd, stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT, text=True, bufsize=1,
            )
            last_pct = -1
            for line in proc.stdout:
                line = line.rstrip()
                if not line:
                    continue
                m = re.search(r'\((\d+)\s*%\)', line)
                if m:
                    pct = int(m.group(1))
                    if pct != last_pct and pct % 10 == 0:
                        self._log(f"    📤 Progreso: {pct}%")
                        last_pct = pct
                elif any(k in line for k in ("Chip is", "MAC:", "Features:")):
                    self._log(f"    {line}")
                elif "error" in line.lower():
                    self._log(f"    ❌ {line}")

            proc.wait()
            if proc.returncode != 0:
                raise RuntimeError(
                    "El flasheo falló. Posibles causas:\n\n"
                    "1. Desconecta y reconecta el cable USB de la ESP32\n"
                    "   luego pulsa 🔄 Actualizar y vuelve a intentarlo.\n\n"
                    "2. Cierra el monitor serial si está abierto en otro programa.\n\n"
                    "3. Si sigue fallando: mantén presionado el botón BOOT\n"
                    "   de la ESP32, presiona RESET, suelta RESET, suelta BOOT\n"
                    "   y luego presiona ⚡ FLASHEAR inmediatamente."
                )

            self._log("\n✅  ¡Firmware subido correctamente!")
            self._log("\n🔄  Esperando que la ESP32 arranque y se conecte al WiFi...")

            # ── 5. Monitor serial → detectar IP ──────
            self._monitor_serial(port)

        except FileNotFoundError as e:
            msg = str(e)
            self._log(f"\n❌  {msg}")
            self.after(0, lambda m=msg: messagebox.showerror("Archivos no encontrados", m, parent=self))

        except Exception as e:
            msg = str(e)
            self._log(f"\n❌  Error: {msg}")
            self.after(0, lambda m=msg: messagebox.showerror("Error durante el flasheo", m, parent=self))

        finally:
            shutil.rmtree(tmpdir, ignore_errors=True)
            self.flashing = False
            self.after(0, lambda: self.flash_btn.configure(
                state="normal", text="⚡   FLASHEAR ESP32"))

    def _monitor_serial(self, port: str, timeout: int = 40):
        """Lee el serial buscando la IP asignada a la ESP32."""
        try:
            time.sleep(3)
            ser = serial.Serial(port, 115200, timeout=1)
            deadline = time.time() + timeout
            self._log("📡  Monitoreando serial...")

            while time.time() < deadline:
                try:
                    line = ser.readline().decode("utf-8", errors="ignore").strip()
                except Exception:
                    line = ""
                if not line:
                    continue

                self._log(f"    {line}")

                # Detectar IP en cualquier formato conocido
                m = re.search(r'IP[:\s]+(\d{1,3}(?:\.\d{1,3}){3})', line)
                if m:
                    ip = m.group(1)
                    self.found_ip = ip
                    self._log(f"\n🌐  ¡ESP32 en línea!  →  http://{ip}")
                    self._log("    Abre esa dirección desde cualquier dispositivo")
                    self._log("    conectado a tu misma red WiFi.")
                    ser.close()
                    self.after(0, lambda _ip=ip: self._show_ip_banner(_ip))
                    return

            ser.close()
            self._log("\n⚠️   IP no detectada automáticamente en el tiempo esperado.")
            self._log("    Si el WiFi no conectó, la ESP32 crea su propia red de respaldo:")
            self._log("    → Busca la red:  ESP32-Config")
            self._log("    → Contraseña:    config123")
            self._log("    → Luego abre:    http://192.168.4.1")

        except Exception as e:
            self._log(f"\n⚠️   Monitor serial: {e}")

    def _show_ip_banner(self, ip: str):
        self.ip_label.configure(text=f"✅   ESP32 conectada  →  http://{ip}")
        self.ip_banner.grid()

    # ═══════════════════════════════════════════════════════════════
    # Búsqueda de herramientas (mkspiffs, esptool)
    # ═══════════════════════════════════════════════════════════════

    def _find_mkspiffs(self) -> str:
        # 1. Empaquetado en el .exe (cualquier variante de nombre)
        for candidate in Path(BIN_DIR).glob("mkspiffs*.exe"):
            return str(candidate)
        # 2. Paquetes de PlatformIO (~/.platformio/packages/tool-mkspiffs*)
        #    Prueba primero la variante esp32+arduino, luego cualquier otra.
        pio_pkg = Path.home() / ".platformio" / "packages"
        preferred = [
            "mkspiffs_espressif32_arduino.exe",
            "mkspiffs_espressif32_espidf.exe",
            "mkspiffs.exe",
        ]
        for pkg in pio_pkg.glob("tool-mkspiffs*"):
            for name in preferred:
                exe = pkg / name
                if exe.exists():
                    return str(exe)
            # Fallback: primer .exe encontrado en el paquete
            for exe in pkg.glob("mkspiffs*.exe"):
                return str(exe)
        raise FileNotFoundError(
            "No se encontró mkspiffs.exe.\n"
            "Ejecuta 'pio run --target uploadfs' en el proyecto al menos una vez\n"
            "para que PlatformIO lo descargue automáticamente."
        )

    def _find_esptool(self) -> list:
        # 1. Empaquetado
        p = Path(BIN_DIR) / "esptool.exe"
        if p.exists():
            return [str(p)]
        # 2. PlatformIO tool-esptoolpy
        pio_pkg = Path.home() / ".platformio" / "packages"
        for pkg in pio_pkg.glob("tool-esptoolpy*"):
            script = pkg / "esptool.py"
            if script.exists():
                return [sys.executable, str(script)]
        # 3. Módulo Python instalado
        return [sys.executable, "-m", "esptool"]


# ─────────────────────────────────────────────────────────────────────────────

if __name__ == "__main__":
    app = Flasher()
    app.mainloop()
