# Comprimir 3 archivos (HTML, CSS Y JS) y escribirlos a una sola línea con interfaz gráfica

import tkinter as tk
from tkinter import filedialog, messagebox
import os
import gzip
import re
from bs4 import BeautifulSoup
import jsmin

# --- LOGICA DE PROCESAMIENTO ---

def procesarComprimir(htmlPath, cssPath, jsPath, destPath):
    # Lee, minifica, combina y comprime los archivos web en un solo .html.gz
    try:
        # 1. Leer el contenido de los archivos originales
        with open(htmlPath, 'r', encoding='utf-8') as f:
            htmlContent = f.read()
        with open(cssPath, 'r', encoding='utf-8') as f:
            cssContent = f.read()
        with open(jsPath, 'r', encoding='utf-8') as f:
            jsContent = f.read()

        # 2. Minificar CSS (eliminar comentarios, saltos de linea y espacios extra)
        # Eliminar comentarios /* ... */
        cssMini = re.sub(r'/\*.*?\*/', '', cssContent, flags=re.DOTALL)
        # Eliminar saltos de linea y tabulaciones
        cssMini = re.sub(r'[\r\n\t]', '', cssMini)
        # Eliminar espacios alrededor de caracteres especiales
        cssMini = re.sub(r'\s*([:;{}])\s*', r'\1', cssMini)
        # Reemplazar múltiples espacios con uno solo (aunque la mayoria ya se fue)
        cssMini = re.sub(r'\s+', ' ', cssMini).strip()

        # 3. Minificar JavaScript usando la libreria jsmin
        jsMini = jsmin.jsmin(jsContent)

        # 4. Combinar todo el HTML usando BeautifulSoup para robustez
        soup = BeautifulSoup(htmlContent, 'lxml')

        # Crear y añadir la etiqueta <style> en el <head>
        if soup.head:
            style_tag = soup.new_tag('style')
            style_tag .string = cssMini
            # Eliminar cualquier link a CSS existente si se desea
            for link_tag in soup.find_all('link', {'rel': 'stylesheet'}):
                link_tag.decompose()
            soup.head.append(style_tag)

        else:
            # Si no hay <head>, no se puede inyectar el CSS de forma limpia
            # Se podría crear uno, pero es mejor advertir
            raise ValueError("El archivo HTML no tiene una etiqueta <head>.")
        
        # Crear y añadir la etiqueta <script> al final del <body>
        if soup.body:
            script_tag = soup.new_tag('script')
            script_tag.string = jsMini
            # Eliminar cualquier script externo si se desea
            for s in soup.find_all('script', src=True):
                s.decompose()
            soup.body.append(script_tag)

        else:
            raise ValueError("El archivo HTML no tiene una etiqueta <body>.")
        
        # Obtener el HTML final como string
        final_html = str(soup)

        # 5. Comprimir el HTML final a Gzip
        gzipped_content = gzip.compress(final_html.encode('utf-8'))

        # 6. Guardar el archivo .gz en el destino
        output_filename = os.path.basename(htmlPath) + '.gz'
        outputPath = os.path.join(destPath, output_filename)

        with open(outputPath, 'wb') as f_out:
            f_out.write(gzipped_content)

        return f"¡Éxito! Archivo guardado en: {outputPath}"
    
    except FileNotFoundError as e:
        return f"Error: No se encontró el archivo {e.filename}"
    except Exception as e:
        return f"Ocurrió un error inesperado: {str(e)}"
    
# --- INTERFAZ GRÁFICA (GUI) ---

class App(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title("Compresor de interfaz GZ V2.0")
        self.geometry("600x250")

        # Variables para almacenar las rutas
        self.htmlPath = tk.StringVar()
        self.cssPath = tk.StringVar()
        self.jsPath = tk.StringVar()
        self.destPath = tk.StringVar()

        # Configuracion de la cuadricula
        self.grid_columnconfigure(1, weight=1)

        # -- Widgets --
        # HTML
        tk.Label(self, text="Archivo HTML:").grid(row=0, column=0, padx=10, pady=5, sticky="w")
        tk.Entry(self, textvariable=self.htmlPath).grid(row=0, column=1, padx=10, pady=5, sticky="ew")
        tk.Button(self, text="Buscar...", command=self.browse_html).grid(row=0, column=2, padx=10, pady=5)

        # CSS
        tk.Label(self, text="Archivo CSS:").grid(row=1, column=0, padx=10, pady=5, sticky="w")
        tk.Entry(self, textvariable=self.cssPath).grid(row=1, column=1, padx=10, pady=5, sticky="ew")
        tk.Button(self, text="Buscar...", command=self.browse_css).grid(row=1, column=2, padx=10, pady=5)
        
        # JS
        tk.Label(self, text="Archivo JS:").grid(row=2, column=0, padx=10, pady=5, sticky="w")
        tk.Entry(self, textvariable=self.jsPath).grid(row=2, column=1, padx=10, pady=5, sticky="ew")
        tk.Button(self, text="Buscar...", command=self.browse_js).grid(row=2, column=2, padx=10, pady=5)
        
        # Destino
        tk.Label(self, text="Carpeta Destino:").grid(row=3, column=0, padx=10, pady=5, sticky="w")
        tk.Entry(self, textvariable=self.destPath).grid(row=3, column=1, padx=10, pady=5, sticky="ew")
        tk.Button(self, text="Buscar...", command=self.browse_dest).grid(row=3, column=2, padx=10, pady=5)

        # Botón de Procesar
        tk.Button(self, text="🚀 Procesar y Comprimir", command=self.run_process, bg="#4CAF50", fg="white", font=('Helvetica', 10, 'bold')).grid(row=4, column=0, columnspan=3, pady=20, padx=10, sticky="ew")

    def browse_html(self):
        path = filedialog.askopenfilename(filetypes=[("HTML files", "*.html"), ("All files", "*.*")])
        if path: self.htmlPath.set(path)

    def browse_css(self):
        path = filedialog.askopenfilename(filetypes=[("CSS files", "*.css"), ("All files", "*.*")])
        if path: self.cssPath.set(path)

    def browse_js(self):
        path = filedialog.askopenfilename(filetypes=[("JavaScript files", "*.js"), ("All files", "*.*")])
        if path: self.jsPath.set(path)

    def browse_dest(self):
        path = filedialog.askdirectory()
        if path: self.destPath.set(path)

    def run_process(self):
        html = self.htmlPath.get()
        css = self.cssPath.get()
        js = self.jsPath.get()
        dest = self.destPath.get()

        if not all([html, css, js, dest]):
            messagebox.showwarning("Campos incompletos", "Por favor, selecciona todos los archivos y la carpeta de destino.")
            return
        
        resultMsg = procesarComprimir(html, css, js, dest)

        if resultMsg.startswith("¡Éxito!"):
            messagebox.showinfo("Proceso completado", resultMsg)
        else:
            messagebox.showerror("Error", resultMsg)

if __name__ == "__main__":
    app = App()
    app.mainloop()
