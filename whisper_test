import tkinter as tk
from tkinter import ttk, messagebox
import threading
import numpy as np
import sounddevice as sd
from scipy.io.wavfile import write
import tempfile
import os
import time

# Lazy import
whisper = None


class WhisperMicApp:
    def __init__(self, root):
        self.root = root
        self.root.title("Whisper Mic")
        self.root.geometry("700x500")

        # --- State ---
        self.recording = False
        self.audio_data = []
        self.samplerate = 16000
        self.model_ready = threading.Event()
        self.stop_event = threading.Event()
        self.animating = False
        self.animation_q = []

        # --- UI ---
        self._build_ui()

        # --- Background: load model ---
        threading.Thread(target=self._load_model, daemon=True).start()

    # ==================== UI ====================

    def _build_ui(self):
        frame = ttk.Frame(self.root)
        frame.pack(fill=tk.BOTH, expand=True, padx=12, pady=12)

        # Scrollable text area
        self.text = tk.Text(frame, wrap="word")
        self.text.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        self.text.tag_configure("header", font=("TkDefaultFont", 10, "bold"))
        self.text.tag_configure("mono", font=("Courier", 10))
        scrollbar = ttk.Scrollbar(frame, command=self.text.yview)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
        self.text.config(yscrollcommand=scrollbar.set)

        # Microphone button
        self.mic_button = ttk.Button(self.root, text="🎤 Start Recording", command=self._toggle_recording)
        self.mic_button.pack(pady=8)

        # Status label
        self.status_var = tk.StringVar(value="Loading Whisper model...")
        ttk.Label(self.root, textvariable=self.status_var).pack()

    # ==================== Whisper ====================

    def _load_model(self):
        global whisper
        try:
            import whisper as _w

            whisper = _w
            self._set_status("Loading Whisper model (turbo)...")
            self.model = whisper.load_model("turbo")
            self.model_ready.set()
            self._set_status("Model ready.")
        except Exception as e:
            self._set_status(f"Error loading model: {e}")
            messagebox.showerror("Error", str(e))

    # ==================== Recording ====================

    def _toggle_recording(self):
        if not self.model_ready.is_set():
            messagebox.showinfo("Please wait", "Model is still loading...")
            return

        if not self.recording:
            self.recording = True
            self.audio_data = []
            self.mic_button.config(text="🛑 Stop Recording")
            self._set_status("Recording... click again to stop.")
            threading.Thread(target=self._record_audio, daemon=True).start()
        else:
            self.recording = False
            self.mic_button.config(text="🎤 Start Recording")
            self._set_status("Processing audio...")
            threading.Thread(target=self._transcribe_audio, daemon=True).start()

    def _record_audio(self):
        def callback(indata, frames, time, status):
            if status:
                print(status)
            if self.recording:
                self.audio_data.append(indata.copy())

        with sd.InputStream(samplerate=self.samplerate, channels=1, callback=callback):
            while self.recording:
                sd.sleep(100)

    def _transcribe_audio(self):
        if not self.audio_data:
            self._set_status("No audio recorded.")
            return

        try:
            import scipy.io.wavfile as wavfile

            audio = np.concatenate(self.audio_data, axis=0)
            with tempfile.NamedTemporaryFile(suffix=".wav", delete=False) as tmp:
                tmp_path = tmp.name
                wavfile.write(tmp_path, self.samplerate, (audio * 32767).astype(np.int16))

            result = self.model.transcribe(tmp_path)
            os.remove(tmp_path)

            text = result.get("text", "").strip() or "(No text detected)"
            self._append_with_typing("New Recording", text)
            self._set_status("Transcription complete.")
        except Exception as e:
            self._set_status(f"Error: {e}")

    # ==================== UI helpers ====================

    def _append_with_typing(self, header, body):
        self.text.insert(tk.END, f"\n=== {header} ===\n", ("header",))
        idx = {"i": 0}
        chunk = 3  # chars per frame
        delay_ms = 10

        def step():
            i = idx["i"]
            if i >= len(body):
                self.text.insert(tk.END, "\n", ("mono",))
                self.text.see(tk.END)
                return
            j = min(i + chunk, len(body))
            self.text.insert(tk.END, body[i:j], ("mono",))
            self.text.see(tk.END)
            idx["i"] = j
            self.root.after(delay_ms, step)

        step()

    def _set_status(self, msg):
        self.status_var.set(msg)

    def on_close(self):
        self.stop_event.set()
        self.root.destroy()


def main():
    root = tk.Tk()
    try:
        style = ttk.Style()
        if "vista" in style.theme_names():
            style.theme_use("vista")
        elif "clam" in style.theme_names():
            style.theme_use("clam")
    except Exception:
        pass

    app = WhisperMicApp(root)
    root.protocol("WM_DELETE_WINDOW", app.on_close)
    root.mainloop()


if __name__ == "__main__":
    main()
